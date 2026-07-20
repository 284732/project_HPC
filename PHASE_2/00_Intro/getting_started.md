# Getting Started — Structure of an MPI Program in C++

> This document is the practical starting point before tackling any chapter of this tutorial. It conceptually precedes chapter 00 (theoretical introduction to C++ for HPC): while that chapter motivates the language choices and the relevant performance characteristics, this document covers the purely operational aspects — environment, headers, compilation, execution — needed to get even the simplest of the MPI programs presented in later chapters up and running.

---

## Table of Contents

1. [Required headers](#1-required-headers)
2. [Compilers](#2-compilers)
3. [Running with mpirun](#3-running-with-mpirun)
4. [Core MPI concepts](#4-core-mpi-concepts)
5. [Syntactic differences: Fortran `call` vs direct calls in C++](#5-syntactic-differences-fortran-call-vs-direct-calls-in-c)
6. [Minimal structure of every MPI program](#6-minimal-structure-of-every-mpi-program)
7. [Common mistakes to avoid](#7-common-mistakes-to-avoid)
8. [Connection to the rest of the tutorial](#8-connection-to-the-rest-of-the-tutorial)

---

## 1. Required headers

### For MPI

```cpp
#include <mpi.h>
```

This is the only header needed to access all MPI functions (`MPI_Init`, `MPI_Send`, `MPI_Bcast`, etc.). It must be included **before** any MPI call, and is typically the first include in the file. From a technical standpoint, `mpi.h` is a C header (MPI implementations expose a standard C interface, not a native C++ one).

### Most common C++ standard headers in HPC

```cpp
#include <iostream>   // std::cout, std::cerr
#include <vector>     // std::vector<T> — contiguous dynamic arrays (chapter 00, section 3.2)
#include <cmath>      // std::sqrt, std::abs, M_PI
#include <iomanip>    // std::setprecision, std::setw — output formatting
#include <algorithm>  // std::max, std::min, std::swap
#include <numeric>    // std::iota, std::accumulate
#include <cstring>    // std::memcpy, strlen — raw buffer operations
```

A few notes on headers that recur throughout this tutorial: `<cmath>` is needed whenever standard math functions are used (for example `std::fabs` to compute pointwise variations in the Jacobi solver, chapter 03a); `<algorithm>` provides `std::swap`, used systematically in the Jacobi solver to swap the `u`/`u_new` buffer pointers without copying (chapter 03a, section 2.7).

### Recommended include order

For clarity and portability, it is good practice to always follow this order:

```cpp
// 1. MPI header (always first, if MPI is used)
#include <mpi.h>

// 2. C++ standard library headers
#include <iostream>
#include <vector>
#include <cmath>

// 3. Third-party libraries, if needed (Eigen, etc. — see chapter 00, section 4)
// #include <Eigen/Dense>

// 4. Local project headers
// #include "utils.h"
```

This ordering is not purely stylistic: including `mpi.h` first reduces the risk of macro conflicts or symbol redefinition with other libraries that might, in turn, internally include system headers in an order different from the one expected by the MPI implementation in use.

## 2. Compilers

### The direct compiler: `g++`

`g++` is the C++ compiler from the GCC project. It has no native knowledge of MPI: to compile a program that uses MPI with `g++` directly, one would need to manually specify all the include and linking paths of the MPI library installed on the system:

```bash
# Do NOT do this manually:
g++ -I/usr/include/mpi -L/usr/lib/x86_64-linux-gnu/openmpi/lib \
    -lmpi -o program program.cpp
```

This approach is fragile: the exact paths of MPI headers and libraries vary depending on the Linux distribution, the installed MPI implementation (OpenMPI, MPICH, Intel MPI, etc.), and the specific version, making the command above non-portable across different systems.

### The recommended wrapper: `mpicxx`

`mpicxx` (sometimes also available as `mpic++`, depending on the implementation) is a **wrapper** around `g++` (or the system C++ compiler configured by the MPI implementation, which may also be Clang or a proprietary compiler) that automatically adds all the include and linking flags needed for MPI, by querying the configuration of the MPI implementation installed at the time it was itself compiled/installed. This is the correct way to compile MPI programs, regardless of the system being worked on:

```bash
mpicxx -O2 -Wall -std=c++14 -o program program.cpp
```

| Flag | Meaning |
| --- | --- |
| `-O2` | Optimization level 2 (a good trade-off between execution speed and ease of debugging; this is the level used as the reference setting in this tutorial's compilation templates) |
| `-O3 -march=native` | Maximum optimization, including instructions specific to the current CPU (see chapter 00, section 6, for a detailed explanation of the meaning and portability risks of `-march=native`) |
| `-Wall` | Enables the standard set of compiler warnings — always use this, since it catches, at compile time, a broad class of common errors (uninitialized variables, suspicious comparisons between different types, functions with unhandled return values) before execution even takes place |
| `-std=c++14` | Selects the C++14 standard (the minimum recommended for future exercises in the repo; use `-std=c++17` if available on the system, to access more recent language features) |

### Checking your MPI installation

```bash
# Check which MPI implementation is installed
mpicxx --version          # shows the underlying compiler used by the wrapper
mpirun --version          # shows the MPI implementation (OpenMPI, MPICH, etc.)

# Ubuntu/Debian: installing OpenMPI
sudo apt install libopenmpi-dev openmpi-bin

# Ubuntu/Debian: installing MPICH (alternative)
sudo apt install mpich
```

> The two main implementations are **OpenMPI** and **MPICH**. For the exercises that follow they are interchangeable, since both faithfully implement the MPI standard and the C++ code syntax presented does not depend on any extension specific to either one.

## 3. Running with `mpirun`

```bash
# Launch the program with N parallel processes
mpirun -np 4 ./program

# Equivalent (some systems use mpiexec)
mpiexec -n 4 ./program

# On a cluster with SLURM (no direct mpirun)
srun --ntasks=4 ./program
```

> `-np` and `-n` are equivalent across common implementations. On clusters managed by a job scheduling system (SLURM, PBS/Torque, LSF), the actual process launcher is determined by the scheduling system itself, not invoked manually by the user as in the two previous commands.

It is worth clarifying the distinction between `mpirun`/`mpiexec` and `srun`: in the first two cases, it is the MPI implementation itself (through its own launcher) that takes care of starting the processes on the available nodes, typically based on a host file or local environment variables; with `srun` (SLURM), it is instead the cluster's resource manager that takes charge of allocating resources (nodes, cores, memory) and starting the processes, delegating to MPI only the phase in which the already-launched processes discover one another and build the communication channels between them (a phase known as *PMI bootstrap*, Process Management Interface). On a production cluster running SLURM, it is therefore typically `srun` (or the invocation of `mpirun` from within a SLURM script, depending on the local configuration) that is actually used, rather than `mpirun` invoked completely on its own as one would on a single development workstation.

## 4. Core MPI concepts

### `MPI_Init` — Initialization

```cpp
MPI_Init(&argc, &argv);
```

**Must be the first MPI call** in the program. It initializes the MPI runtime environment and makes all communication mechanisms available: before this call, no MPI function can be used.

`argc` and `argv` are passed from `main` by pointer because some MPI implementations use command-line arguments for internal configuration (for example launcher-specific parameters, filtered out and removed from the arguments before the application program sees them), and `MPI_Init` may therefore modify their contents. It is good practice to always pass `argc`/`argv` to `MPI_Init` even when the application program does not need its own command-line arguments, to guarantee full compatibility with this implementation-specific behavior.

### `MPI_Finalize` — Shutdown

```cpp
MPI_Finalize();
```

**Must be the last MPI call** in the program. It releases all resources allocated by MPI (internal communication buffers, network channels, communicator management data structures) and synchronizes the orderly shutdown of all processes. No MPI function may be invoked after this call.

> ⚠️ If the program terminates without invoking `MPI_Finalize` (for example via a direct `exit()`, or as the result of an unhandled crash), MPI may leave "zombie" processes or communication resources locked on the cluster, with effects ranging from a simple temporary waste of resources up to, in more serious cases on shared systems, the need for manual intervention by the system administrator to free the network/scheduling resources that remained allocated.

### `MPI_COMM_WORLD` — The global communicator

```cpp
MPI_COMM_WORLD
```

This is the **default communicator** that includes all processes launched by `mpirun`. A communicator is a group of processes that can communicate with each other (a concept explored in depth in chapter 01a, section 2.3). `MPI_COMM_WORLD` is always available immediately after `MPI_Init` and contains, by definition, every process of the current MPI job: it is the default choice for nearly all the communications presented in the exercises that follow in the repo.

### `MPI_Comm_rank` — Who am I?

```cpp
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

Assigns to `rank` the **unique identifier** of the current process within the specified communicator. Ranks range from `0` to `size-1`. The rank is the mechanism by which each process determines its own identity and decides its own behavior (for example `if (rank == 0)` to distinguish the master process, a recurring pattern throughout this tutorial's repo).

### `MPI_Comm_size` — How many are we?

```cpp
int size;
MPI_Comm_size(MPI_COMM_WORLD, &size);
```

Assigns to `size` the **total number of processes** present in the specified communicator. This value corresponds exactly to the one passed to `mpirun -np` (or to `-n`, `--ntasks`, depending on the launcher used, section 3), for the `MPI_COMM_WORLD` communicator.

## 5. Syntactic differences: Fortran `call` vs direct calls in C++

Anyone coming from MPI in Fortran will immediately encounter three systematic syntactic differences in every MPI program written in C++.

### Absence of the `call` keyword

In Fortran, subroutines are invoked with the `call` keyword:

```fortran
call MPI_Init(ierr)
call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
```

In C++, MPI functions are standard C functions and are invoked directly by name, with no introductory keyword:

```cpp
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

There is no equivalent of `call`: every MPI routine is simply a function call expression, syntactically identical to any other C++ function call.

### Error code: last argument vs return value

In Fortran, every MPI routine accepts `ierr` as its **last argument**, an output parameter through which the routine communicates the outcome of the operation:

```fortran
call MPI_Send(buf, count, MPI_DOUBLE_PRECISION, dest, tag, MPI_COMM_WORLD, ierr)
```

In C++, the error code is instead the **return value** of the function itself:

```cpp
int err = MPI_Send(buf, count, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
```

In practice, in the code of this tutorial the return value is often ignored (neither of the two variants, `int err = ...` or the "bare" call with no assignment, alters the behavior of the call itself), but it can and should be checked explicitly, typically by comparing it against the `MPI_SUCCESS` constant. MPI's default behavior, in the absence of a custom error handler explicitly installed via `MPI_Comm_set_errhandler` (not covered later in the repo), is in any case to terminate the program with a diagnostic message at the first detected error condition, making explicit checking of the return value an additional robustness measure rather than strictly necessary for the correctness of the exercises presented here.

### Pointers for output arguments

This is the most important difference and the most common source of compilation errors for those coming from Fortran. In Fortran, all arguments are passed **by reference by default** — the compiler handles memory addresses automatically, transparently to the programmer:

```fortran
integer :: rank
call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
! Fortran automatically passes the address of 'rank' to MPI
```

In C++, when MPI needs to **write a value into a variable** (an output argument), its address must be passed explicitly using the `&` operator:

```cpp
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//                             ^
//                             Required: passes the memory address of 'rank'
//                             Without &, MPI cannot modify the variable
//                             (the current VALUE would be passed instead, not
//                             undefined but useless for the purposes of the call:
//                             the C++ compiler would in any case report a
//                             type error, since MPI_Comm_rank expects an
//                             int*, not an int)
```

For **data buffers** (arrays), the `&` operator is not needed, because an array name automatically decays to a pointer to its first element, according to the standard semantics of the C/C++ language:

```cpp
double buf[100];
MPI_Send(buf, 100, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
//        ^ no & needed: 'buf' is already a pointer

// With std::vector, use .data() to get the pointer to the internal buffer:
std::vector<double> v(100);
MPI_Send(v.data(), 100, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
```

**Quick reference:**

| Argument type | Fortran | C++ |
| --- | --- | --- |
| Scalar output (rank, size, etc.) | `rank` | `&rank` |
| Data array | `buf` | `buf` or `v.data()` |
| Communicator (input) | `MPI_COMM_WORLD` | `MPI_COMM_WORLD` |
| Request (output) | `request` | `&request` |
| Status (output) | `status` | `&status` or `MPI_STATUS_IGNORE` |

## 6. Minimal structure of every MPI program

This is the starting template from which all subsequent exercises are derived:

```cpp
#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {

    // ── 1. Initialization ────────────────────────────────────────────
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ── 2. Program logic ─────────────────────────────────────────────
    // This code is executed by ALL processes simultaneously.
    // The 'rank' variable is used to differentiate their behavior.

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

### Compilation and execution

```bash
mpicxx -O2 -Wall -o hello hello.cpp
mpirun -np 4 ./hello
```

### Expected output (order may vary)

```text
Master: running with 4 processes.
Worker 1: ready.
Worker 3: ready.
Worker 2: ready.
```

> The order of the output is **non-deterministic**: each process writes to `cout` as soon as it reaches that line of code, independently of the other processes, which proceed with their own execution at speeds that are not synchronized with one another. This is normal and expected in MPI. It should also be noted that, since these are `size` independent processes (not threads of a single process sharing a physically common `cout` stream), the individual writes to `cout` from each process do not risk interleaving *at the level of individual characters* within the same line (each process writes to its own independent standard output stream, typically routed by the MPI launcher to the same terminal), but **the relative order between lines emitted by different processes** is in no way guaranteed by the MPI standard, and can vary from one run to another of the exact same program, even with identical hardware and number of processes.

## 7. Common mistakes to avoid

### Forgetting `MPI_Init` or `MPI_Finalize`

```cpp
// WRONG: MPI call before Init
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // undefined behavior
MPI_Init(&argc, &argv);
```

```cpp
// WRONG: exiting the program without Finalize
MPI_Init(&argc, &argv);
// ...
return 0;  // MPI was not shut down correctly
```

### Printing without checking the rank

```cpp
// OFTEN UNINTENTIONAL: every process prints the same line
// → output repeated once per process
std::cout << "Final result: " << result << std::endl;

// CORRECT: only process 0 prints the global result
if (rank == 0)
    std::cout << "Final result: " << result << std::endl;
```

This mistake is particularly insidious when `result` is, in fact, a value that **should** be identical across all processes (for example the outcome of an `MPI_Allreduce`, chapter 02, section 6): the program produces output that is apparently "correct" in the numerical value printed, but repeated `size` times — a flaw that is easily overlooked during development with a small number of processes, and that quickly becomes annoying (or problematic, if the output is redirected and subsequently processed by automated scripts) as the number of processes used grows.

### Using uninitialized buffers in MPI calls

```cpp
// WRONG: buf is uninitialized; MPI_Recv will write into undefined memory
double buf;
MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

// CORRECT: always initialize receive buffers
double buf = 0.0;
MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

The technical reason behind this recommendation should be made clear: `MPI_Recv` will in any case fully overwrite `buf` with the contents of the received message (assuming the reception succeeds), so the initial value of `buf` never influences the *result* of the receive operation itself. Explicit initialization is, however, a relevant defensive robustness measure in more complex scenarios: during debugging (to distinguish at a glance, in a memory dump or in a debugger, a buffer actually populated by MPI from one never touched due to a sender/receiver matching bug), and especially in scenarios where the reception might silently fail or not complete due to a logic error elsewhere in the program (for example a tag or rank mismatch, chapter 01a, section 12): a buffer initialized to a known, recognizable value (e.g. `0.0`, or a sentinel value clearly anomalous for the application domain) makes it much easier to diagnose, after the fact, whether that particular reception actually occurred as expected.

## 8. Connection to the rest of the repo

Once this basic structure is clear, every exercise follows the same general pattern: `MPI_Init` → rank-differentiated logic → `MPI_Finalize`. What changes, chapter after chapter, is exclusively the **communication logic** inserted in between:

```text
getting_started.md        ←  you are here
        ↓
00_Intro/                 ←  chapter 00 — motivations, C++ for HPC, numerical libraries
        ↓
01_Point_to_Point/         ←  chapter 01a/01b — MPI_Send/MPI_Recv (blocking and non-blocking) between two processes
        ↓
02_Collective/              ←  chapter 02 — Bcast, Scatter, Gather, Reduce across all processes
        ↓
03_Topologies/
   ├── Jacobi/                ←  chapter 03a — distributed Jacobi solver
   └── Virtual_Topologies/    ←  chapter 03b — virtual Cartesian grids
```

This document, together with the readme of chapter 00_intro, therefore constitutes the common prerequisite for all subsequent chapters: from here on, each new chapter will introduce exclusively the MPI functions specific to the communication pattern being covered, taking for granted the general structure (`Init`/`Finalize`, rank handling, compilation with `mpicxx`, execution with `mpirun`) presented in this document.
