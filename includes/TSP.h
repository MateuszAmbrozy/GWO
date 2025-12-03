#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <numeric>
#include <cstddef>
#include <future>
#include <chrono>


#include "Point.h"
#include "Wolf.h"

class TSP
{
private:
    static constexpr int MAX_COORD_DIST = 2000;
    static constexpr double EXPLOIT_BOUND_1 = 0.33;
    static constexpr double EXPLOIT_BOUND_2 = 0.66;


    int numCities;
    std::vector<Point> cities;
    std::vector<double> distance_matrix;
    std::vector<Point> bestRoute;
    std::vector<int> visited;
    int timestamp;
    int populationSize;
    int maxIterations;

    Wolf alpha_wolf;
    Wolf beta_wolf;
    Wolf delta_wolf;
    std::vector<Wolf> population;
    double bestLength;

    std::mt19937 rand_gen;
    std::uniform_real_distribution<double> unif_dist;
    std::uniform_int_distribution<int> city_dist;
    std::uniform_int_distribution<int> coord_dist;


    void generate_random_cities(int n)
    {
        if (n <= 0) {
            std::cout << "ERROR::TSP::generate_random_path: n < 0\n";
            return;
        }

        cities.clear();
        cities.resize(static_cast<std::size_t>(n));

        int num_threads = std::thread::hardware_concurrency();
        if (num_threads <= 0) num_threads = 1;

        int chunk = n / num_threads;

        std::vector<std::future<void>> futures;

        for (int t = 0; t < num_threads; t++) {
            int start = t * chunk;
            int end = (t == num_threads - 1) ? n : start + chunk;

            futures.push_back(std::async(std::launch::async, [this, start, end]() {
                std::mt19937 local_gen(std::random_device{}());
                std::uniform_int_distribution<int> local_dist(coord_dist);

                for (int i = start; i < end; i++) {
                    int x = local_dist(local_gen);
                    int y = local_dist(local_gen);
                    cities[i] = Point(x, y);
                }
                }));
        }

        for (auto& f : futures) f.get();
    }

    void build_distance_matrix()
    {
        std::size_t N = static_cast<std::size_t>(numCities);
        std::size_t total = N * N;
        distance_matrix.assign(total, 0.0);

        int num_threads = std::thread::hardware_concurrency();
        if (num_threads <= 0) num_threads = 1;

        int chunk = numCities / num_threads;

        std::vector<std::future<void>> futures;
        futures.reserve(num_threads);

        for (int t = 0; t < num_threads; t++) {
            int i_start = t * chunk;
            int i_end = (t == num_threads - 1) ? numCities : i_start + chunk;

            futures.push_back(std::async(std::launch::async, [this, i_start, i_end, N]() {

                for (int i = i_start; i < i_end; i++) {
                    std::size_t idx_i = static_cast<std::size_t>(i);

                    for (int j = i + 1; j < numCities; j++) {
                        std::size_t idx_j = static_cast<std::size_t>(j);

                        double d_squared = cities[idx_i].dist(cities[idx_j]);

                        distance_matrix[idx_i * N + idx_j] = d_squared;
                        distance_matrix[idx_j * N + idx_i] = d_squared;
                    }
                }

                }));
        }

        for (auto& f : futures) f.get();
    }


    double calculate_route_length(const std::vector<int>& route) const
    {
        double length = 0.0;
        std::size_t n = route.size();
        std::size_t cities_count = static_cast<std::size_t>(numCities);
        for (std::size_t i = 0U; i + 1U < n; ++i)
        {
            std::size_t idx_from = static_cast<std::size_t>(route[i]);
            std::size_t idx_to = static_cast<std::size_t>(route[i + 1U]);
            length += distance_matrix[idx_from * cities_count + idx_to];
        }
        std::size_t last_idx = static_cast<std::size_t>(route[n - 1U]);
        std::size_t first_idx = static_cast<std::size_t>(route[0]);
        length += distance_matrix[last_idx * cities_count + first_idx];
        return length;
    }

    void update_wolf_fitness(Wolf& wolf)
    {
        const std::vector<int>& r = wolf.getRoute();
        double fitness = calculate_route_length(r);
        wolf.setFitness(fitness);
    }

    void initialize_population()
    {
        population.clear();
        population.resize(static_cast<std::size_t>(populationSize));

        int num_threads = std::thread::hardware_concurrency();
        if (num_threads <= 0) num_threads = 1;

        int chunk = populationSize / num_threads;

        std::vector<std::future<void>> futures;
        futures.reserve(num_threads);

        for (int t = 0; t < num_threads; t++) {
            int start = t * chunk;
            int end = (t == num_threads - 1) ? populationSize : start + chunk;

            futures.push_back(std::async(std::launch::async,
                [this, start, end]()
                {
                    for (int i = start; i < end; i++) {
                        population[i] = Wolf(numCities);       // generuje trasę
                        update_wolf_fitness(population[i]);    // liczy dystans
                    }
                }));
        }

        for (auto& f : futures) f.get();

        alpha_wolf = population[0];
        beta_wolf = population[1];
        delta_wolf = population[2];
        update_leaders();

        bestLength = alpha_wolf.getFitness();
    }


