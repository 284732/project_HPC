/*
 * loops.cpp
 * ---------
 * Iterative constructs in C++: for, range-based for, while, do-while.
 *
 * Build: g++ -Wall -Wextra -std=c++17 loops.cpp -o loops
 * Run:   ./loops
 */

#include <iostream>
#include <iomanip>
#include <vector>

int main()
{
    // --- for loop: sum of the first N integers (number of iterations known) ---
    int N = 10;
    long sum = 0;
    for (int i = 1; i <= N; ++i) {
        sum += i;
    }
    std::cout << "Sum of the first " << N << " integers (for)   = " << sum << "\n";

    // --- Range-based for (C++11): iterates a container without indices ---
    std::vector<int> data = {2, 4, 6, 8};
    long sum_v = 0;
    for (int x : data) {        // x is a copy of each element
        sum_v += x;
    }
    std::cout << "Sum of the vector (range-based for) = " << sum_v << "\n";

    // With a reference the elements can be modified in place
    for (int& x : data) {
        x *= 10;
    }
    std::cout << "Vector after *10: ";
    for (int x : data) std::cout << x << " ";
    std::cout << "\n";

    // --- while loop: the condition is checked BEFORE each iteration ---
    int n = 5;
    long factorial = 1;
    int k = n;
    while (k > 1) {
        factorial *= k;
        --k;
    }
    std::cout << "\nFactorial of " << n << " (while) = " << factorial << "\n";

    // --- do-while loop: the body is executed AT LEAST once ---
    double value = 100.0;
    int halvings = 0;
    do {
        value /= 2.0;
        ++halvings;
    } while (value >= 1.0);
    std::cout << "Halvings of 100 down to <1 (do-while) = " << halvings << "\n";

    // --- nested for: a small times table (typical use on matrices) ---
    std::cout << "\nMultiplication table 1..4:\n";
    for (int r = 1; r <= 4; ++r) {
        for (int c = 1; c <= 4; ++c) {
            std::cout << std::setw(4) << r * c;
        }
        std::cout << "\n";
    }

    return 0;
}
