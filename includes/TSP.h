#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include <stdlib.h>     /* srand, rand */
#include <random>
#include <time.h>       /* time */
#include <algorithm>
#include  <iomanip>

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
    std::uniform_real_distribution<double> unif_dist; // Zakres [0.0, 1.0]
    std::uniform_int_distribution<int> city_dist;     // Zakres [0, numCities-1]

    void generate_random_cities(int n) {
        if (n <= 0) {
            std::cout << "ERROR::TSP::generate_random_path: n < 0\n";
            return;
        }
        cities.clear();
        for (int i = 0; i < n; i++) {
            // Używam Twojej logiki (int 0-10), ale z lepszym generatorem
            int x = rand_gen() % 11; 
            int y = rand_gen() % 11;
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

    // --- Metody Głównej Pętli GWO ---

    /**
     * @brief Operator "ruchu" dla TSP. 
     * Tworzy nową trasę, która jest o JEDEN krok bliżej trasy docelowej.
     * To jest nasza implementacja "odejmowania" permutacji.
     */
    std::vector<int> move_towards(const std::vector<int>& current_route,
                                  const std::vector<int>& target_route) {
        std::vector<int> new_route = current_route;

        // 1. Znajdź indeks, w którym trasy się różnią
        int swap_idx_1 = -1;
        for(int i = 0; i < numCities; ++i) {
            if (new_route[i] != target_route[i]) {
                swap_idx_1 = i;
                break;
            }
        }

        if (swap_idx_1 == -1) return new_route; // Trasy są identyczne

        // 2. Znajdź miasto, które *powinno* być na tym indeksie
        int city_to_find = target_route[swap_idx_1];

        // 3. Znajdź, gdzie to miasto jest *obecnie*
        int swap_idx_2 = -1;
        for (int i = 0; i < numCities; ++i) {
            if (new_route[i] == city_to_find) {
                swap_idx_2 = i;
                break;
            }
        }

        // 4. Zamień je miejscami
        if (swap_idx_2 != -1) {
            std::swap(new_route[swap_idx_1], new_route[swap_idx_2]);
        }
        
        return new_route;
    }

    /**
     * @brief Aktualizuje pozycję (trasę) pojedynczego wilka Omega.
     */
    void update_omega_wolf(Wolf& wolf, double a) {
        std::vector<int> new_route;

        // Parametr A kontroluje eksplorację/eksploatację
        double A1 = a * (2.0 * unif_dist(rand_gen) - 1.0); // A w zakresie [-a, a]
        
        // Naśladujemy formułę GWO: X(t+1) = (X1+X2+X3)/3
        // W TSP, "uśredniamy" wpływ liderów, np. wybierając jednego losowo.
        
        double r = unif_dist(rand_gen);
        if (r < 0.33) {
            new_route = move_towards(wolf.getRoute(), alfa.getRoute());
        } else if (r < 0.66) {
            new_route = move_towards(wolf.getRoute(), beta.getRoute());
        } else {
            new_route = move_towards(wolf.getRoute(), delta.getRoute());
        }

        // Dodajemy element eksploracji GWO
        if (std::abs(A1) > 1.0) {
            // EKSPLORACJA: Zamiast podążać za liderem, wykonaj losową mutację
            // (np. zamień dwa losowe miasta)
            int idx1 = city_dist(rand_gen);
            int idx2 = city_dist(rand_gen);
            while (idx1 == idx2) idx2 = city_dist(rand_gen);
            std::swap(new_route[idx1], new_route[idx2]);
        }
        // Jeśli |A1| <= 1.0, to EKSPLOATACJA (używamy trasy 'move_towards' bez zmian)

        // Selekcja zachłanna: akceptuj nową trasę tylko, jeśli jest lepsza
        double new_fitness = calculate_route_length(new_route);
        if (new_fitness < wolf.getFitness()) {
            wolf.setRoute(new_route);
            wolf.setFitness(new_fitness);
        }
    }

    /**
     * @brief Sprawdza całe stado i aktualizuje liderów Alfa, Beta, Delta.
     */
    void update_leaders() {
        // To jest prosta, ale niezbyt wydajna metoda.
        // Lepsza byłaby kolejka priorytetowa.
        
        // 1. Połącz wszystkich wilków w jedno stado
        std::vector<Wolf> full_pack = omega;
        full_pack.push_back(alfa);
        full_pack.push_back(beta);
        full_pack.push_back(delta);

        // 2. Posortuj
        std::sort(full_pack.begin(), full_pack.end());

        // 3. Przypisz nowych liderów
        alfa = full_pack[0];
        beta = full_pack[1];
        delta = full_pack[2];

        // 4. Odbuduj wektor omega
        omega.clear();
        for (size_t i = 3; i < full_pack.size(); ++i) {
            omega.push_back(full_pack[i]);
        }
    }

public:
    /**
     * @brief Konstruktor klasy TSP.
     * Inicjalizuje problem ORAZ natychmiast go rozwiązuje za pomocą GWO.
     * @param n Liczba miast do wygenerowania.
     * @param popSize Rozmiar stada GWO (np. 50).
     * @param iterations Liczba iteracji GWO (np. 1000).
     */
    TSP(int n, int popSize = 50, int iterations = 1000)
        : numCities(n),
          populationSize(popSize),
          maxIterations(iterations),
          bestLength(std::numeric_limits<double>::max()),
          rand_gen(std::random_device{}()), // Inicjalizacja generatora losowego
          unif_dist(0.0, 1.0),
          city_dist(0, n - 1) 
    {
        if (n <= 0) {
            std::cout << "ERROR::TSP::TSP n <= 0\n";
            return;
        }
        if (popSize <= 3) {
            std::cout << "ERROR::TSP::TSP populacja musi byc > 3\n";
            populationSize = 4; // Wymuszenie minimalnego rozmiaru
        }

        // 1. Przygotuj problem
        this->generate_random_cities(n);
        this->build_distance_matrix();

        // 2. Przygotuj algorytm
        this->initialize_population();

        // 3. ROZWIĄŻ PROBLEM (zgodnie z prośbą, aby konstruktor to robił)
        // W normalnym projekcie ta linia byłaby w publicznej metodzie solve()
        bestRoute = this->solve(); 
    }

    /**
     * @brief Główna pętla algorytmu GWO.
     * Zwraca najlepszą trasę jako wektor Punktów.
     */
    std::vector<Point> solve() {
        for (int t = 0; t < maxIterations; ++t) {
            // 1. Oblicz parametr 'a' (maleje liniowo od 2 do 0)
            double progress = static_cast<double>(t) / maxIterations;
            double a = 2.0 * std::pow(1.0 - progress, 2.0); // kwadratowe tempo zaniku

            // double a = 2.0 - t * (2.0 / maxIterations);


            // 2. Zaktualizuj pozycje wszystkich wilków Omega
            for (Wolf& wolf : omega) {
                update_omega_wolf(wolf, a);
            }

            // 3. Zaktualizuj liderów Alfa, Beta, Delta
            update_leaders();

            // 4. Zapisz najlepszy dotychczasowy wynik
            if (alfa.getFitness() < bestLength) {
                bestLength = alfa.getFitness();
                // Opcjonalnie: Pokaż postęp
                 std::cout << "Iteracja " << t << ": Nowa najlepsza trasa = " << bestLength << std::endl;
            }
        }

        std::cout << "GWO zakonczone. Najlepsza znaleziona dlugosc trasy: " << bestLength << std::endl;
        
        // Zwróć rozwiązanie w formie wektora Point, zgodnie z Twoją definicją
        std::vector<int> bestRouteIndices = alfa.getRoute();
        std::vector<Point> bestRoutePoints;
        for (int city_index : bestRouteIndices) {
            bestRoutePoints.push_back(cities[city_index]);
        }
        bestRoute = bestRoutePoints; 
        return bestRoutePoints;
    }

    /**
     * @brief Wypisuje na konsolę ostateczne rozwiązanie (trasę Alfy).
     */
    void print_solution() {
        std::cout << "Najlepsza znaleziona trasa (Dlugosc: " << bestLength << "):" << std::endl;
        alfa.printRoute(); // Zakładam, że klasa Wolf ma metodę printRoute()
    }

    std::vector<Point> get_best_route() const {
        return bestRoute;
    }

    /**
     * @brief Zwraca długość najlepszej znalezionej trasy.
     */
    double get_best_path_length() const {
        return bestLength;
    }
};