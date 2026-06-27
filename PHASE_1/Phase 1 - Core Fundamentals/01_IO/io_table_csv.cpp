/*
 * io_table_csv.cpp
 * ----------------
 * Tabular (CSV) I/O, a very common format for scientific data.
 * The program writes a CSV file with a header, then reads it back, splitting
 * each row on the ',' delimiter with std::getline, converting the fields with
 * std::stoi/std::stod, and printing an aligned table.
 *
 * Build: g++ -Wall -Wextra -std=c++17 io_table_csv.cpp -o io_table_csv
 * Run:   ./io_table_csv
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

int main()
{
    const std::string file_name = "measurements.csv";

    // --- Write a CSV file (header + rows) ---
    {
        std::ofstream out(file_name);
        if (!out) {
            std::cerr << "Cannot create " << file_name << "\n";
            return 1;
        }
        out << "step,time,energy\n";
        for (int i = 0; i < 5; ++i) {
            double t = i * 0.5;
            double e = 100.0 - i * i * 3.0;
            out << i << "," << t << "," << e << "\n";
        }
    }
    std::cout << "File '" << file_name << "' created.\n\n";

    // --- Read it back, parsing on the ',' delimiter ---
    std::ifstream in(file_name);
    if (!in) {
        std::cerr << "Cannot open " << file_name << "\n";
        return 1;
    }

    std::string line;
    std::getline(in, line);   // skip the header

    // aligned table header
    std::cout << std::left
              << std::setw(6) << "step"
              << std::setw(8) << "time"
              << std::setw(10) << "energy" << "\n";

    double sum_e = 0.0;
    int count = 0;

    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string cell;
        std::vector<std::string> cells;
        while (std::getline(iss, cell, ',')) {   // split on the comma
            cells.push_back(cell);
        }
        if (cells.size() == 3) {
            int    step   = std::stoi(cells[0]);
            double time   = std::stod(cells[1]);
            double energy = std::stod(cells[2]);

            std::cout << std::left << std::fixed << std::setprecision(2)
                      << std::setw(6) << step
                      << std::setw(8) << time
                      << std::setw(10) << energy << "\n";
            sum_e += energy;
            ++count;
        }
    }

    if (count > 0) {
        std::cout << "\nAverage energy = " << std::fixed << std::setprecision(2)
                  << sum_e / count << " (over " << count << " rows)\n";
    }

    return 0;
}
