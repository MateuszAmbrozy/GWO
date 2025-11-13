#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include <stdlib.h>
#include <random>
#include <time.h>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <numeric>

#include "Point.h" 
#include "Wolf.h"

class TSP {
private:
    int numCities;
    std::vector<Point> cities;
    std::vector<std::vector<double>> distance_matrix;
    std::vector<Point> bestRoute;

    int populationSize;
    int maxIterations;

    Wolf alfa;
    Wolf beta;
    Wolf delta;
    std::vector<Wolf> omega;

    double bestLength;

    std::mt19937 rand_gen;
    std::uniform_real_distribution<double> unif_dist;
    std::uniform_int_distribution<int> city_dist;

    void generate_random_cities(int n) {
        if (n <= 0) {
            std::cout << "ERROR::TSP::generate_random_path: n < 0\n";
            return;
        }
        cities.clear();
        for (int i = 0; i < n; i++) {
            int x = static_cast<int>(rand_gen() % 1000); 
            int y = static_cast<int>(rand_gen() % 1000);
            this->cities.push_back(Point(x, y));
        }
    }

    void build_distance_matrix() {
        distance_matrix.assign(static_cast<size_t>(numCities), std::vector<double>(static_cast<size_t>(numCities), 0.0));
        for (int i = 0; i < numCities; i++) {
            for (int j = i + 1; j < numCities; j++) {
                double d = cities[static_cast<size_t>(i)].dist(cities[static_cast<size_t>(j)]);
                distance_matrix[static_cast<size_t>(i)][static_cast<size_t>(j)] = d;
                distance_matrix[static_cast<size_t>(j)][static_cast<size_t>(i)] = d;
            }
        }
    }

    double calculate_route_length(const std::vector<int>& route) {
        double length = 0.0;
        for (size_t i = 0; i < route.size() - 1U; ++i) { 
            length += distance_matrix[static_cast<size_t>(route[i])][static_cast<size_t>(route[i + 1U])];
        }
        length += distance_matrix[static_cast<size_t>(route[route.size() - 1U])][static_cast<size_t>(route[0])]; 
        return length;
    }

    void update_wolf_fitness(Wolf& wolf) {
        double fitness = calculate_route_length(wolf.getRoute());
        wolf.setFitness(fitness);
    }

    void initialize_population() {
        std::vector<Wolf> full_pack;
        for (int i = 0; i < populationSize; ++i) {
            full_pack.emplace_back(numCities);
            update_wolf_fitness(full_pack.back());
        }
        std::sort(full_pack.begin(), full_pack.end());
        alfa = full_pack[0];
        beta = full_pack[1];
        delta = full_pack[2];
        bestLength = alfa.getFitness();
        omega.clear();
        for (int i = 3; i < populationSize; ++i) {
            omega.push_back(full_pack[static_cast<size_t>(i)]);
        }
    }

    std::vector<int> move_towards(const std::vector<int>& current_route,
                                    const std::vector<int>& target_route) {
        int n = numCities;
        std::vector<int> child(static_cast<size_t>(n), -1);
        
        std::vector<bool> visited(static_cast<size_t>(n), false);

        int start = city_dist(rand_gen);
        int end = city_dist(rand_gen);
        if (start > end) {
            std::swap(start, end);
        }

        for (int i = start; i <= end; ++i) {
            int city = target_route[static_cast<size_t>(i)];
            child[static_cast<size_t>(i)] = city;
            visited[static_cast<size_t>(city)] = true;
        }

        int current_scan_pos = (end + 1) % n; 
        int child_insert_pos = (end + 1) % n;

        for (int i = 0; i < n; ++i) {
            int city = current_route[static_cast<size_t>(current_scan_pos)];
            
            if (!visited[static_cast<size_t>(city)]) {
                child[static_cast<size_t>(child_insert_pos)] = city;
                visited[static_cast<size_t>(city)] = true;
                
                child_insert_pos = (child_insert_pos + 1) % n;
            }
            
            current_scan_pos = (current_scan_pos + 1) % n;
        }

        return child;
    }

