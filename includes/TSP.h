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
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <type_traits>
#include <utility>
#include <omp.h>
#include "Point.h"
#include "Wolf.h"

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads) : stop(false) {
        for (std::size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this]() {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this]() { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

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
    int num_threads;
    Wolf alpha_wolf;
    Wolf beta_wolf;
    Wolf delta_wolf;
    std::vector<Wolf> population;
    double bestLength;
    ThreadPool pool;

    std::mt19937 rand_gen;
    std::uniform_real_distribution<double> unif_dist;
    std::uniform_int_distribution<int> city_dist;
    std::uniform_int_distribution<int> coord_dist;


    void generate_random_cities(int n) {
        if (n <= 0) {
            std::cout << "ERROR::TSP::generate_random_path: n < 0\n";
            return;
        }
        cities.clear();
        cities.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; i++) {
            int x = coord_dist(rand_gen);
            int y = coord_dist(rand_gen);
            cities.push_back(Point(x, y));
        }
    }

    void build_distance_matrix() {
        std::size_t total = static_cast<std::size_t>(numCities) * static_cast<std::size_t>(numCities);
        distance_matrix.assign(total, 0.0);
        for (int i = 0; i < numCities; i++) {
            std::size_t idx_i = static_cast<std::size_t>(i);
            for (int j = i + 1; j < numCities; j++) {
                std::size_t idx_j = static_cast<std::size_t>(j);
                double d_squared = cities[idx_i].dist(cities[idx_j]);
                distance_matrix[idx_i * static_cast<std::size_t>(numCities) + idx_j] = d_squared;
                distance_matrix[idx_j * static_cast<std::size_t>(numCities) + idx_i] = d_squared;
            }
        }
    }


    double calculate_route_length(const std::vector<int>& route) const {
        double length = 0.0;
        std::size_t n = route.size();
        std::size_t cities_count = static_cast<std::size_t>(numCities);
        for (std::size_t i = 0U; i + 1U < n; ++i) {
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
        population.reserve(static_cast<std::size_t>(populationSize));
        for (int i = 0; i < populationSize; ++i) {
            population.emplace_back(numCities);
            update_wolf_fitness(population.back());
        }
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

        int start = city_dist(rand_gen);
        int end = city_dist(rand_gen);
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
        std::vector<int> tmp_route(numCities);
        std::vector<int> visited(numCities, -1);
        int timestamp = 1;

        const auto& current_route = wolf.getRoute();

        double old_fitness = wolf.getFitness();
        double new_fitness = old_fitness;
        double p_explore = 0.5 * a;
        if (p_explore < 0.0) {
            p_explore = 0.0;
        } else if (p_explore > 1.0) {
            p_explore = 1.0;
        }
        double r_mode = unif_dist(rand_gen);
        if (r_mode < p_explore) {
            int i = city_dist(rand_gen);
            int j = city_dist(rand_gen);
            while (i == j) {
                j = city_dist(rand_gen);
            }
            if (i > j) {
                int tmp = i;
                i = j;
                j = tmp;
            }
            int city_A_idx = (i - 1 + numCities) % numCities;
            int city_B_idx = i;
            int city_C_idx = j;
            int city_D_idx = (j + 1) % numCities;

            std::size_t idx_A = static_cast<std::size_t>(current_route[static_cast<std::size_t>(city_A_idx)]);
            std::size_t idx_B = static_cast<std::size_t>(current_route[static_cast<std::size_t>(city_B_idx)]);
            std::size_t idx_C = static_cast<std::size_t>(current_route[static_cast<std::size_t>(city_C_idx)]);
            std::size_t idx_D = static_cast<std::size_t>(current_route[static_cast<std::size_t>(city_D_idx)]);

            std::size_t cities_count = static_cast<std::size_t>(numCities);

            double old_edges_cost = distance_matrix[idx_A * cities_count + idx_B] +
                                    distance_matrix[idx_C * cities_count + idx_D];
            double new_edges_cost = distance_matrix[idx_A * cities_count + idx_C] +
                                    distance_matrix[idx_B * cities_count + idx_D];
            double delta = new_edges_cost - old_edges_cost;
            new_fitness = old_fitness + delta;

            std::copy(current_route.begin(), current_route.end(), tmp_route.begin());
            std::reverse(tmp_route.begin() + static_cast<std::ptrdiff_t>(i),
                         tmp_route.begin() + static_cast<std::ptrdiff_t>(j) + static_cast<std::ptrdiff_t>(1));
        } else {
            double r = unif_dist(rand_gen);
            if (r < EXPLOIT_BOUND_1) {
                move_towards(current_route, alpha_wolf.getRoute(), tmp_route, visited, timestamp);
            } else if (r < EXPLOIT_BOUND_2) {
                move_towards(current_route, beta_wolf.getRoute(), tmp_route, visited, timestamp);
            } else {
                move_towards(current_route, delta_wolf.getRoute(), tmp_route, visited, timestamp);
            }
            new_fitness = calculate_route_length(tmp_route);
        }
        if (new_fitness < old_fitness) {
            wolf.setRoute(tmp_route);
            wolf.setFitness(new_fitness);
        }
    }

    void update_leaders() {
        for (int i = 0; i < populationSize; i++) {
            Wolf& w = population[static_cast<std::size_t>(i)];
            double f = w.getFitness();
            if (f < alpha_wolf.getFitness()) {
                delta_wolf = beta_wolf;
                beta_wolf = alpha_wolf;
                alpha_wolf = w;
            } else if (f < beta_wolf.getFitness()) {
                delta_wolf = beta_wolf;
                beta_wolf = w;
            } else if (f < delta_wolf.getFitness()) {
                delta_wolf = w;
            }
        }
    }

public:
    TSP(int n, int popSize = 50, int iterations = 1000, int num_threads = 4)
        : pool(num_threads),
        numCities(n),
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
        this->num_threads = num_threads;
        if (this->num_threads <= 0) {
            this->num_threads = 1;
        }

        visited.assign(static_cast<std::size_t>(n), -1);
        distance_matrix.resize(static_cast<std::size_t>(numCities) * static_cast<std::size_t>(numCities));
        generate_random_cities(n);
        build_distance_matrix();
        initialize_population();
    }

    std::vector<Point> solve() {
        std::cout << "Rozpoczynanie algorytmu GWO..." << std::endl;

        int chunk = (populationSize - 3) / num_threads;
        for (int t = 0; t < maxIterations; ++t)
        {
            double progress = double(t) / double(maxIterations);
            double a = 2.0 * std::pow(1.0 - progress, 2.0);

            std::vector<std::future<void>> futures;
            futures.reserve(num_threads);

            for (int th = 0; th < num_threads; th++)
            {
                int start = 3 + th * chunk;
                int end = (th == num_threads - 1) ? populationSize : start + chunk;

                futures.push_back(
                    pool.enqueue(
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
