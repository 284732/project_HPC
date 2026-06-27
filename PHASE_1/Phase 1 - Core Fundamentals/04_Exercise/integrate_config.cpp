/*
 * integrate_config.cpp
 * --------------------
 * A small example that brings together the three Phase 1 topics:
 *   - I/O:         reads parameters from a configuration file and writes a report;
 *   - control flow: a loop accumulates the trapezoidal sum;
 *   - procedures:   the integrand and the integrator are functions, and the
 *                   integrator receives the function to integrate as a parameter.
 *
 * If the configuration file does not exist, it is created with default values
 * (the same pattern used in the project's Monte Carlo case study).
 *
 * Build: g++ -Wall -Wextra -std=c++17 integrate_config.cpp -o integrate_config
 * Run:   ./integrate_config
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cmath>
#include <iomanip>

// --- procedures ---

// The function to integrate: f(x) = x^2.
double integrand(double x)
{
    return x * x;
}

// Generic trapezoidal integrator: receives the function as a parameter.
double trapezoid(double (*f)(double), double a, double b, int n)
{
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    for (int i = 1; i < n; ++i) {   // control flow: accumulation loop
        sum += f(a + i * h);
    }
    return sum * h;
}

// --- I/O: read the configuration, creating defaults if the file is missing ---
std::map<std::string, std::string> load_or_create_config(const std::string& file_name)
{
    std::map<std::string, std::string> cfg = {
        {"a", "0.0"}, {"b", "1.0"}, {"n", "1000"}
    };

    std::ifstream in(file_name);
    if (!in) {
        std::ofstream out(file_name);
        for (const auto& kv : cfg) {
            out << kv.first << "=" << kv.second << "\n";
        }
        std::cout << "Config '" << file_name
                  << "' not found: created with defaults.\n";
        return cfg;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            cfg[key] = val;
        }
    }
    return cfg;
}

int main()
{
    const std::string config_file = "integration_config.txt";
    const std::string result_file = "integration_result.txt";

    auto cfg = load_or_create_config(config_file);
    double a = std::stod(cfg["a"]);
    double b = std::stod(cfg["b"]);
    int    n = std::stoi(cfg["n"]);

    if (n <= 0) {
        std::cerr << "Error: n must be positive.\n";
        return 1;
    }

    double result = trapezoid(integrand, a, b, n);
    double exact  = (b * b * b - a * a * a) / 3.0;   // exact integral of x^2
    double error  = std::fabs(result - exact);

    std::cout << std::fixed << std::setprecision(8);
    std::cout << "Integral of x^2 on [" << a << ", " << b << "] with n = " << n
              << " -> " << result << "\n";
    std::cout << "Exact value = " << exact << ", absolute error = " << error << "\n";

    // --- I/O: write a small report ---
    std::ofstream out(result_file);
    if (!out) {
        std::cerr << "Cannot write the report file.\n";
        return 1;
    }
    out << "--- Numerical Integration Report ---\n";
    out << "function : x^2\n";
    out << "interval : [" << a << ", " << b << "]\n";
    out << "intervals: " << n << "\n";
    out << std::fixed << std::setprecision(8);
    out << "result   : " << result << "\n";
    out << "exact    : " << exact << "\n";
    out << "abs error: " << error << "\n";

    std::cout << "Report written to '" << result_file << "'.\n";
    return 0;
}