    std::vector<int> inversion_mutation(const std::vector<int>& route) {
        std::vector<int> r = route;
        
        int i = city_dist(rand_gen);
        int j = city_dist(rand_gen);
        
        while (i == j) {
            j = city_dist(rand_gen);
        }
        
        if (i > j) {
            std::swap(i, j);
        }
        
        std::reverse(r.begin() + static_cast<long>(i), r.begin() + static_cast<long>(j) + 1);
        
        return r;
    }

    void update_omega_wolf(Wolf& wolf, double a) {
        std::vector<int> current_route = wolf.getRoute();
        std::vector<int> new_route;

        double A1 = a * (2.0 * unif_dist(rand_gen) - 1.0);
        double r = unif_dist(rand_gen);

        if (std::abs(A1) >= 1.0) {
            new_route = inversion_mutation(current_route);
        } 
        else {
            if (r < 0.33) {
                new_route = move_towards(current_route, alfa.getRoute());
            } else if (r < 0.66) {
                new_route = move_towards(current_route, beta.getRoute());
            } else {
                new_route = move_towards(current_route, delta.getRoute());
            }
        }

        double new_fitness = calculate_route_length(new_route);
        if (new_fitness < wolf.getFitness()) {
            wolf.setRoute(new_route);
            wolf.setFitness(new_fitness);
        }
    }

    void update_leaders() {
        std::vector<Wolf> full_pack = omega;
        full_pack.push_back(alfa);
        full_pack.push_back(beta);
        full_pack.push_back(delta);
        std::sort(full_pack.begin(), full_pack.end());
        alfa = full_pack[0];
        beta = full_pack[1];
        delta = full_pack[2];
        omega.clear();
        for (size_t i = 3U; i < full_pack.size(); ++i) { 
            omega.push_back(full_pack[i]);
        }
    }
public:
    TSP(int n, int popSize = 50, int iterations = 1000)
        : numCities(n),
          populationSize(popSize),
          maxIterations(iterations),
          bestLength(std::numeric_limits<double>::max()),
          rand_gen(std::random_device{}()),
          unif_dist(0.0, 1.0),
          city_dist(0, n - 1)
    {
        if (n <= 0) {
            std::cout << "ERROR::TSP::TSP n <= 0\n";
            return;
        }
        if (popSize <= 3) {
            std::cout << "ERROR::TSP::TSP populacja musi byc > 3\n";
            populationSize = 4;
        }
        this->generate_random_cities(n);
        this->build_distance_matrix();
        this->initialize_population();
    }

    std::vector<Point> solve() {
        std::cout << "Rozpoczynanie algorytmu GWO..." << std::endl;
        for (int t = 0; t < maxIterations; ++t) {
            double progress = static_cast<double>(t) / static_cast<double>(maxIterations); 
            double a = 2.0 * std::pow(1.0 - progress, 2.0);
            for (Wolf& wolf : omega) {
                update_omega_wolf(wolf, a);
            }
            update_leaders();
            if (alfa.getFitness() < bestLength) {
                bestLength = alfa.getFitness();
                std::cout << "Iteracja " << t << ": Nowa najlepsza trasa (GWO) = " << bestLength << std::endl;
            }
        }
        std::cout << "GWO zakonczone. Najlepsza znaleziona dlugosc: " << bestLength << std::endl;

        std::vector<Point> bestRoutePoints;
        for (int city_index : alfa.getRoute()) {
            bestRoutePoints.push_back(cities[static_cast<size_t>(city_index)]); 
        }
        bestRoute = bestRoutePoints; 
        return bestRoutePoints;
    }

    void print_solution() {
        std::cout << "Najlepsza znaleziona trasa (Dlugosc: " << bestLength << "):" << std::endl;
        alfa.printRoute();
    }

    std::vector<Point> get_best_route() const {
        return bestRoute;
    }

    double get_best_path_length() const {
        return bestLength;
    }
};