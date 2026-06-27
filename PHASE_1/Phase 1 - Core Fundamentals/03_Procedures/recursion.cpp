/*
 * recursion.cpp
 * -------------
 * Recursion in C++ and comparison with the iterative version.
 *   - recursive and iterative factorial;
 *   - recursive Fibonacci (clear but inefficient) and iterative.
 *
 * Build: g++ -Wall -Wextra -std=c++17 recursion.cpp -o recursion
 * Run:   ./recursion
 */

#include <iostream>
#include <iomanip>

// Recursive factorial: base case n <= 1, step n * factorial(n-1).
long factorial_rec(int n)
{
    if (n <= 1) {
        return 1;                            // base case
    }
    return static_cast<long>(n) * factorial_rec(n - 1);   // recursive step
}

// The same function in iterative form.
long factorial_it(int n)
{
    long r = 1;
    for (int i = 2; i <= n; ++i) {
        r *= i;
    }
    return r;
}

// "Naive" recursive Fibonacci: elegant but with exponential cost.
long fib_rec(int n)
{
    if (n < 2) {
        return n;
    }
    return fib_rec(n - 1) + fib_rec(n - 2);
}

// Iterative Fibonacci: linear cost.
long fib_it(int n)
{
    long a = 0, b = 1;
    for (int i = 0; i < n; ++i) {
        long t = a + b;
        a = b;
        b = t;
    }
    return a;
}

int main()
{
    int n = 10;

    std::cout << "Factorial of " << n << ": recursive = " << factorial_rec(n)
              << ", iterative = " << factorial_it(n) << "\n";

    std::cout << "\nFirst Fibonacci numbers (recursive vs iterative):\n";
    for (int i = 0; i <= 10; ++i) {
        std::cout << "  F(" << std::setw(2) << i << ") = " << fib_rec(i)
                  << "  (it: " << fib_it(i) << ")\n";
    }

    std::cout << "\nNote: the recursive version of Fibonacci is clear but recomputes\n"
                 "the same values many times; the iterative one is much more efficient.\n";

    return 0;
}
