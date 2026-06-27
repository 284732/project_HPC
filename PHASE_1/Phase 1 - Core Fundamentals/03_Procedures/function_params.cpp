/*
 * function_params.cpp
 * -------------------
 * Passing functions as parameters: function pointers, lambdas and std::function.
 * This is a powerful abstraction: a generic routine (here, numerical integration
 * with the trapezoidal rule) works on ANY function provided by the caller.
 *
 * Build: g++ -Wall -Wextra -std=c++17 function_params.cpp -o function_params
 * Run:   ./function_params
 */

#include <iostream>
#include <iomanip>
#include <functional>   // std::function
#include <cmath>        // std::sin

const double PI = 3.14159265358979;

// Version taking a plain function pointer: double (*)(double).
double integrate(double (*f)(double), double a, double b, int n)
{
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    for (int i = 1; i < n; ++i) {
        sum += f(a + i * h);
    }
    return sum * h;
}

// Version taking a std::function: also accepts lambdas (even with captures).
double integrate(const std::function<double(double)>& f, double a, double b, int n)
{
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    for (int i = 1; i < n; ++i) {
        sum += f(a + i * h);
    }
    return sum * h;
}

// An ordinary function that can be passed by pointer.
double square(double x)
{
    return x * x;
}

int main()
{
    std::cout << std::fixed << std::setprecision(6);

    // 1) Pass an ordinary function by pointer.
    double i1 = integrate(square, 0.0, 1.0, 1000);
    std::cout << "Integral of x^2 on [0, 1]  = " << i1 << "  (exact 0.333333)\n";

    // 2) Pass a lambda through std::function.
    std::function<double(double)> g = [](double x) { return std::sin(x); };
    double i2 = integrate(g, 0.0, PI, 1000);
    std::cout << "Integral of sin on [0, pi] = " << i2 << "  (exact 2.000000)\n";

    // 3) A lambda capturing a parameter: f(x) = c * x.
    double c = 3.0;
    std::function<double(double)> line = [c](double x) { return c * x; };
    double i3 = integrate(line, 0.0, 2.0, 1000);
    std::cout << "Integral of 3x on [0, 2]   = " << i3 << "  (exact 6.000000)\n";

    return 0;
}
