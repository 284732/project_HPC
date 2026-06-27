/*
 * pointers.cpp
 * ------------
 * Passing parameters by pointer (the mechanism inherited from C).
 * A pointer holds the address of a variable; through it a function can read and
 * modify the caller's variable. Unlike a reference, a pointer can be null, which
 * is useful for optional output parameters.
 *
 * Build: g++ -Wall -Wextra -std=c++17 pointers.cpp -o pointers
 * Run:   ./pointers
 */

#include <iostream>

// Modifies the pointed-to variable. Checks for nullptr first.
void increment(int* p)
{
    if (p != nullptr) {
        ++(*p);             // *p is the variable pointed to by p
    }
}

// Swaps two variables through their addresses.
void swap_values(int* a, int* b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// Optional output parameter: the remainder is written only if 'rem' is not null.
int divide(int dividend, int divisor, int* rem)
{
    if (rem != nullptr) {
        *rem = dividend % divisor;
    }
    return dividend / divisor;
}

int main()
{
    int x = 10;
    increment(&x);                       // pass the address of x
    std::cout << "After increment(&x): x = " << x << "\n";

    int a = 1, b = 2;
    swap_values(&a, &b);
    std::cout << "After swap_values:   a = " << a << ", b = " << b << "\n";

    // The null pointer is handled safely (no crash, nothing happens).
    int* p = nullptr;
    increment(p);
    std::cout << "increment(nullptr) handled safely\n";

    // Using and ignoring the optional output parameter.
    int r;
    int q = divide(17, 5, &r);
    std::cout << "17 / 5 = " << q << " with remainder " << r << "\n";
    int q2 = divide(20, 4, nullptr);     // remainder not requested
    std::cout << "20 / 4 = " << q2 << " (remainder ignored)\n";

    return 0;
}
