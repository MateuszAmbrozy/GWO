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

// Zakładam, że te pliki istnieją w Twoim projekcie
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
            int x = rand_gen() % 100;
            int y = rand_gen() % 100;
            this->cities.push_back(Point(x, y));
        }
    }

    void build_distance_matrix() {
        distance_matrix.assign(numCities, std::vector<double>(numCities, 0.0));
        for (int i = 0; i < numCities; i++) {
            for (int j = i + 1; j < numCities; j++) {
                double d = cities[i].dist(cities[j]);
                distance_matrix[i][j] = d;
                distance_matrix[j][i] = d;
            }
        }
    }

    double calculate_route_length(const std::vector<int>& route) {
        double length = 0.0;
        for (size_t i = 0; i < numCities - 1; ++i) {
            length += distance_matrix[route[i]][route[i + 1]];
        }
        length += distance_matrix[route[numCities - 1]][route[0]];
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
            omega.push_back(full_pack[i]);
        }
    }

    bool is_valid_permutation(const std::vector<int>& route) {
        if ((int)route.size() != numCities) return false;
        std::vector<int> cnt(numCities, 0);
        for (int v : route) {
            if (v < 0 || v >= numCities) return false;
            cnt[v]++;
            if (cnt[v] > 1) return false;
        }
        return true;
    }

    std::vector<int> repair_permutation(const std::vector<int>& route) {
        std::vector<int> res = route;
        std::vector<int> cnt(numCities, 0);
        for (int& v : res) {
            if (v < 0 || v >= numCities) v = -1;
            else cnt[v]++;
        }
        std::vector<int> missing;
        for (int i = 0; i < numCities; ++i) if (cnt[i] == 0) missing.push_back(i);
        int m = 0;
        std::vector<int> seen(numCities, 0);
        for (int& v : res) {
            if (v == -1) {
                v = missing[m++];
            } else {
                if (seen[v] == 1) {
                    v = missing[m++];
                } else {
                    seen[v] = 1;
                }
            }
        }
        return res;
    }

    std::vector<int> move_towards(const std::vector<int>& current_route,
                                  const std::vector<int>& target_route) {
        int n = numCities;
        std::vector<int> child(n, -1);
        int start = city_dist(rand_gen);
        int end = city_dist(rand_gen);
        if (start > end) std::swap(start, end);
        for (int i = start; i <= end; ++i) child[i] = target_route[i];
        int pos = (end + 1) % n;
        for (int i = 0; i < n; ++i) {
            int city = current_route[(end + 1 + i) % n];
            if (std::find(child.begin(), child.end(), city) == child.end()) {
                child[pos] = city;
                pos = (pos + 1) % n;
            }
        }
        if (!is_valid_permutation(child)) child = repair_permutation(child);
        return child;
    }

    std::vector<int> swap_mutation(const std::vector<int>& route) {
        std::vector<int> r = route;
        int i = city_dist(rand_gen);
        int j = city_dist(rand_gen);
        while (i == j) j = city_dist(rand_gen);
        std::swap(r[i], r[j]);
        return r;
    }

    void update_omega_wolf(Wolf& wolf, double a) {
        std::vector<int> new_route;
        double A1 = a * (2.0 * unif_dist(rand_gen) - 1.0);
        double r = unif_dist(rand_gen);
        if (r < 0.33) new_route = move_towards(wolf.getRoute(), alfa.getRoute());
        else if (r < 0.66) new_route = move_towards(wolf.getRoute(), beta.getRoute());
        else new_route = move_towards(wolf.getRoute(), delta.getRoute());
        if (std::abs(A1) > 1.0) new_route = swap_mutation(new_route);
        if (!is_valid_permutation(new_route)) new_route = repair_permutation(new_route);
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
        for (size_t i = 3; i < full_pack.size(); ++i) {
            omega.push_back(full_pack[i]);
        }
    }

    // -----------------------------------------------------------------
    // 💎 NOWA FUNKCJA: LOKALNY OPTYMALIZATOR 2-OPT 💎
    // -----------------------------------------------------------------
    /**
     * @brief Stosuje algorytm 2-opt do danej trasy, aby usunąć skrzyżowania.
     * Modyfikuje trasę w miejscu (przez referencję) i zwraca nowy, ulepszony koszt.
     */
    double apply_2_opt(std::vector<int>& route) {
        bool improvement = true;
        double best_fitness = calculate_route_length(route);

        while (improvement) {
            improvement = false;
            for (int i = 0; i < numCities - 1; ++i) {
                for (int k = i + 1; k < numCities; ++k) {
                    // Rozważamy zamianę krawędzi (i, i+1) oraz (k, k+1)
                    // na (i, k) oraz (i+1, k+1)
                    // (z obsługą zawijania dla krawędzi (k, k+1))

                    // Krawędź 1: (a) -> (b)
                    int a = route[i];
                    int b = route[i + 1]; // Dla i=n-1 to nie będzie używane

                    // Krawędź 2: (c) -> (d)
                    int c = route[k];
                    int d = route[(k + 1) % numCities]; // Obsługa zawijania z końca do początku

                    // Koszt przed zamianą: dist(a,b) + dist(c,d)
                    double current_cost = distance_matrix[a][b] + distance_matrix[c][d];
                    // Koszt po zamianie: dist(a,c) + dist(b,d)
                    double new_cost = distance_matrix[a][c] + distance_matrix[b][d];

                    // Używamy małej tolerancji, aby uniknąć problemów z precyzją
                    if (new_cost < current_cost - 1e-9) {
                        // Znaleziono poprawę.
                        // Odwracamy segment trasy od [i+1] do [k]
                        std::reverse(route.begin() + i + 1, route.begin() + k + 1);
                        
                        best_fitness = calculate_route_length(route); // Oblicz nowy fitness
                        improvement = true;
                        
                        // Strategia "First Improvement": przerywamy pętle i zaczynamy od nowa
                        goto restart_2_opt_loops;
                    }
                }
            }
        restart_2_opt_loops:; // Etykieta dla "goto"
        }
        return best_fitness;
    }


