/*
 * mathutils.hpp
 * -------------
 * Header file: it declares the INTERFACE of a small library of functions
 * (prototypes only, no bodies). This is the standard way to separate interface
 * from implementation. The include guard prevents the header from being
 * included more than once in the same translation unit.
 *
 * The implementation lives in mathutils.cpp; main_modular.cpp uses these
 * functions by including this header.
 */

#ifndef MATHUTILS_HPP
#define MATHUTILS_HPP

namespace mathutils {

    // Area of a circle of the given radius.
    double circle_area(double radius);

    // Factorial of n (n >= 0).
    long factorial(int n);

    // True if n is a prime number.
    bool is_prime(int n);

}  // namespace mathutils

#endif  // MATHUTILS_HPP