    void move_towards(const std::vector<int>& current_route,
        const std::vector<int>& target_route,
        std::vector<int>& child,
        std::vector<int>& visited,
        int& timestamp)
    {
        int n = numCities;
        timestamp++;

        for (int i = 0; i < n; i++)
            child[i] = -1;

        thread_local std::mt19937 rng(std::random_device{}());
        thread_local std::uniform_int_distribution<int> city_dist(0, numCities - 1);

        int start = city_dist(rng);
        int end = city_dist(rng);
        if (start > end) std::swap(start, end);

        for (int i = start; i <= end; i++) {
            int c = target_route[i];
            child[i] = c;
            visited[c] = timestamp;
        }

        int scan_pos = (end + 1) % n;
        int ins_pos = (end + 1) % n;

        for (int k = 0; k < n; k++) {
            int c = current_route[scan_pos];
            if (visited[c] != timestamp) {
                child[ins_pos] = c;
                visited[c] = timestamp;
                ins_pos = (ins_pos + 1) % n;
            }
            scan_pos = (scan_pos + 1) % n;
        }
    }

    void update_omega_wolf(Wolf& wolf, double a)
    {
        // thread-local RNG
        thread_local std::mt19937 rng(std::random_device{}());
        thread_local std::uniform_real_distribution<double> unif(0.0, 1.0);
        std::uniform_int_distribution<int> city_dist(0, numCities - 1);

        std::vector<int> tmp_route(numCities);
        std::vector<int> visited(numCities, -1);
        int timestamp = 1;

        const auto& current_route = wolf.getRoute();

        double old_fitness = wolf.getFitness();
        double new_fitness = old_fitness;

        // Ręczne ograniczenie p_explore w przedziale [0, 1]
        double p_explore = 0.5 * a;
        if (p_explore < 0.0) {
            p_explore = 0.0;
        }
        else if (p_explore > 1.0) {
            p_explore = 1.0;
        }

        // Losowanie r_mode
        double r_mode = unif(rng);

        if (r_mode < p_explore)
        {
            int i = city_dist(rng);
            int j = city_dist(rng);
            while (i == j) j = city_dist(rng);
            if (i > j) std::swap(i, j);

            int city_A_idx = (i - 1 + numCities) % numCities;
            int city_B_idx = i;
            int city_C_idx = j;
            int city_D_idx = (j + 1) % numCities;

            std::size_t idx_A = current_route[city_A_idx];
            std::size_t idx_B = current_route[city_B_idx];
            std::size_t idx_C = current_route[city_C_idx];
            std::size_t idx_D = current_route[city_D_idx];

            double old_edges_cost =
                distance_matrix[idx_A * numCities + idx_B] +
                distance_matrix[idx_C * numCities + idx_D];

            double new_edges_cost =
                distance_matrix[idx_A * numCities + idx_C] +
                distance_matrix[idx_B * numCities + idx_D];

            new_fitness = old_fitness + (new_edges_cost - old_edges_cost);

            std::copy(current_route.begin(), current_route.end(), tmp_route.begin());
            std::reverse(tmp_route.begin() + i, tmp_route.begin() + j + 1);
        }
        else
        {
            double r = unif(rng);

            if (r < EXPLOIT_BOUND_1)
                move_towards(current_route, alpha_wolf.getRoute(), tmp_route, visited, timestamp);
            else if (r < EXPLOIT_BOUND_2)
                move_towards(current_route, beta_wolf.getRoute(), tmp_route, visited, timestamp);
            else
                move_towards(current_route, delta_wolf.getRoute(), tmp_route, visited, timestamp);

            new_fitness = calculate_route_length(tmp_route);
        }

        if (new_fitness < old_fitness)
        {
            wolf.setRoute(tmp_route);
            wolf.setFitness(new_fitness);
        }
    }

