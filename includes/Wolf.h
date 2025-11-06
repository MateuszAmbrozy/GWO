#pragma once
#include <vector>   
#include <iostream> 
#include <algorithm>
#include <random>   
#include <limits>

class Wolf {
private:
    std::vector<int> route;
    double fitness;
public:
    Wolf() : fitness(std::numeric_limits<double>::max()) {}
    Wolf(int numCities) : fitness(std::numeric_limits<double>::max()) {
        route.resize(numCities);
        for(int i=0; i<numCities; i++){
            route[i] = i;
        }
        thread_local std::random_device rd;
        thread_local std::mt19937 g(rd());
        
        std::shuffle(route.begin(), route.end(), g);
    }
    //get
    double getFitness() const {
        return fitness;
    }

    const std::vector<int>& getRoute() const {
        return route;
    }

    //set
    void setRoute(const std::vector<int>& newRoute) {
        this->route = newRoute;
    }

    void setFitness(double newFitness) {
        fitness = newFitness;
    }

    void printRoute() const {
        std::cout << "Trasa (Fitness: " << fitness << "): [ ";
        for (int city : route) {
            std::cout << city << " ";
        }
        std::cout << "]" << std::endl;
    }

    bool operator<(const Wolf& other) const {
        return this->fitness < other.fitness;
    }
};