#include <iostream>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include "TSP.h"
#include "FileHandler.h"

const int LICZBA_MIAST = 20;
const int POPULACJA_WILKOW = 500000;
const int LICZBA_ITERACJI = 100;//100000;

int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    //auto start_time = std::chrono::high_resolution_clock::now();

    TSP problem_tsp(LICZBA_MIAST, POPULACJA_WILKOW, LICZBA_ITERACJI);

    problem_tsp.solve();

    auto end_time = std::chrono::high_resolution_clock::now();

    //std::chrono::duration<double> duration = end_time - start_time;
    //std::cout << "\nCzas wykonania: " << duration.count() << " sekundy\n"
    //    << std::endl;

    problem_tsp.print_solution();

    save_best_route_to_file(problem_tsp, "best_route.txt");

    return 0;
}