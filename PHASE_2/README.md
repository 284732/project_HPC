# 📡 MPI in C++ for High Performance Computing

## Overview

This repository collects a sequence of examples and case studies of increasing difficulty on parallel programming with **MPI (Message Passing Interface)** in C++.

The material covers the fundamental communication paradigms of distributed-memory systems, including point-to-point communication (blocking and non-blocking), collective communication, virtual topologies, domain decomposition techniques, and the implementation of an iterative Jacobi solver. Particular attention is devoted to concepts recurring in High Performance Computing (HPC) applications, such as halo exchange, synchronization between processes, and scalability of distributed algorithms.

The repository is organized as a sequence of thematic **chapters**, each dedicated to a specific aspect of MPI programming, accompanied by theoretical notes, commented source code, and representative execution outputs.

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
└── 03_Topologies/
    ├── Jacobi/                  ← Distributed Jacobi solver
    └── Virtual_Topologies/      ← Cartesian communicators and neighbor discovery
```

## 📚 Chapter Index

| Chapter | Topic | Main Concepts |
| --- | --- | --- |
| 00 | Introduction | HPC fundamentals, Scientific Computing, Fortran vs C++ comparison |
| 01a | Blocking Point-to-Point Communication | MPI_Send, MPI_Recv, deadlock prevention, ping-pong benchmark |
| 01b | Non-Blocking Point-to-Point Communication | MPI_Isend, MPI_Irecv, MPI_Wait, MPI_Waitall, computation-communication overlap |
| 02 | Collective Communication | Bcast, Scatter, Gather, Reduce, Allreduce, Alltoall |
| 03a | Distributed Jacobi Solver | Domain decomposition, halo exchange, convergence criterion |
| 03b | Virtual Topologies | MPI_Cart_create, MPI_Cart_shift, MPI_Sendrecv, Cartesian grids |

---

## ⚙️ Software Requirements

### MPI Implementation

The examples can be compiled indifferently with either MPICH or OpenMPI.

### Ubuntu / Debian

```bash
sudo apt install mpich build-essential
```

or

```bash
sudo apt install libopenmpi-dev openmpi-bin
```

Both implementations are fully compliant with the MPI standard and interchangeable for the purposes of this repository; there is no need to install both at the same time, and it is not advisable to mix them within the same compilation environment, to avoid conflicts between headers and runtime libraries.

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

The `mpicxx` compiler wrapper automatically provides, at compile and link time, the necessary include paths and MPI libraries, removing the need to specify them manually. The `-O2` flag enables a moderate compiler optimization level (balanced between compilation time and the performance of the generated code), while `-Wall` activates the standard set of compiler warnings, useful during development for catching common errors (uninitialized variables, suspicious comparisons, etc.) before execution even takes place.

---

## The MPI Programming Model

MPI follows the **SPMD (Single Program, Multiple Data)** paradigm:

* A single executable is launched across multiple processes.
* Each process has a unique identifier, the **rank**.
* Processes cooperate by exchanging messages through MPI communication routines.
* Every process executes the same source code, but follows different execution paths depending on its rank.

Minimal example:

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

Every MPI program must invoke `MPI_Init` before any other MPI call (typically as the first operational instruction in `main`, immediately after handling `argc`/`argv`) and `MPI_Finalize` as the last MPI call before the process terminates: no MPI function, with the exception of a very small number of queries explicitly specified separately by the standard, is defined outside this initialization/finalization window. Omitting `MPI_Finalize`, or invoking MPI functions after it, produces undefined behavior.

---

## Common MPI Datatypes

| C++ Type | MPI Datatype |
| --- | --- |
| int | MPI_INT |
| double | MPI_DOUBLE |
| float | MPI_FLOAT |
| char | MPI_CHAR |
| long | MPI_LONG |
| long long | MPI_LONG_LONG |

This table covers the scalar types most frequently used in the repository's chapters; MPI defines a broader set of predefined datatypes (including unsigned types, composite types such as `MPI_DOUBLE_INT` for the `MPI_MAXLOC`/`MPI_MINLOC` operations discussed in chapter 02, and the ability to build custom derived datatypes for non-contiguous data structures), not covered in this summary table because they are not necessary for understanding the basic examples.

---

## Educational Path

The repository follows a progression from elementary communication mechanisms to more advanced distributed algorithms:

```text
Introduction (00)
      ↓
Blocking Point-to-Point (01a)
      ↓
Non-Blocking Point-to-Point (01b)
      ↓
Collective Communication (02)
      ↓
Distributed Jacobi Solver (03a)
      ↓
Virtual Topologies (03b)
```

Note that, in this sequence, the Jacobi solver (03a) didactically precedes virtual topologies (03b): chapter 03a introduces domain decomposition and halo exchange using manual computation of neighbor ranks (without a Cartesian topology), while chapter 03b subsequently shows how the same neighbor-discovery operations can be delegated to `MPI_Cart_shift`, generalizing the decomposition pattern to multidimensional grids. Anyone working through the two chapters in this order therefore first observes the problem (manual neighbor management in a strip decomposition) and then the tool that simplifies its solution in more general cases (Cartesian topology), rather than the reverse path.

Each chapter contains:

* A theoretical framing of the topic covered
* Commented C++ implementations
* Representative execution examples, with annotated expected output
* References to relevant MPI concepts covered in previous chapters, where applicable
