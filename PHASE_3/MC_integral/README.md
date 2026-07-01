# `MC.cpp` — Monte Carlo Integration (sequential)

## Overview
This program estimates a definite integral using the **Monte Carlo method**, without any parallelization. It serves as the sequential reference implementation for comparison against MPI-parallelized versions of the same problem.

## What it computes
At each random point $x$, the program evaluates the function:

<div align="center">

$$
f(x) = \frac{4}{1 + x^2}
$$

</div>

It then averages all the computed values and multiplies the result by the interval width $(b - a)$ to estimate the integral with the **Average value theorem**:

<div align="center">

$$
\int_{a}^{b} f(x)\,dx \approx (b-a) \cdot \frac{1}{n}\sum_{i=1}^{n} f(x_i)
$$

</div>

Since $f(x) = \dfrac{4}{1+x^2}$, integrating over $[0, 1]$ gives $\pi$ — so this program is effectively a Monte Carlo estimator of π.

## How it works

1. **Reads input parameters** from `input.txt`:
   - Number of random sample points (`n_of_points`) and declare it as `int64_t`.
   - Lower bound `a` and upper bound `b` of the integration interval
  
```cpp
#include <fstream>

ifstream in_file("input.txt");
in_file >> label >> n_of_points;
in_file >> label >> a;
in_file >> label >> b;
in_file.close();
```

2. **Generates random numbers** using `mt19937` seeded with `random_device` and uniformly distributed over `[a, b]`.

```cpp
random_device rd; // Random seed.
mt19937 generator(rd()); // Random number generator.
uniform_real_distribution <double> distribution(a, b);
```

4. **Main loop**: for each of the `n_of_points` iterations, it draws a random `x`, evaluates `f(x) = 4 / (1 + x²)`, and accumulates the result in `total_sum`.

```cpp
for (int i = 0; i < n_of_points; i++) {
    
    // Generation of a random value into the interval.
    x_random = distribution(generator);

    // Calculation of the function in x.
    f_x = 4.0 / (1.0 + (x_random*x_random));

    // Add the value of f_x to the total sum.
    total_sum += f_x;
}
```

5. **Timing**: execution time is measured with `std::chrono::high_resolution_clock`, wrapping only the sampling loop (I/O is excluded).

6. **Writes the result** to `output.txt`: the estimated integral value and the computational time in milliseconds.

```cpp
ofstream out_file("output.txt");
out_file << "Integral estimation : " << ((b - a) * (total_sum / n_of_points)) << "\n";
out_file << "Computational time : " << duration.count() << " ms.";
out_file.close();
```
