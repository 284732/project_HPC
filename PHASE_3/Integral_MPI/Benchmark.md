# Parallel Integral Estimation with MPI & Monte Carlo

## Overview
This document describes two MPI programs that estimate the same definite integral over `[a, b]` using two different numerical strategies, with the workload distributed across multiple processes:

- **`integral_mpi.cpp`** — deterministic **quadrature** method (interleaved round-robin sampling).
- **`MC_MPI.cpp`** — stochastic **Monte Carlo** method (random sampling).

Input parameters for each program are read from a dedicated input file, and results are written to a dedicated output file.

## What it computes
Both programs numerically approximate:

<div align="center">

$$
4 \cdot \int_{a}^{b} \frac{1}{1 + x^2}dx
$$

</div>

Since the integrand is $\dfrac{4}{1+x^2}$ over $[0, 1]$, the result converges to **π**.

- The **quadrature** version sums, over `n` equally spaced sample points across `[a, b]`, the value of the integrand multiplied by the step size `h`:

$$
\int_{a}^{b} f(x)dx \approx \sum_{i=1}^{n} f(x_i) \cdot h
$$

- The **Monte Carlo** version estimates the integral by averaging the integrand over `n` points drawn **randomly and uniformly** from `[a, b]`, then scaling by the interval amplitude:

$$
\int_{a}^{b} f(x)dx \approx (b-a) \cdot \frac{1}{n}\sum_{i=1}^{n} f(x_i)
$$

---

# 1. Quadrature — `integral_mpi.cpp`

## How it works