public:
    // -----------------------------------------------------------------
    // 🛠️ MODYFIKACJA KONSTRUKTORA 🛠️
    // -----------------------------------------------------------------
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
        
        // !!! USUNIĘTO: bestRoute = this->solve(); !!!
        // Konstruktor nie powinien uruchamiać algorytmu.
        // Należy to zrobić ręcznie po utworzeniu obiektu.
    }

    // -----------------------------------------------------------------
    // 🚀 MODYFIKACJA METODY SOLVE 🚀
    // -----------------------------------------------------------------
    std::vector<Point> solve() {
        std::cout << "Rozpoczynanie algorytmu GWO..." << std::endl;
        for (int t = 0; t < maxIterations; ++t) {
            double progress = static_cast<double>(t) / maxIterations;
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

        // --- SEKCJA HYBRYDOWA: 2-OPT ---
        std::cout << "Uruchamianie lokalnej optymalizacji 2-opt..." << std::endl;
        std::vector<int> bestRouteIndices = alfa.getRoute();
        double finalLength = apply_2_opt(bestRouteIndices);

        if (finalLength < bestLength) {
            std::cout << "2-opt znalazl lepsze rozwiazanie! Ostateczna dlugosc: " << finalLength << std::endl;
            bestLength = finalLength;
            alfa.setRoute(bestRouteIndices);
            alfa.setFitness(finalLength);
        } else {
            std::cout << "2-opt nie znalazl poprawy. Wynik GWO jest ostateczny." << std::endl;
        }
        // --- Koniec sekcji 2-opt ---


        std::vector<Point> bestRoutePoints;
        for (int city_index : alfa.getRoute()) { // Używamy zaktualizowanej trasy alfa
            bestRoutePoints.push_back(cities[city_index]);
        }
        bestRoute = bestRoutePoints; // Zapisujemy ostateczną trasę
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