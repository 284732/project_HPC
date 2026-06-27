/*
 * break_continue.cpp
 * ------------------
 * Fine control of iteration with break and continue.
 *   - continue: skips the current iteration;
 *   - break:    interrupts the loop.
 * Includes a small "parameter sweep": it looks for the value of the step h that
 * brings the error below a threshold, and stops with break.
 *
 * Build: g++ -Wall -Wextra -std=c++17 break_continue.cpp -o break_continue
 * Run:   ./break_continue
 */

#include <iostream>
#include <iomanip>

int main()
{
    // --- continue: sum only the even numbers from 1 to 10 ---
    int sum_even = 0;
    for (int i = 1; i <= 10; ++i) {
        if (i % 2 != 0) {
            continue;          // skip the odd numbers
        }
        sum_even += i;
    }
    std::cout << "Sum of the evens from 1 to 10 (continue) = " << sum_even << "\n";

    // --- break: find the first integer whose square exceeds 50 ---
    int found = -1;
    for (int i = 1; i <= 100; ++i) {
        if (i * i > 50) {
            found = i;
            break;             // exit as soon as found
        }
    }
    std::cout << "First integer with square > 50 (break) = " << found << "\n";

    // --- Parameter sweep: look for the step h that brings the error below threshold ---
    double threshold = 1e-3;
    double h = 1.0;
    int step = 0;
    std::cout << "\nParameter sweep (reducing the step h):\n";
    std::cout << std::fixed;
    while (true) {                 // infinite loop interrupted by break
        double error = h * h;      // error ~ h^2
        std::cout << "  step " << step
                  << ": h = " << std::setprecision(5) << h
                  << ", error = " << std::setprecision(6) << error << "\n";
        if (error < threshold) {
            std::cout << "  -> threshold " << std::scientific << std::setprecision(0)
                      << threshold << std::fixed
                      << " reached with h = " << std::setprecision(5) << h << "\n";
            break;
        }
        h /= 2.0;
        ++step;
    }

    return 0;
}