    void update_leaders()
    {
        int num_threads = std::thread::hardware_concurrency();
        if (num_threads <= 0) num_threads = 1;

        int chunk = populationSize / num_threads;

        struct Top3Local {
            Wolf alpha;
            Wolf beta;
            Wolf delta;
            double af, bf, df;
        };

        std::vector<std::future<Top3Local>> futures;
        futures.reserve(num_threads);

        // --- FAZA RÓWNOLEGŁA ---
        for (int t = 0; t < num_threads; t++)
        {
            int start = t * chunk;
            int end = (t == num_threads - 1) ? populationSize : start + chunk;

            futures.push_back(std::async(std::launch::async,
                [this, start, end]() -> Top3Local
                {
                    Top3Local loc;
                    loc.af = std::numeric_limits<double>::infinity();
                    loc.bf = std::numeric_limits<double>::infinity();
                    loc.df = std::numeric_limits<double>::infinity();

                    for (int i = start; i < end; i++)
                    {
                        Wolf& w = population[i];
                        double f = w.getFitness();

                        if (f < loc.af) {
                            loc.delta = loc.beta;
                            loc.df = loc.bf;

                            loc.beta = loc.alpha;
                            loc.bf = loc.af;

                            loc.alpha = w;
                            loc.af = f;
                        }
                        else if (f < loc.bf) {
                            loc.delta = loc.beta;
                            loc.df = loc.bf;

                            loc.beta = w;
                            loc.bf = f;
                        }
                        else if (f < loc.df) {
                            loc.delta = w;
                            loc.df = f;
                        }
                    }

                    return loc;
                }));
        }

        // --- FAZA MERGE (SEKWENCYJNA) ---
        Wolf a, b, d;
        double af = std::numeric_limits<double>::infinity();
        double bf = std::numeric_limits<double>::infinity();
        double df = std::numeric_limits<double>::infinity();

        for (auto& fut : futures)
        {
            Top3Local loc = fut.get();

            auto try_insert = [&](Wolf& w, double f)
                {
                    if (f < af) {
                        d = b; df = bf;
                        b = a; bf = af;
                        a = w; af = f;
                    }
                    else if (f < bf) {
                        d = b; df = bf;
                        b = w; bf = f;
                    }
                    else if (f < df) {
                        d = w; df = f;
                    }
                };

            try_insert(loc.alpha, loc.af);
            try_insert(loc.beta, loc.bf);
            try_insert(loc.delta, loc.df);
        }

        // --- USTAWIENIE GLOBALNYCH LIDERÓW ---
        alpha_wolf = a;
        beta_wolf = b;
        delta_wolf = d;
    }


public:
    TSP(int n, int popSize = 50, int iterations = 1000)
        : numCities(n),
        timestamp(0),
        populationSize(popSize),
        maxIterations(iterations),
        bestLength(std::numeric_limits<double>::max()),
        rand_gen(std::random_device{}()),
        unif_dist(0.0, 1.0),
        city_dist(0, n - 1),
        coord_dist(0, MAX_COORD_DIST)
    {
        if (n <= 0)
        {
            std::cout << "ERROR::TSP::TSP n <= 0\n";
            return;
        }
        if (popSize <= 3)
        {
            std::cout << "ERROR::TSP::TSP populacja musi byc > 3\n";
            populationSize = 4;
        }
        visited.assign(static_cast<std::size_t>(n), -1);
        distance_matrix.resize(static_cast<std::size_t>(numCities) * static_cast<std::size_t>(numCities));
        generate_random_cities(n);
        build_distance_matrix();
        initialize_population();
    }

    std::vector<Point> solve() {
        std::cout << "Rozpoczynanie algorytmu GWO..." << std::endl;

        for (int t = 0; t < maxIterations; ++t)
        {
            double progress = double(t) / double(maxIterations);
            double a = 2.0 * std::pow(1.0 - progress, 2.0);

            int num_threads = std::thread::hardware_concurrency();
            if (num_threads <= 0) num_threads = 1;

            int chunk = (populationSize - 3) / num_threads;

            std::vector<std::future<void>> futures;
            futures.reserve(num_threads);

            // --- równoległe aktualizowanie wilków omega ---
            for (int th = 0; th < num_threads; th++)
            {
                int start = 3 + th * chunk;
                int end = (th == num_threads - 1) ? populationSize : start + chunk;

                futures.push_back(
                    std::async(std::launch::async,
                        [this, start, end, a]()
                        {
                            for (int i = start; i < end; i++)
                            {
                                update_omega_wolf(population[i], a);
                            }
                        }
                    )
                );
            }

            for (auto& f : futures) f.get();

            // zrównoleglenie leaderów (masz je już gotowe)
            update_leaders();

            if (alpha_wolf.getFitness() < bestLength)
            {
                bestLength = alpha_wolf.getFitness();
            }
        }

        std::cout << "GWO zakonczone. Najlepsza znaleziona dlugosc: " << bestLength << std::endl;
        std::vector<Point> bestRoutePoints;
        const std::vector<int>& bestRouteIdx = alpha_wolf.getRoute();
        bestRoutePoints.reserve(bestRouteIdx.size());
        for (std::size_t k = 0U; k < bestRouteIdx.size(); ++k) {
            int city_index = bestRouteIdx[k];
            bestRoutePoints.push_back(cities[static_cast<std::size_t>(city_index)]);
        }
        bestRoute = bestRoutePoints;
        return bestRoutePoints;
    }

    void print_solution()
    {
        std::cout << "Najlepsza znaleziona trasa (Dlugosc: " << std::sqrt(bestLength) << "):" << std::endl;
        alpha_wolf.printRoute();
    }

    std::vector<Point> get_best_route() const
    {
        return bestRoute;
    }

    double get_best_path_length() const
    {
        return bestLength;
    }
};
