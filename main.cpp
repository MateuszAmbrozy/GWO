#include <iostream>
#include <ctime>     // Potrzebne do std::srand
#include <cstdlib>   // Potrzebne do std::srand i std::rand
#include "TSP.h"     // Zakładając, że plik TSP.h jest w katalogu 'includes'

// --- Stałe dla problemu GWO ---
// Możesz je zmieniać, aby testować algorytm
const int LICZBA_MIAST = 20;
const int POPULACJA_WILKOW = 50;
const int LICZBA_ITERACJI = 1000;

int main() {
    // Inicjalizacja generatora liczb losowych
    // Ważne dla Twojej funkcji 'generate_random_cities', która używa rand()
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::cout << "Rozpoczynam rozwiazywanie TSP dla " << LICZBA_MIAST << " miast." << std::endl;
    std::cout << "Parametry GWO: Populacja = " << POPULACJA_WILKOW 
              << ", Iteracje = " << LICZBA_ITERACJI << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Przetwarzanie..." << std::endl;


    TSP problem_tsp(LICZBA_MIAST, POPULACJA_WILKOW, LICZBA_ITERACJI);


    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Algorytm GWO zakonczony." << std::endl;
    
    problem_tsp.print_solution();

    return 0;
}