/*
 * scope_static.cpp
 * ----------------
 * Variable scope and storage classes in C++.
 *   - global vs local variable;
 *   - static local variable (preserves its value between calls);
 *   - const / constexpr for constants.
 *
 * Build: g++ -Wall -Wextra -std=c++17 scope_static.cpp -o scope_static
 * Run:   ./scope_static
 */

#include <iostream>
#include <iomanip>

// Global variable: visible to all functions in this file.
int global_counter = 0;

// Uses a static local variable: initialized only once, it keeps its value
// between calls.
int calls()
{
    static int n = 0;   // NOT reset on every call
    ++n;
    return n;
}

void increment_global()
{
    ++global_counter;             // modifies the global variable
    int local = 100;              // local variable: exists only here
    ++local;
    (void)local;                  // avoids the "set but not used" warning
}

int main()
{
    // constexpr: constant computed at compile time
    constexpr double PI = 3.14159265358979;
    std::cout << "Constant PI = " << std::fixed << std::setprecision(5) << PI << "\n\n";

    // The static inside calls() remembers how many times it has been invoked.
    std::cout << "Consecutive calls to the function (static variable):\n";
    for (int i = 0; i < 5; ++i) {
        std::cout << "  call number " << calls() << "\n";
    }

    // The global is shared across the calls of different functions.
    for (int i = 0; i < 3; ++i) {
        increment_global();
    }
    std::cout << "\nglobal_counter after 3 increments = " << global_counter << "\n";

    return 0;
}
