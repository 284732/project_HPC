/*
 * functions.cpp
 * -------------
 * Functions in C++: definition and prototypes, return value, void functions,
 * overloading and default arguments.
 *
 * Build: g++ -Wall -Wextra -std=c++17 functions.cpp -o functions
 * Run:   ./functions
 */

#include <iostream>
#include <string>

// --- Prototypes (declarations) ---
double circle_area(double radius);
int    maximum(int a, int b);
void   print_header(const std::string& title);   // void function

// Overloading: same name, parameters of different type
int    square(int x);
double square(double x);

// Default argument: exp is 2 if the caller omits it
double power(double base, int exp = 2);

int main()
{
    print_header("Function examples");

    double r = 2.0;
    std::cout << "Area of a circle of radius " << r << " = " << circle_area(r) << "\n";
    std::cout << "Maximum of 17 and 42 = " << maximum(17, 42) << "\n";

    // Overloading: the compiler picks the right version based on the type
    std::cout << "square(5)   = " << square(5) << "  (int version)\n";
    std::cout << "square(2.5) = " << square(2.5) << "  (double version)\n";

    // Default arguments
    std::cout << "power(3.0)     = " << power(3.0) << "  (default exp = 2)\n";
    std::cout << "power(2.0, 10) = " << power(2.0, 10) << "\n";

    return 0;
}

// --- Definitions ---

double circle_area(double radius)
{
    const double PI = 3.14159265358979;
    return PI * radius * radius;
}

int maximum(int a, int b)
{
    return (a > b) ? a : b;
}

void print_header(const std::string& title)
{
    std::cout << "=== " << title << " ===\n";
}

int    square(int x)    { return x * x; }
double square(double x) { return x * x; }

double power(double base, int exp)
{
    double r = 1.0;
    for (int i = 0; i < exp; ++i) {
        r *= base;
    }
    return r;
}
