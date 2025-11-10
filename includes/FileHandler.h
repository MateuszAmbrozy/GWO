#include "TSP.h" 
#include <fstream>


void save_best_route_to_file(const TSP& problem, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile) {
        std::cerr << "Blad: nie mozna otworzyc pliku " << filename << " do zapisu." << std::endl;
        return;
    }

    outfile << "# Najlepsza trasa znaleziona przez GWO" << std::endl;
    outfile << "# Format: indeks x y" << std::endl;

    std::vector<Point> bestRoute = problem.get_best_route();
    int index = 0;
    for (const auto& p : bestRoute) {
        outfile << index++ << " " << p.get_x() << " " << p.get_y() << std::endl;
    }
    outfile << "# Dlugosc trasy: " << problem.get_best_path_length() << std::endl;

    outfile.close();
    std::cout << "Trasa zapisana do pliku " << filename << std::endl;
}