First of all is necessary to initialize the **MPI environment** with the following lines of code:
```cpp
#include <mpi.h>

// Initialize the MPI environment.
// I use & because MPI needs to modify the variables, then it needs the memory register, not the stored value into them.
MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &size);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

1. **Input reading (rank 0 only)**: process 0 opens `input.txt` and parses:
   - Number of iterations (sample points)
   - Lower bound (`a`) and upper bound (`b`) of the interval

   It also checks that the number of iterations is not smaller than the number of MPI processes, since each process needs at least one point to compute.

```cpp
// Task 0 reads the data from the input file.
if (rank == 0) {
    ifstream in_file("input.txt");

    // Control if the file is open correctly.
    // ! before means the state 'not'.
    if (!in_file.is_open()) {
        cout << "ERROR : The file is not correctly open!\n";
        return 0;
    }

    getline(in_file, line); // Save into 'line' the current line of the file.
    // Take the position of the ':' and takes the string after (+1).
    // stoi (or stod) converts the substring in an integer (or double).
    iterations = stoi(line.substr(line.find(":") + 1));

    // Control that the number of iterations is higher than the number of tasks.
    if (size > iterations) {
        cout << "ERROR : Impossible to solve with a number of iterations lower than the number of tasks!\n";
        return 0;
    }

    // Reads the 2 bounds of the interval.
    for (int i = 0; i < 2; i++) {
        getline(in_file, line);
        bounds[i] = stod(line.substr(line.find(":") + 1));
    }
    in_file.close();
}    
```

2. **Broadcast**: the number of iterations and the interval bounds are broadcast from rank 0 to all other processes with `MPI_Bcast`, so every process has the full set of parameters.

```cpp
MPI_Bcast(&iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&bounds, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
```

3. **Workload distribution**: each process determines how many sample points (`local_tasks`) it must compute. The total is divided evenly (`iterations / size`), and the remainder is distributed one extra point at a time to the first ranks, so that no points are lost due to integer division.

```cpp
// Find the number of steps that every tasks have to elaborate.
local_tasks = int(iterations / size);
remainder = iterations % size;
// Distribute the remainder to the processes.
for (int i = 0; i < remainder; i++) {
    if (rank == i) {
        local_tasks++;
    }
}
```

4. **Interleaved sampling**: each process computes its own starting point `x_start`, offset according to its `rank`, and advances with a `step_per_task` equal to `step * size`. This means the sample points are **interleaved round-robin** across processes rather than split into contiguous blocks — process 0 takes points 0, size, 2·size, ...; process 1 takes points 1, size+1, 2·size+1, ...; and so on.

```cpp
// Each process calculates its starting point, the step and the step for the next x.
interval_amp = bounds[1] - bounds[0];
x_start = bounds[0] + (interval_amp / double(iterations)) * double(rank);
step = double(interval_amp / iterations);
// Once the task i has completed the calculation for one task, the next start with a distance of step_per_task.
step_per_task = step * double(size);
```

5. **Local computation**: each process accumulates its own `local_sum` by evaluating the integrand (already multiplied by the step `h`) at each of its assigned points, via the helper function `calculate_function`. 

```cpp
// Calculate function code:
double calculate_function(double x, double step) {
    return (double(4) * step) / (double(1) + (x*x));
}

// Each task calculates its local sum.
for (int i = 0; i < local_tasks; i++) {
    local_sum += calculate_function(x_start, step);
    x_start += step_per_task;
}
```

6. **Reduction**: `MPI_Reduce` (with `MPI_SUM`) collects and sums all local partial sums into a single global `estimate` on rank 0. Being a blocking collective, it also implicitly synchronizes all processes.

```cpp
MPI_Reduce(&local_sum, &estimate, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
```

7. **Timing**: execution time is measured between two `MPI_Barrier` calls (ensuring all processes are synchronized before and after) using `std::chrono::high_resolution_clock`.

8. **Output**: only rank 0 writes the final result to `output.dat` — the number of processes used, the estimated integral value, and the computation time in milliseconds.

```cpp
// Process 0 have saved into estimate the value of the integral, now writes the results in the output file.
if (rank == 0) {
    ofstream out_file("output.dat");
    out_file << "Resolution with " << size << " processes:\n";
    out_file << "Estimated value = " << estimate << "\n";
    out_file << "Computational time = " << duration.count() << " ms.";
    out_file.close();
}
```
Once the procedure is finisced is necessary to **close the MPI environment** with the command `MPI_Finalize`.
Also must be present the `return 0` in order to finalize the `void main`.
```cpp
MPI_Finalize();
return 0;
```

## Task distribution scheme

The sample points are assigned to processes in an interleaved (round-robin) fashion rather than in contiguous blocks:

```
Task:     0   1   2   3   0   1   2   3   0   1   2   3   0   1   2   3 ...
Step:     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
          ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼
      a ──┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴── b

Task 0 → steps: 0, 4, 8, 12, ...
Task 1 → steps: 1, 5, 9, 13, ...
Task 2 → steps: 2, 6, 10, 14, ...
Task 3 → steps: 3, 7, 11, 15, ...
```

## Compile & run

```bash
# Create the executable
mpic++ integral_mpi.cpp -o integral_mpi

# Run the executable.
mpirun -n 2 ./integral_mpi
```

## Input / Output

**`input.txt`** (must be present in the working directory):
```
number_of_iteration: 100000000
lower_bound: 0.0
upper_bound: 1.0
```

**`output.dat`** (example, run with 2 processes):
```
Resolution with 2 processes:
Estimated value = 3.14159
Computational time = 280 ms.
```

---

# 2. Monte Carlo — `MC_MPI.cpp`

## How it works

The initialization of the **MPI environment** follows the same pattern as the quadrature program:

```cpp
#include <mpi.h>

MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &size);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

1. **Input reading (rank 0 only)**: process 0 opens `input_MC_MPI.txt` and reads, using the `>>` stream extraction operator (each value preceded by a text label that is discarded into `label`):
   - Number of points to sample (`n_of_points`)
   - Lower bound (`bounds[0]`) and upper bound (`bounds[1]`) of the interval

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

2. **Broadcast**: unlike the quadrature version, here the broadcast happens **before** the check on the number of points, so every process (not just rank 0) can evaluate the error condition and terminate consistently.

```cpp
// Process 0 broadcasts the number of points and bounds to all processes.
MPI_Bcast(&n_of_points, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
MPI_Bcast(bounds, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
```

3. **Validity check (all processes)**: if the number of points is lower than the number of processes, every rank prints (rank 0 only) the error, then all ranks call `MPI_Finalize` and exit. This avoids a deadlock that would occur if only rank 0 attempted to terminate.

```cpp
if (n_of_points < size) {
    if (rank == 0) {
        cout << "ERROR : Impossible to solve with a number of iterations lower than the number of tasks!\n";
    }
    MPI_Finalize();
    return 0;
}
```

4. **Workload distribution**: identical logic to the quadrature program — the total number of points is divided evenly (`n_of_points / size`), and the remainder is assigned one extra point at a time to the first ranks.

```cpp
local_n_of_points = int(n_of_points / size);
remainder = n_of_points % size;
for (int i = 0; i < remainder; i++) {
    if (rank == i) {
        local_n_of_points++;
    }
}
```

5. **Independent random generation**: each process creates its **own** random engine, seeded independently via `random_device`, so that different processes generate different (uncorrelated) sequences of random points, all drawn uniformly from `[a, b]`.

```cpp
random_device rd; // Random seed.
mt19937 generator(rd()); // Random number generator.
uniform_real_distribution <double> distribution(bounds[0], bounds[1]);
```

6. **Timing**: unlike the quadrature program (which uses `std::chrono` and `MPI_Barrier`), here timing is measured directly with `MPI_Wtime()`, called right before the sampling loop and right after the reduction.

```cpp
double start_time = MPI_Wtime();
// ...
double end_time = MPI_Wtime();
```

7. **Local computation**: each process draws `local_n_of_points` random values `x_random` in `[a, b]` and accumulates `f(x) = 4 / (1 + x²)` into its own `local_sum` (note: here the sum is **not** yet multiplied by the step, unlike the quadrature version — the scaling by `(b-a)/n` happens later, only on rank 0).

```cpp
for (int i = 0; i < local_n_of_points; i++) {
    // Each process executes the same cycle, but with different random values.
    x_random = distribution(generator);
    f_x = 4.0 / (1.0 + (x_random * x_random));
    local_sum += f_x;
}
```

8. **Reduction**: `MPI_Reduce` (with `MPI_SUM`) sums all local partial sums into `total_sum` on rank 0, exactly as in the quadrature program.

```cpp
MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
```

9. **Output**: only rank 0 computes the final Monte Carlo estimate — average of `f(x)` scaled by the interval amplitude `(b - a)` — and writes it to `output_MC_MPI.txt` together with the computation time in milliseconds.

```cpp
if (rank == 0) {
    ofstream out_file("output_MC_MPI.txt");
    out_file << "Integral estimation : " << ((bounds[1] - bounds[0]) * (total_sum / n_of_points)) << "\n";
    out_file << "Computational time : " << (end_time - start_time) * 1000 << " ms.";
    out_file.close();
}
```

As always, the MPI environment must be closed before exiting:

```cpp
MPI_Finalize();
return 0;
```

## Compile & run

```bash
# Create the executable
mpic++ MC_MPI.cpp -o MC_MPI

# Run the executable.
mpirun -n 2 ./MC_MPI
```

## Input / Output

**`input_MC_MPI.txt`** (must be present in the working directory, whitespace-separated `label value` pairs):
```
number_of_points: 100000000
lower_bound: 0.0
upper_bound: 1.0
```

**`output_MC_MPI.txt`** (example, run with 2 processes):
```
Integral estimation : 3.14169
Computational time : 26114 ms.
```

---

# 3. Comparison: Quadrature vs Monte Carlo

## Key implementation differences

| Aspect | Quadrature (`integral_mpi.cpp`) | Monte Carlo (`MC_MPI.cpp`) |
|---|---|---|
| Sampling | Deterministic, interleaved round-robin | Random, uniform per process |
| Random engine | Not used | `mt19937` seeded via `random_device`, one instance per process |
| Broadcast vs. validity check order | Check happens on rank 0 before broadcast | Broadcast happens before the check, so all ranks can exit consistently |
| Timing method | `std::chrono::high_resolution_clock` + `MPI_Barrier` | `MPI_Wtime()` |
| Scaling of the sum | Step `h` applied inside `calculate_function`, before reduction | Scaling by `(b-a)/n` applied on rank 0, after reduction |
| Convergence | Deterministic, error decreases predictably with `n` | Statistical, error decreases with `1/√n` |

## Accuracy and performance

| Method | Processes | Estimated value | Error vs π | Computational time |
|---|---|---|---|---|
| Monte Carlo (`MC_MPI.cpp`) | 1 | 3.14169 | 0.00010 | 26114 ms |
| Quadrature (`integral_mpi.cpp`) | 2 | 3.14159 | 0.00000 | 280 ms |

Beyond the massive speed advantage (~93× faster), the MPI quadrature is also deterministic and therefore inherently more accurate than the stochastic Monte Carlo method, which requires an extremely large number of samples to reduce its statistical error to a comparable level.

## Strong scaling (Quadrature)

In the following table are reported tha evolution of the values of **efficiency** and **speedup** related to the increasing number of tasks.

| N of tasks | Time [ms] | Theoretical Speedup | Real Speedup | Efficiency |
|---|---|---|---|---|
| 1 | 745 | 1 | 1 | 100% |
| 2 | 394 | 2 | 1.89 | 94.5% |
| 4 | 222 | 4 | 3.35 | 83.9% |
| 8 | 120 | 8 | 6.20 | 77.6% |
| 16 | 49 | 16 | 15.20 | 95% |

- The real speedup increases with the number of tasks but progressively diverges from the ideal theoretical speedup (from 1.89 on 2 tasks to 6.20 on 8 tasks): this growing gap is due to communication and synchronization overhead between processes, which becomes increasingly significant relative to the actual computation time as the workload per process decreases.
- Efficiency decreases steadily as the number of tasks grows (100% → 94.5% → 83.9% → 77.6%), reflecting the fact that communication overhead accounts for a progressively larger share of the total execution time relative to computation, which shrinks as the workload is split across more processes.

## Notes
- MPI communication used in both programs: `MPI_Bcast` (broadcast parameters), `MPI_Reduce` (sum partial results). The quadrature program additionally uses `MPI_Barrier` for timing synchronization.
- In both programs, the number of processes must not exceed the number of points/iterations, otherwise the program exits with an error (in the Monte Carlo version, all processes exit consistently after the broadcast; in the quadrature version, only rank 0 checks before broadcasting).
- Only rank 0 performs file I/O (both reading input and writing output) in both programs, avoiding race conditions on shared files.
- Each MPI process in the Monte Carlo program uses an independently seeded random engine, ensuring that processes do not generate correlated (duplicate) sequences of points.
- Compared to a sequential approach, both MPI-parallelized methods achieve significantly lower execution time; the quadrature method additionally achieves higher accuracy at equal or lower cost, since it is not subject to statistical sampling error.

