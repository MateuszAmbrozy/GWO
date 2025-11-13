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

#include "Point.h"
#include "Wolf.h"

class TSP {
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

    std::vector<int> tmp_route;

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

    void update_wolf_fitness(Wolf& wolf) {
        const std::vector<int>& r = wolf.getRoute();
        double fitness = calculate_route_length(r);
        wolf.setFitness(fitness);
    }

    void initialize_population() {
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
        tmp_route.assign(static_cast<std::size_t>(numCities), 0);
    }

    void move_towards(const std::vector<int>& current_route, const std::vector<int>& target_route, std::vector<int>& child) {
        int n = numCities;
        timestamp++;
        for (int i = 0; i < n; i++) {
            child[static_cast<std::size_t>(i)] = -1;
        }
        int start = city_dist(rand_gen);
        int end = city_dist(rand_gen);
        if (start > end) {
            int tmp = start;
            start = end;
            end = tmp;
        }
        for (int i = start; i <= end; i++) {
            std::size_t idx = static_cast<std::size_t>(i);
            int c = target_route[idx];
            child[idx] = c;
            visited[static_cast<std::size_t>(c)] = timestamp;
        }
        int scan_pos = end + 1;
        if (scan_pos >= n) {
            scan_pos = 0;
        }
        int ins_pos = end + 1;
        if (ins_pos >= n) {
            ins_pos = 0;
        }
        for (int k = 0; k < n; k++) {
            std::size_t scan_idx = static_cast<std::size_t>(scan_pos);
            int c = current_route[scan_idx];
            std::size_t c_idx = static_cast<std::size_t>(c);
            if (visited[c_idx] != timestamp) {
                std::size_t ins_idx = static_cast<std::size_t>(ins_pos);
                child[ins_idx] = c;
                visited[c_idx] = timestamp;
                ins_pos++;
                if (ins_pos == n) {
                    ins_pos = 0;
                }
            }
            scan_pos++;
            if (scan_pos == n) {
                scan_pos = 0;
            }
        }
    }

    void update_omega_wolf(Wolf& wolf, double a) {
        const std::vector<int>& current_route = wolf.getRoute();
        if (tmp_route.size() != static_cast<std::size_t>(numCities)) {
            tmp_route.resize(static_cast<std::size_t>(numCities));
        }
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
                move_towards(current_route, alpha_wolf.getRoute(), tmp_route);
            } else if (r < EXPLOIT_BOUND_2) {
                move_towards(current_route, beta_wolf.getRoute(), tmp_route);
            } else {
                move_towards(current_route, delta_wolf.getRoute(), tmp_route);
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
    TSP(int n, int popSize = 50, int iterations = 1000)
        : numCities(n),
          timestamp(0),
          populationSize(popSize),
          maxIterations(iterations),
          bestLength(std::numeric_limits<double>::max()),
          rand_gen(std::random_device{}()),
          unif_dist(0.0, 1.0),
          city_dist(0, n - 1),
          coord_dist(0, MAX_COORD_DIST) {
        if (n <= 0) {
            std::cout << "ERROR::TSP::TSP n <= 0\n";
            return;
        }
        if (popSize <= 3) {
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
        for (int t = 0; t < maxIterations; ++t) {
            double progress = static_cast<double>(t) / static_cast<double>(maxIterations);
            double a = 2.0 * std::pow(1.0 - progress, 2.0);
            for (int i = 3; i < populationSize; i++) {
                update_omega_wolf(population[static_cast<std::size_t>(i)], a);
            }
            update_leaders();
            if (alpha_wolf.getFitness() < bestLength) {
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

    void print_solution() {
        std::cout << "Najlepsza znaleziona trasa (Dlugosc: " << std::sqrt(bestLength) << "):" << std::endl;
        alpha_wolf.printRoute();
    }

    std::vector<Point> get_best_route() const {
        return bestRoute;
    }

    double get_best_path_length() const {
        return bestLength;
    }
};
