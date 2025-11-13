#include <iostream>
#include <ctime>   
#include <cstdlib> 
#include "TSP.h"   
#include "FileHandler.h"

const int LICZBA_MIAST = 200;
const int POPULACJA_WILKOW = 100;
const int LICZBA_ITERACJI = 100000;

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    TSP problem_tsp(LICZBA_MIAST, POPULACJA_WILKOW, LICZBA_ITERACJI);

    problem_tsp.solve();

    problem_tsp.print_solution();

    save_best_route_to_file(problem_tsp, "best_route.txt");

    return 0;
}