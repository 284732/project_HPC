/*
 * mathutils.cpp
 * -------------
 * Implementation of the functions declared in mathutils.hpp.
 * This file is compiled separately and then linked together with the file that
 * uses it (main_modular.cpp). See the Makefile rule for "main_modular".
 */

#include "mathutils.hpp"

namespace mathutils {

double circle_area(double radius)
{
    const double PI = 3.14159265358979;
    return PI * radius * radius;
}

long factorial(int n)
{
    long r = 1;
    for (int i = 2; i <= n; ++i) {
        r *= i;
    }
    return r;
}

bool is_prime(int n)
{
    if (n < 2) {
        return false;
    }
    for (int d = 2; (long)d * d <= n; ++d) {
        if (n % d == 0) {
            return false;
        }
    }
    return true;
}

}  // namespace mathutils
