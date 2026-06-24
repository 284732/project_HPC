# Getting Started — Structure of an MPI Program in C++

This document is the practical starting point before tackling any exercise in this
tutorial. It explains how to set up the environment, which headers to include, how
to compile and run, and what the correct minimal structure of every MPI C++ program
looks like.

---

## 1. Required Includes

### For MPI

```cpp
#include <mpi.h>
```

This is the only header needed to access all MPI functions (`MPI_Init`, `MPI_Send`,
`MPI_Bcast`, etc.). It must be included **before** any MPI call and is typically the
first include in the file.

### Most Common C++ Standard Headers in HPC

```cpp
#include <iostream>   // std::cout, std::cerr
#include <vector>     // std::vector<T> — dynamic contiguous arrays
#include <cmath>      // std::sqrt, std::abs, M_PI
#include <iomanip>    // std::setprecision, std::setw — output formatting
#include <algorithm>  // std::max, std::min, std::swap
#include <numeric>    // std::iota, std::accumulate
#include <cstring>    // std::memcpy, strlen — raw buffer operations
```

### Recommended Include Order

Always follow this order for clarity and portability:

```cpp
// 1. MPI header (always first if using MPI)
#include <mpi.h>

// 2. C++ standard library headers
#include <iostream>
#include <vector>
#include <cmath>

// 3. Third-party libraries if needed (Eigen, etc.)
// #include <Eigen/Dense>

// 4. Local project headers
// #include "utils.h"
```

---

## 2. Compilers

### The Direct Compiler: `g++`

`g++` is the C++ compiler from GCC. It does not know about MPI on its own — you would
need to manually specify all MPI library and include paths.

```bash
# Do NOT do this manually:
g++ -I/usr/include/mpi -L/usr/lib/x86_64-linux-gnu/openmpi/lib \
    -lmpi -o program program.cpp
```

### The Recommended Wrapper: `mpicxx`

`mpicxx` (or `mpic++`) is a **wrapper** around `g++` that automatically adds all the
necessary include and linking flags for MPI. This is the correct way to compile MPI
programs.

```bash
mpicxx -O2 -Wall -std=c++14 -o program program.cpp
```

| Flag | Meaning |
|------|---------|
| `-O2` | Optimization level 2 (good speed/debug trade-off) |
| `-O3 -march=native` | Maximum optimization + CPU-specific instructions |
| `-Wall` | Enable all warnings — always use this |
| `-std=c++14` | C++14 standard (minimum recommended; use `c++17` if available) |
| `-g` | Debug symbols (required to use `gdb`) |

### Checking Your MPI Installation

```bash
# Check which MPI implementation is installed
mpicxx --version          # shows the underlying compiler
mpirun --version          # shows the MPI implementation (OpenMPI, MPICH, etc.)

# Ubuntu/Debian: install OpenMPI
sudo apt install libopenmpi-dev openmpi-bin

# Ubuntu/Debian: install MPICH (alternative)
sudo apt install mpich
```

> The two main implementations are **OpenMPI** and **MPICH**. For the exercises in
> this tutorial they are interchangeable. On HPC clusters, use whichever is available
> (typically Intel MPI or OpenMPI).

---

## 3. Running with `mpirun`

```bash
# Launch the program with N parallel processes
mpirun -np 4 ./program

# Equivalent (some systems use mpiexec)
mpiexec -n 4 ./program

# On a cluster with SLURM (no direct mpirun)
srun --ntasks=4 ./program
```

> `-np` and `-n` are equivalent. On clusters the launcher is chosen by the job
> scheduling system (SLURM, PBS, etc.).

---

## 4. Core MPI Concepts

### `MPI_Init` — Initialization

```cpp
MPI_Init(&argc, &argv);
```

**Must be the first MPI call** in the program. It initializes the MPI environment
and makes all communication mechanisms available. No MPI function can be used before
this call (except `MPI_Initialized`).

`argc` and `argv` are passed from `main` because some MPI implementations use
command-line arguments for internal configuration.

