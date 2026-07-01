# `integral_mpi.cpp` — Parallel Integral Estimation with MPI

## Overview
This program estimates a definite integral over a generic interval with the workload distributed across multiple processes via **MPI**. 
Input parameters are read from `input.txt` and results are written to `output.dat`.

## What it computes
The program numerically approximates:

<div align="center">

$$
4 \cdot \int_{a}^{b} \frac{1}{1 + x^2}\,dx
$$

</div>

by summing, over `n` equally spaced sample points across `[a, b]`, the value of the integrand multiplied by the step size `h`:

<div align="center">

$$
\int_{a}^{b} f(x)\,dx \approx \sum_{i=1}^{n} f(x_i) \cdot h
$$

</div>

Since the integrand is $\dfrac{4}{1+x^2}$ over $[0, 1]$, the result converges to **π**.

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

3. **Broadcast**: the number of iterations and the interval bounds are broadcast from rank 0 to all other processes with `MPI_Bcast`, so every process has the full set of parameters.

```cpp
MPI_Bcast(&iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&bounds, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
```

4. **Workload distribution**: each process determines how many sample points (`local_tasks`) it must compute. The total is divided evenly (`iterations / size`), and the remainder is distributed one extra point at a time to the first ranks, so that no points are lost due to integer division.

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

5. **Interleaved sampling**: each process computes its own starting point `x_start`, offset according to its `rank`, and advances with a `step_per_task` equal to `step * size`. This means the sample points are **interleaved round-robin** across processes rather than split into contiguous blocks — process 0 takes points 0, size, 2·size, ...; process 1 takes points 1, size+1, 2·size+1, ...; and so on.

```cpp
// Each process calculates its starting point, the step and the step for the next x.
interval_amp = bounds[1] - bounds[0];
x_start = bounds[0] + (interval_amp / double(iterations)) * double(rank);
step = double(interval_amp / iterations);
// Once the task i has completed the calculation for one task, the next start with a distance of step_per_task.
step_per_task = step * double(size);
```

6. **Local computation**: each process accumulates its own `local_sum` by evaluating the integrand (already multiplied by the step `h`) at each of its assigned points, via the helper function `calculate_function`.

```cpp
// Each task calculates its local sum.
for (int i = 0; i < local_tasks; i++) {
    local_sum += calculate_function(x_start, step);
    x_start += step_per_task;
}
```

7. **Reduction**: `MPI_Reduce` (with `MPI_SUM`) collects and sums all local partial sums into a single global `estimate` on rank 0. Being a blocking collective, it also implicitly synchronizes all processes.

```cpp
MPI_Reduce(&local_sum, &estimate, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
```

8. **Timing**: execution time is measured between two `MPI_Barrier` calls (ensuring all processes are synchronized before and after) using `std::chrono::high_resolution_clock`.

9. **Output**: only rank 0 writes the final result to `output.dat` — the number of processes used, the estimated integral value, and the computation time in milliseconds.

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
