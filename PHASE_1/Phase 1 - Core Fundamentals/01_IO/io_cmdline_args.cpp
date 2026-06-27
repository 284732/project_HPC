/*
 * io_cmdline_args.cpp
 * -------------------
 * Command-line arguments as a form of input: argc and argv.
 * This is the typical way an HPC program receives parameters (problem size,
 * number of steps, input file name) when launched from a script or a job
 * scheduler, without interactive prompts.
 *
 * Build: g++ -Wall -Wextra -std=c++17 io_cmdline_args.cpp -o io_cmdline_args
 * Run:   ./io_cmdline_args              (uses the default values)
 *        ./io_cmdline_args 8 1.5        (n = 8, factor = 1.5)
 */

#include <iostream>
#include <string>
#include <cstdlib>   // std::atoi, std::atof

int main(int argc, char* argv[])
{
    // argv[0] is the program name; the real arguments start at argv[1].
    std::cout << "Program name: " << argv[0] << "\n";
    std::cout << "Number of arguments (besides the name): " << argc - 1 << "\n";

    // Default values, overridden only if the user provides arguments.
    int    n      = 5;
    double factor = 2.0;

    if (argc >= 2) {
        n = std::atoi(argv[1]);      // first argument -> n
    }
    if (argc >= 3) {
        factor = std::atof(argv[2]); // second argument -> factor
    }

    if (n <= 0) {
        std::cerr << "Error: n must be positive (received " << n << ").\n";
        return 1;
    }

    std::cout << "Using n = " << n << ", factor = " << factor << "\n";
    for (int i = 1; i <= n; ++i) {
        std::cout << "  " << i << " * factor = " << i * factor << "\n";
    }

    return 0;
}
