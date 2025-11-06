#include "TSP.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <ctime>

TSP::TSP(int n, int populationSize = 50, int maxIterations = 1000)
:         this->numCities(n),
          this->populationSize(populationSize),
          this->maxIterations(maxIterations),
          this->bestLength(std::numeric_limits<double>::max()),
          this->rand_gen(std::random_device{}()),
          this->unif_dist(0.0, 1.0),
          this->city_dist(0, n - 1)
{
    if(n <= 0) {
        std::cout << "ERROR::TSP::TSP n <= 0\n";
        return;
    }
    this->generate_random_cities(n);
    this->build_distance_matrix();

}

void TSP::generate_random_cities(int n) {
    if(n <= 0) {
        std::cout << "ERROR::TSP::generate_random_path: n < 0\n";
    }   
    cities.clear();
    for (int i = 0; i < n; i++) {
        int x = (int)rand() / (int)(RAND_MAX / 11);
        int y = (int)rand() / (int)(RAND_MAX / 11);
        this->cities.push_back(Point(x, y));
    }
}

void TSP::build_distance_matrix() {
    size_t length = this->path.size();
    distance_matrix.assign(length, std::vector<double>(length, 0.0));
    for(size_t i = 0; i < length; i++) {
        for(size_t j = 0; j < length; j++) {
            if(i == j) {
                distance_matrix[i][j] = 0;
            } else {
                double d = path[i].dist(path[j]);
                distance_matrix[i][j] = d;
                distance_matrix[j][i] = d;
            }
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
