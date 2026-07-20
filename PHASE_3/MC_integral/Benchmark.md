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

## Compile & run
For the compilation of this code is possible to use the `gcc` compiler with command `g++` for C++ programming language.
The commands below are necessary for creating the executable file and the execution of the program.
```bash
# CREATION OF THE EXECUTABLE.
g++ -o mc MC.cpp
# RUN THE EXECUTABLE.
./mc
```

## Input & Output
The program is executed with the following input data inserted in `input.txt`.
```
number_of_iteration: 100000000
lower_bound: 0.0
upper_bound: 1.0
```
`n_of_points` is declared as `int64_t` in order to be able to read a large integer value because Montecarlo method reduce the error with high number of samples.

The output of the execution is writed on file `output.txt` with the execution time.
The output data are the following:
```
Integral estimation : 3.14169
Computational time : 26114 ms.
```

---

# `MC_MPI.cpp` — Monte Carlo Integration (parallelized with MPI)

## Overview
This program estimates the same integral as `MC.cpp` with the **Monte Carlo method**, but distributes the generation of random points across multiple processes using **MPI**. The statistical approach is unchanged; only the workload is split and then recombined.

## What it computes
Exactly as in the sequential version, at each random point $x$ the program evaluates:

<div align="center">

$$
f(x) = \frac{4}{1 + x^2}
$$

</div>

Each process computes a **local** average over its own share of random points, and the global estimate is obtained by summing all local contributions and applying the same **Average value theorem**:

<div align="center">

$$
\int_{a}^{b} f(x)\,dx \approx (b-a) \cdot \frac{1}{n}\sum_{i=1}^{n} f(x_i)
$$

</div>

Since $f(x) = \dfrac{4}{1+x^2}$, integrating over $[0, 1]$ gives $\pi$, so this program is a parallel Monte Carlo estimator of π.

## How it works

First of all is necessary to initialize the **MPI environment**:

```cpp
#include <mpi.h>

MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &size);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

1. **Reads input parameters (rank 0 only)** from `input_MC_MPI.txt`:
   - Number of random sample points (`n_of_points`), declared as `int64_t`.
   - Lower bound `bounds[0]` and upper bound `bounds[1]` of the integration interval.

```cpp
// Process 0 reads the input file and broadcasts the values to all processes.
if (rank == 0) {
    ifstream in_file("input_MC_MPI.txt");
    in_file >> label >> n_of_points;
    in_file >> label >> bounds[0];
    in_file >> label >> bounds[1];
    in_file.close();
}
```

2. **Broadcast**: `n_of_points` and `bounds` are sent from rank 0 to every other process with `MPI_Bcast`, so all processes share the same input data. This happens **before** the validity check below, so that every process can evaluate it consistently, not only rank 0.

```cpp
MPI_Bcast(&n_of_points, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
MPI_Bcast(bounds, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
```

3. **Validity check (all processes)**: if `n_of_points` is lower than the number of processes, every rank prints the error (rank 0 only), then **all** ranks call `MPI_Finalize` and return. Terminating on every rank, rather than only rank 0, avoids leaving the other processes stuck waiting on a collective that will never be called.

```cpp
if (n_of_points < size) {
    if (rank == 0) {
        cout << "ERROR : Impossible to solve with a number of iterations lower than the number of tasks!\n";
    }
    MPI_Finalize();
    return 0;
}
```

4. **Workload distribution**: each process computes `local_n_of_points`, its own share of random points to generate. The total is divided evenly (`n_of_points / size`), and the remainder is distributed one extra point at a time to the first ranks, so no points are lost to integer division.

```cpp
local_n_of_points = int(n_of_points / size);
remainder = n_of_points % size;
for (int i = 0; i < remainder; i++) {
    if (rank == i) {
        local_n_of_points++;
    }
}
```

5. **Generates random numbers (per process)**: each process creates its **own** `mt19937` generator, independently seeded via `random_device`, uniformly distributed over `[bounds[0], bounds[1]]`. This ensures each process draws a different, uncorrelated sequence of random points.

```cpp
random_device rd; // Random seed.
mt19937 generator(rd()); // Random number generator.
uniform_real_distribution <double> distribution(bounds[0], bounds[1]);
```

6. **Timing**: unlike `MC.cpp` (which uses `std::chrono`), here the elapsed time is measured with `MPI_Wtime()`, called right before the sampling loop and right after the reduction.

```cpp
double start_time = MPI_Wtime();
// ...
double end_time = MPI_Wtime();
```

7. **Main loop (local)**: for each of its `local_n_of_points` iterations, the process draws a random `x`, evaluates `f(x) = 4 / (1 + x²)`, and accumulates the result in its own `local_sum`.

```cpp
for (int i = 0; i < local_n_of_points; i++) {
    // Each process executes the same cycle, but with different random values.
    x_random = distribution(generator);
    f_x = 4.0 / (1.0 + (x_random * x_random));
    local_sum += f_x;
}
```

8. **Reduction**: `MPI_Reduce` with `MPI_SUM` collects all the `local_sum` values and sums them into `total_sum` on rank 0. Being a blocking collective, it also synchronizes all processes before rank 0 proceeds.

```cpp
MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
```

9. **Writes the result (rank 0 only)** to `output_MC_MPI.txt`: the estimated integral value — computed as `(bounds[1] - bounds[0]) * (total_sum / n_of_points)`, exactly the same formula as the sequential version but applied to the globally-reduced sum — and the computational time in milliseconds.

```cpp
if (rank == 0) {
    ofstream out_file("output_MC_MPI.txt");
    out_file << "Integral estimation : " << ((bounds[1] - bounds[0]) * (total_sum / n_of_points)) << "\n";
    out_file << "Computational time : " << (end_time - start_time) * 1000 << " ms.";
    out_file.close();
}
```

Once the procedure is finished, is necessary to **close the MPI environment**:

```cpp
MPI_Finalize();
return 0;
```

## Compile & run
For the compilation of this code is necessary to use the MPI wrapper compiler `mpic++`.
The commands below are necessary for creating the executable file and the execution of the program with MPI (2 tasks).
```bash
# CREATION OF THE EXECUTABLE.
mpic++ -o mc_mpi MC_MPI.cpp
# RUN THE EXECUTABLE.
mpirun -n 2 ./mc_mpi
```

## Input & Output
The program is executed with the following input data inserted in `input_MC_MPI.txt`.
```
number_of_points: 100000000
lower_bound: 0.0
upper_bound: 1.0
```
As in the sequential version, `n_of_points` is declared as `int64_t` to allow reading a large number of samples, since the Monte Carlo method reduces its error as the sample size grows.

The output of the execution is writed on file `output_MC_MPI.txt` with the execution time.
The output data are the following (example, run with 2 processes):
```
Integral estimation : 3.14169
Computational time : 26114 ms.
```
## Notes
- Being purely serial, this implementation is the natural **baseline for execution time** when comparing against a parallel (MPI) version of the same integral estimation.
- Being a stochastic method, accuracy depends on the number of sample points `n_of_points`: increasing it reduces the statistical error but increases runtime.
- Is chosen this kind of structure in order to allow to estimate the integral over other intervals by changing the value of `lower_bound` and `upper_bound` in the input file `input.txt`.

