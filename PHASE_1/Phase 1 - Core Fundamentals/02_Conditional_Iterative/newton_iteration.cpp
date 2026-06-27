/*
 * newton_iteration.cpp
 * --------------------
 * Iterative numerical method controlled by a loop: computing a square root with
 * the Newton-Raphson method. This is a typical pattern in scientific computing:
 * iterate until the result is accurate enough (convergence), with a safety cap
 * on the number of iterations.
 *
 * Build: g++ -Wall -Wextra -std=c++17 newton_iteration.cpp -o newton_iteration
 * Run:   ./newton_iteration
 */

#include <iostream>
#include <iomanip>
#include <cmath>     // std::fabs, std::sqrt (reference value only)

// Computes sqrt(a) iteratively. 'iters' is an output parameter (by reference)
// reporting how many iterations were needed.
double my_sqrt(double a, double tol, int& iters)
{
    iters = 0;
    if (a < 0.0) {
        return -1.0;   // not defined for negative numbers
    }
    if (a == 0.0) {
        return 0.0;
    }

    double x = (a > 1.0) ? a : 1.0;   // initial guess
    const int max_iters = 100;        // safety cap

    // while loop: keep iterating while the error is above the tolerance
    while (std::fabs(x * x - a) > tol) {
        x = 0.5 * (x + a / x);        // Newton step
        ++iters;
        if (iters >= max_iters) {
            break;                    // avoid an infinite loop
        }
    }
    return x;
}

int main()
{
    const double tol = 1e-12;

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Square root by Newton-Raphson (tolerance " << std::scientific
              << tol << std::fixed << "):\n";

    for (double a : {2.0, 9.0, 150.0}) {
        int iters;
        double r = my_sqrt(a, tol, iters);
        std::cout << "  sqrt(" << std::setprecision(1) << a << ") ~ "
                  << std::setprecision(10) << r
                  << "  in " << iters << " iterations"
                  << "  (std::sqrt = " << std::sqrt(a) << ")\n";
    }

    return 0;
}
