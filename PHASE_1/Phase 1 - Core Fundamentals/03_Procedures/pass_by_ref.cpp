/*
 * pass_by_ref.cpp
 * ---------------
 * Parameter passing in C++:
 *   - by value     (the function receives a copy);
 *   - by reference with & (the function modifies the original);
 *   - by constant reference const& (no copy, read-only);
 *   - std::vector as a parameter.
 *
 * Build: g++ -Wall -Wextra -std=c++17 pass_by_ref.cpp -o pass_by_ref
 * Run:   ./pass_by_ref
 */

#include <iostream>
#include <vector>

// By value: modifies only the local copy, NOT the caller's variable.
void by_value(int x)
{
    x = 99;
    std::cout << "  [inside by_value] the local copy is now " << x << "\n";
}

// By reference: r is an alias of the variable passed, which gets modified.
void doubleValue(int& r)
{
    r = r * 2;
}

// Returning multiple values using references as output parameters:
// computes the minimum and maximum of a vector at the same time.
// The vector is passed by const& (read-only, no copy).
void min_max(const std::vector<int>& v, int& out_min, int& out_max)
{
    out_min = out_max = v[0];
    for (int x : v) {
        if (x < out_min) out_min = x;
        if (x > out_max) out_max = x;
    }
}

// Vector passed by NON-constant reference: modified in place.
void double_vector(std::vector<double>& v)
{
    for (double& x : v) {
        x *= 2.0;
    }
}

int main()
{
    // --- by value vs by reference ---
    int a = 21;
    by_value(a);
    std::cout << "After by_value(a):    a = " << a << " (unchanged)\n";

    doubleValue(a);
    std::cout << "After doubleValue(a): a = " << a << " (modified)\n";

    // --- multiple output values via references ---
    std::vector<int> data = {7, 3, 9, 1, 8, 5};
    int mn, mx;
    min_max(data, mn, mx);
    std::cout << "\nVector: ";
    for (int x : data) std::cout << x << " ";
    std::cout << "\nMinimum = " << mn << ", Maximum = " << mx << "\n";

    // --- vector modified in place ---
    std::vector<double> v = {1.0, 2.5, 3.0};
    double_vector(v);
    std::cout << "\nVector after double_vector: ";
    for (double x : v) std::cout << x << " ";
    std::cout << "\n";

    return 0;
}