### `MPI_Finalize` — Shutdown

```cpp
MPI_Finalize();
```

**Must be the last MPI call** in the program. It releases all MPI resources and
synchronizes the shutdown of all processes. No MPI function can be used after this
call.

> ⚠️ If you exit the program without calling `MPI_Finalize` (e.g. via `exit()` or
> a crash), MPI may leave zombie processes or locked resources on the cluster.

### `MPI_COMM_WORLD` — The Global Communicator

```cpp
MPI_COMM_WORLD
```

This is the **default communicator** that includes all processes launched by `mpirun`.
A communicator is a group of processes that can communicate with each other.
`MPI_COMM_WORLD` is always available after `MPI_Init` and contains every process.

### `MPI_Comm_rank` — Who Am I?

```cpp
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

Assigns to `rank` the **unique identifier** of the current process within the
communicator. Ranks go from `0` to `size-1`. The rank is how each process knows
its own identity and decides what to do (e.g. `if (rank == 0)` for the master
process).

### `MPI_Comm_size` — How Many Are We?

```cpp
int size;
MPI_Comm_size(MPI_COMM_WORLD, &size);
```

Assigns to `size` the **total number of processes** in the communicator. This
corresponds to the value passed to `mpirun -np`.

---

## 5. Minimal Structure of Every MPI Program

This is the starting template from which all tutorial exercises are derived:

```cpp
#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {

    // ── 1. Initialization ────────────────────────────────────────────
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ── 2. Program Logic ─────────────────────────────────────────────
    // This code is executed by ALL processes simultaneously.
    // The 'rank' variable is used to differentiate behavior.

    if (rank == 0) {
        // Only the master process (rank 0) runs this block
        std::cout << "Master: running with " << size << " processes." << std::endl;
    } else {
        // All other processes run this
        std::cout << "Worker " << rank << ": ready." << std::endl;
    }

    // ── 3. Shutdown ──────────────────────────────────────────────────
    MPI_Finalize();
    return 0;
}
```

### Compilation and Execution

```bash
mpicxx -O2 -Wall -o hello hello.cpp
mpirun -np 4 ./hello
```

### Expected Output (order may vary)

```
Master: running with 4 processes.
Worker 1: ready.
Worker 3: ready.
Worker 2: ready.
```

> Output order is **non-deterministic**: each process writes to `cout` whenever it
> reaches that line, independently of the others. This is normal and expected in MPI.

---

## 6. Common Mistakes to Avoid

### Forgetting `MPI_Init` or `MPI_Finalize`

```cpp
// ✗ WRONG: MPI call before Init
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // undefined behavior
MPI_Init(&argc, &argv);
```

```cpp
// ✗ WRONG: exiting without Finalize
MPI_Init(&argc, &argv);
// ...
return 0;  // MPI was not shut down correctly
```

### Printing Without Checking the Rank

```cpp
// ✗ OFTEN UNINTENDED: every process prints the same line
// → output repeated once per process
std::cout << "Final result: " << result << std::endl;

// ✓ CORRECT: only process 0 prints the global result
if (rank == 0)
    std::cout << "Final result: " << result << std::endl;
```

### Using Uninitialized Buffers in MPI Calls

```cpp
// ✗ WRONG: buf is uninitialized; MPI_Recv will write into undefined memory
double buf;
MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

// ✓ CORRECT: always initialize receive buffers
double buf = 0.0;
MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

---

## 7. Connection to the Rest of the Tutorial

Once this base structure is clear, every exercise in the tutorial follows the same
pattern: `MPI_Init` → rank-differentiated logic → `MPI_Finalize`. What changes
is the **communication logic** in between:

```
getting_started.md   ←  you are here
        ↓
01_point_to_point/   ←  MPI_Send / MPI_Recv between two processes
        ↓
02_communicators/    ←  subgroups and custom communicators
        ↓
03_collective/       ←  Bcast, Scatter, Gather, Reduce across all processes
        ↓
04_topologies/       ←  virtual grids and 2D Jacobi solver
```