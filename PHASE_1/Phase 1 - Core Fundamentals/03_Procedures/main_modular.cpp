/*
 * main_modular.cpp
 * ----------------
 * Uses the mathutils library by including its header. It is compiled together
 * with mathutils.cpp (separate compilation + linking), the way real projects
 * are organized: the header provides the interface, the .cpp the implementation.
 *
 * Build: g++ -Wall -Wextra -std=c++17 main_modular.cpp mathutils.cpp -o main_modular
 * Run:   ./main_modular
 */

#include <iostream>
#include "mathutils.hpp"

int main()
{
    std::cout << "Using the mathutils library (header + implementation):\n";
    std::cout << "  circle_area(2.0) = " << mathutils::circle_area(2.0) << "\n";
    std::cout << "  factorial(6)     = " << mathutils::factorial(6) << "\n";

    std::cout << "  primes up to 20  : ";
    for (int i = 2; i <= 20; ++i) {
        if (mathutils::is_prime(i)) {
            std::cout << i << " ";
        }
    }
    std::cout << "\n";

    return 0;
}
