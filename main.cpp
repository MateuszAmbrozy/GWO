#include <iostream>
#include <ctime>     // Potrzebne do std::srand
#include <cstdlib>   // Potrzebne do std::srand i std::rand
#include "TSP.h"     // Zakładając, że plik TSP.h jest w katalogu 'includes'
#include "FileHandler.h"

// --- Stałe dla problemu GWO ---
// Możesz je zmieniać, aby testować algorytm
const int LICZBA_MIAST = 200;
const int POPULACJA_WILKOW = 100;
const int LICZBA_ITERACJI = 1000;

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::cout << "Rozpoczynam rozwiazywanie TSP dla " << LICZBA_MIAST << " miast." << std::endl;
    std::cout << "Parametry GWO: Populacja = " << POPULACJA_WILKOW 
              << ", Iteracje = " << LICZBA_ITERACJI << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    TSP problem_tsp(LICZBA_MIAST, POPULACJA_WILKOW, LICZBA_ITERACJI);

    problem_tsp.solve();
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Algorytm GWO zakonczony." << std::endl;

    problem_tsp.print_solution();

    save_best_route_to_file(problem_tsp, "best_route.txt");

    return 0;
}