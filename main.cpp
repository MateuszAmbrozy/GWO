#include <iostream>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include "TSP.h"
#include "FileHandler.h"

const int LICZBA_MIAST = 200;
const int POPULACJA_WILKOW = 100;
const int LICZBA_ITERACJI = 10000;//100000;

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        return 1;
    }
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    auto start_time = std::chrono::high_resolution_clock::now();

    TSP problem_tsp(LICZBA_MIAST, POPULACJA_WILKOW, LICZBA_ITERACJI, std::atoi(argv[1]));

    problem_tsp.solve();

    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "\nCzas wykonania: " << duration.count() << " sekundy\n"
        << std::endl;

    problem_tsp.print_solution();

    save_best_route_to_file(problem_tsp, "best_route.txt");

    return 0;
}