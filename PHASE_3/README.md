# PHASE_3 — Parallel Computation of Integrals and Linear Systems with MPI

This folder collects three C++ programs that tackle the same kind of numerical problem — integration and the solution of linear systems — comparing a **sequential (Monte Carlo)** approach with **parallel MPI-based** approaches (numerical quadrature and the Jacobi method). The educational goal is to observe how parallelization via MPI (Message Passing Interface) affects both correctness and computation time compared to a serial solution, while also practicing with collective communication primitives (`MPI_Bcast`, `MPI_Reduce`, `MPI_Allreduce`, `MPI_Allgather`).

## Contents of the folder

| File | Type | Description |
|---|---|---|
| `MC.cpp` | C++ source (sequential) | Estimates the integral of `4/(1+x²)` over `[0,1]` (which converges to π) using the Monte Carlo method, without parallelization. |
| `integral_mpi.cpp` | C++ source (parallel, MPI) | Computes the same integral using a rectangle-based quadrature method, distributing the workload across MPI processes. |
| `Jacobi_linear_system.cpp` | C++ source (parallel, MPI) | Solves a 4×4 linear system using the iterative Jacobi method, assigning one row (one unknown) to each of the 4 required MPI processes. |

## Details of the three programs

### 1. `MC.cpp` — Monte Carlo Integration (sequential)
Reads the number of sample points and the interval bounds from an input file, and generates uniformly distributed random numbers with `mt19937`.

At each random point $x$, the program evaluates the function:

<div align="center">

$$
f(x) = \frac{4}{1 + x^2}
$$

</div>

The two bounds of the interval are read from the `input.txt` file.
At the end, it averages all the computed values and multiplies the result by the interval width $(b-a)$ to estimate the integral:

<div align="center">

$$\int_{a}^{b} f(x) dx \approx (b-a) \cdot \frac{1}{n}\sum_{i=1}^{n} f(x_i)$$

</div>

### 2. `integral_mpi.cpp` — Rectangle Quadrature (parallel, MPI)
Process rank 0 reads the parameters from the input file and broadcasts them to all other processes with `MPI_Bcast`. Each process computes how many "steps" (sub-intervals) it is responsible for — also handling the remainder of the division — and its own partial sum of the integrand function evaluated at equally spaced points. The partial sums are then accumulated on process 0 via `MPI_Reduce`. Computation time is measured between two `MPI_Barrier` calls to ensure all processes are synchronized. The result is written to the output file.

### 3. `Jacobi_linear_system.cpp` — Jacobi Method (parallel, MPI)
Iteratively solves the 4×4 linear system:
```
10x -  y + 2z      = 6
-x + 11y -  z + 3t = 25
2x -  y + 10z -  t = -11
     3y -  z + 8t   = 15
```
The program requires exactly 4 MPI processes, one for each unknown (x, y, z, t). At each iteration, every process computes the new value of its own variable using the values from the other processes at the previous iteration; the maximum difference between old and new value is computed with `MPI_Allreduce` (MAX operation) and used as the stopping criterion (threshold `epsilon = 0.01`); the new values are then shared among all processes with `MPI_Allgather`. The final result and the number of iterations are written to the output file.

## Concluding remarks
Comparing the sequential Monte Carlo run (~26 seconds) with the MPI quadrature run (280 ms) concretely shows the benefit of parallelization on a numerical integration problem, both in terms of speed and accuracy of the result obtained (both converge to the expected value of π ≈ 3.14159). The third program extends the exercise from integral computation to solving linear systems, showcasing a different MPI communication pattern (`Allreduce`/`Allgather` collectives instead of `Bcast`/`Reduce`) typical of component-wise decomposition iterative algorithms.

