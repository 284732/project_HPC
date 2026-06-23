# 📡 MPI in C++ for High Performance Computing

## Overview

This repository collects a set of progressively advanced examples and case studies on parallel programming with **MPI (Message Passing Interface)** in C++.

The material covers the fundamental communication paradigms of distributed-memory systems, including point-to-point and collective communications, virtual topologies, domain decomposition techniques, and the implementation of an iterative Jacobi solver. Particular attention is devoted to concepts commonly encountered in High Performance Computing (HPC) applications such as halo exchange, synchronization, scalability, and performance analysis.

The repository is organized as a sequence of thematic modules, each focusing on a specific aspect of MPI programming and accompanied by theoretical notes, commented source code, and representative execution outputs.

---

## 🗂️ Repository Structure

```text
PHASE_2/
│
├── 00_Intro/                    ← Scientific Computing, HPC concepts and
│                                  Fortran vs C++ considerations
│
├── 01_Point_to_Point/           ← Point-to-point communication
│   ├── Blocking/                ← MPI_Send / MPI_Recv
│   └── NonBlocking/             ← MPI_Isend / MPI_Irecv
│
├── 02_Collective/               ← Collective communication primitives
│
├── 03_Topologies/
│   ├── Virtual_Topologies/      ← Cartesian communicators and neighbor discovery
│   └── Jacobi/                  ← Distributed Jacobi solver
│
└── 04_Benchmark/                ← Performance analysis and language comparison
```

---

## 📚 Contents

| Module | Topic                                     | Main Concepts                                                      |
| ------ | ----------------------------------------- | ------------------------------------------------------------------ |
| 00     | Introduction                              | HPC fundamentals, Scientific Computing, Fortran vs C++             |
| 01a    | Blocking Point-to-Point Communication     | MPI_Send, MPI_Recv, deadlock avoidance, ping-pong benchmark        |
| 01b    | Non-Blocking Point-to-Point Communication | MPI_Isend, MPI_Irecv, MPI_Wait, MPI_Waitall, communication overlap |
| 02     | Collective Communication                  | Broadcast, Scatter, Gather, Reduce, Allreduce, Alltoall            |
| 03a    | Virtual Topologies                        | MPI_Cart_create, MPI_Cart_shift, MPI_Sendrecv, Cartesian grids     |
| 03b    | Distributed Jacobi Solver                 | Domain decomposition, halo exchange, convergence detection         |
| 04     | Benchmarking and Performance Analysis     | Scalability, speedup, efficiency, Fortran vs C++ discussion        |

---

## ⚙️ Software Requirements

### MPI Implementation

The examples can be compiled using either MPICH or OpenMPI.

### Ubuntu / Debian

```bash
sudo apt install mpich build-essential
```

or

```bash
sudo apt install libopenmpi-dev openmpi-bin
```

---

## Compilation

Generic compilation template:

```bash
mpicxx -O2 -Wall -o program program.cpp
```

Execution example:

```bash
mpirun -np 4 ./program
```

The `mpicxx` compiler wrapper automatically provides the required include paths and MPI libraries during compilation and linking.

---

## MPI Programming Model

MPI follows the **SPMD (Single Program Multiple Data)** paradigm:

* A single executable is launched on multiple processes.
* Each process owns a unique identifier called **rank**.
* Processes cooperate by exchanging messages through MPI communication routines.
* Every process executes the same source code while following different execution paths according to its rank.

Example:

```cpp
#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "Process " << rank
              << " of " << size << std::endl;

    MPI_Finalize();

    return 0;
}
```

---

## Common MPI Datatypes

| C++ Type  | MPI Datatype  |
| --------- | ------------- |
| int       | MPI_INT       |
| double    | MPI_DOUBLE    |
| float     | MPI_FLOAT     |
| char      | MPI_CHAR      |
| long      | MPI_LONG      |
| long long | MPI_LONG_LONG |

---

## Educational Path

The repository follows a progression from basic communication mechanisms to more advanced distributed algorithms:

```text
Introduction
      ↓
Blocking Point-to-Point
      ↓
Non-Blocking Point-to-Point
      ↓
Collective Communication
      ↓
Virtual Topologies
      ↓
Domain Decomposition
      ↓
Jacobi Solver
      ↓
Performance Analysis
```

Each module contains:

* Theoretical background
* Commented C++ implementations
* Representative execution examples
* References to the relevant MPI concepts

---

## Performance Analysis

The final section includes benchmark-oriented material focused on:

* Execution time measurements
* Communication overhead evaluation
* Strong scaling analysis
* Speedup computation

[
S(p) = \frac{T_1}{T_p}
]

where (T_1) is the serial execution time and (T_p) is the execution time using (p) processes.

Efficiency is computed as:

[
E(p) = \frac{S(p)}{p}
]

These metrics are applied to the distributed Jacobi solver to evaluate scalability.

---

## References

* MPI Forum, *MPI Standard 4.1*
* William Gropp, Ewing Lusk, Anthony Skjellum, *Using MPI*
* Peter Pacheco, *An Introduction to Parallel Programming*
* Open MPI Documentation
* MPICH Documentation
* Intel oneAPI HPC Toolkit Documentation
