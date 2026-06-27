/*
 * io_file_text.cpp
 * ----------------
 * Text file I/O in C++: std::ofstream, std::ifstream, std::getline,
 * std::istringstream. The program creates a text file, fills it with a few rows
 * of data and then reads it back line by line, converting the values to numbers.
 *
 * Build: g++ -Wall -Wextra -std=c++17 io_file_text.cpp -o io_file_text
 * Run:   ./io_file_text
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

int main()
{
    const std::string file_name = "data.txt";

    // --- Writing: ofstream truncates/creates the file ---
    {
        std::ofstream out(file_name);
        if (!out) {
            std::cerr << "Cannot open the file for writing\n";
            return 1;
        }
        out << "temperature pressure\n";   // header line
        out << "300.0 1.0\n";
        out << "400.0 1.5\n";
        out << "500.0 2.0\n";
    } // out is closed automatically here (RAII)
    std::cout << "File '" << file_name << "' created.\n\n";

    // --- Reading line by line with getline ---
    std::ifstream in(file_name);
    if (!in) {
        std::cerr << "Cannot open the file for reading\n";
        return 1;
    }

    std::string line;
    double sum_t = 0.0;
    int count = 0;

    // skip the header line
    if (std::getline(in, line)) {
        std::cout << "Header: " << line << "\n";
    }

    std::cout << "Data read:\n";
    while (std::getline(in, line)) {
        std::istringstream iss(line);   // parse the fields of the line
        double t, p;
        if (iss >> t >> p) {
            std::cout << "  T = " << std::fixed << std::setprecision(1)
                      << std::setw(6) << t
                      << "   P = " << std::setw(4) << p << "\n";
            sum_t += t;
            ++count;
        }
    }

    if (count > 0) {
        std::cout << "\nAverage temperature = " << std::fixed << std::setprecision(2)
                  << sum_t / count << " (over " << count << " values)\n";
    }

    return 0;
}
