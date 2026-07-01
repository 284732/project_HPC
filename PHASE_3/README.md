# PHASE 3 — Parallel Computation of Integrals and Linear Systems with MPI

This folder collects three C++ programs that tackle the same kind of numerical problem — integration and the solution of linear systems — comparing a **sequential (Monte Carlo)** approach with **parallel MPI-based** approaches (numerical quadrature and the Jacobi method). The educational goal is to observe how parallelization via MPI (Message Passing Interface) affects both correctness and computation time compared to a serial solution, while also practicing with collective communication primitives (`MPI_Bcast`, `MPI_Reduce`, `MPI_Allreduce`, `MPI_Allgather`).

## Contents of the folder

| File | Type | Description |
|---|---|---|
| `MC.cpp` | C++ (sequential) | Estimates the integral of `4/(1+x²)` over `[0,1]` (which converges to π) using the Monte Carlo method, without parallelization. |
| `integral_mpi.cpp` | C++ (parallel, MPI) | Computes the same integral using a rectangle-based quadrature method, distributing the workload across MPI processes. |
`jacobi_2d_full.cpp` / `jacobi_2d_full.f90` | C++ and Fortran (parallel, MPI) | Two equivalent implementations of a 2D Jacobi heat diffusion solver using a Cartesian topology with 2D domain decomposition and halo exchange, benchmarked against each other. |

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

### 2. `integral_mpi.cpp` — Integral estimation with MPI
Process rank 0 reads the parameters from the input file and broadcasts them to all other processes with `MPI_Bcast`. Each process computes how many "steps" (sub-intervals) it is responsible for also handling the remainder of the division and its own partial sum of the integrand function evaluated at equally spaced points. 

```
Interval [a, b] divided into sub-intervals (for 4 tasks):

Task:     0   1   2   3   0   1   2   3   0   1   2   3   0   1   2   3 ...
          |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
          ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼
      a ──┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴── b

Task 0 → steps: 0, 4, 8, 12, ...
Task 1 → steps: 1, 5, 9, 13, ...
Task 2 → steps: 2, 6, 10, 14, ...
Task 3 → steps: 3, 7, 11, 15, ...
```

The partial sums are then accumulated on process 0 via `MPI_Reduce`. Computation time is measured between two `MPI_Barrier` calls to ensure all processes are synchronized. The result is written to the output file `output.dat` on the following format:
```
Resolution with 2 processes:
Estimated value = 3.14159
Computational time = 280 ms.
```

### 3. `jacobi_2d_full.cpp` / `jacobi_2d_full.f90` — 2D Jacobi Heat Diffusion Solver (parallel, MPI, C++ vs Fortran)
Solves the 2D Jacobi iteration for steady-state heat diffusion on a 128×128 grid, with the bottom boundary fixed at 1.0 and all other boundaries at 0.0. Unlike the previous programs, this one uses a full **2D domain decomposition**: `MPI_Dims_create` automatically computes the best process grid shape, and `MPI_Cart_create` builds a Cartesian topology so each process owns a rectangular subdomain and knows its four neighbors (`MPI_Cart_shift`) — north, south, east, and west.

Each process maintains its subdomain with a **1-cell ghost layer** on every side. At each iteration, boundary values are exchanged with neighboring processes via `MPI_Sendrecv` (halo exchange): rows are sent as contiguous blocks, while columns — non-contiguous in memory — require a derived MPI datatype (`MPI_Type_vector`). After the exchange, each process updates its interior points with the standard 5-point stencil average and computes its local maximum change; `MPI_Allreduce` (MAX) combines these into a global convergence check against the threshold `EPS = 1e-5`.

The C++ and Fortran versions are functionally identical but implemented natively in each language's idioms (indexing, array layout, array swap strategy) and were benchmarked against each other — see `Benchmark.md`, `run_benchmark.sh`, `analyze.py`, and `benchmark_results.csv`.

#### Benchmark results (4 physical cores, N = 128×128)

| lang | np | time (s) | speedup | efficiency |
|------|----|----------|---------|------------|
| cpp  | 1  | 0.258    | 1.00    | 100.0%     |
| cpp  | 2  | 0.149    | 1.73    | 86.6%      |
| cpp  | 4  | 0.117    | 2.21    | 55.1%      |
| f    | 1  | 0.206    | 1.00    | 100.0%     |
| f    | 2  | 0.108    | 1.91    | 95.4%      |
| f    | 4  | 0.134    | 1.54    | 38.4%      |

Both implementations converge in exactly **8242 iterations** with identical results, confirming numerical equivalence. Fortran is faster at low process counts thanks to its column-major memory layout, which keeps the Jacobi stencil's row-neighbor accesses contiguous — an advantage C++ does not have (row-major layout). At `np = 4`, however, C++ overtakes Fortran: with smaller subdomains (64×64), communication overhead dominates, and C++'s contiguous row-based halo exchange becomes more efficient. Full details and analysis are in `Benchmark.md`.


## Concluding remarks
Comparing the sequential Monte Carlo run (~26 seconds) with the MPI quadrature run (280 ms) concretely shows the benefit of parallelization on a numerical integration problem, both in terms of speed and accuracy of the result obtained (both converge to the expected value of π ≈ 3.14159). The third program extends the exercise from integral computation to solving linear systems, showcasing a different MPI communication pattern (`Allreduce`/`Allgather` collectives instead of `Bcast`/`Reduce`) typical of component-wise decomposition iterative algorithms.

