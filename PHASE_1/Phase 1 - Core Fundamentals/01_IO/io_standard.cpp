/*
 * io_standard.cpp
 * ---------------
 * Standard I/O in C++: std::cout, std::cin, format manipulators and standard streams.
 *
 * Build:  g++ -Wall -Wextra -std=c++17 io_standard.cpp -o io_standard
 * Run:    ./io_standard
 *         echo "5 3.14" | ./io_standard   (input via redirection)
 */

#include <iostream>
#include <iomanip>
#include <string>

int main()
{
    // --- Output with cout and the << operator ---
    int         integer   = 42;
    double      real      = 3.14159265358979;
    char        character = 'C';
    std::string text      = "HPC";

    std::cout << "Output examples with <<:\n";
    std::cout << "  integer       -> " << integer << "\n";
    std::cout << "  real          -> " << real << "\n";

    // Format manipulators (<iomanip>)
    std::cout << "  real .3f      -> " << std::fixed << std::setprecision(3)
              << real << "\n";
    std::cout << "  scientific    -> " << std::scientific << real << "\n";
    std::cout << std::defaultfloat;                 // restore the default format
    std::cout << "  character     -> " << character << "\n";
    std::cout << "  string        -> " << text << "\n";
    std::cout << "  aligned w8    -> |" << std::fixed << std::setprecision(3)
              << std::setw(8) << real << "|\n";
    std::cout << std::defaultfloat;

    // --- Diagnostic message on cerr (separate stream) ---
    std::cerr << "[info] this message goes to cerr\n";

    // --- Input with cin and the >> operator ---
    int    n;
    double x;

    std::cout << "\nEnter an integer and a real number (e.g. 5 3.14): ";
    // Extraction returns the stream, which can be evaluated as a bool:
    // true if both reads succeeded.
    if (!(std::cin >> n >> x)) {
        std::cerr << "Invalid input: an integer and a real number were expected.\n";
        return 1;
    }

    std::cout << "You entered n = " << n << " and x = " << x << "\n";
    std::cout << "Sum = " << n + x << ", product = " << n * x << "\n";

    return 0;
}
