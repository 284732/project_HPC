# 00 — Introduction: C++ for High Performance Computing

## Motivation

High Performance Computing (HPC) applications require the efficient use of modern hardware resources, including multicore CPUs, vector units, accelerators, and distributed-memory clusters.

Among the programming languages commonly adopted in scientific computing, C++ occupies a central role thanks to its combination of:

* High execution performance
* Direct control over memory management
* Compatibility with established numerical libraries
* Native support for parallel programming models
* Scalability from small applications to large scientific codes

This repository focuses on distributed-memory parallel programming using MPI and provides a collection of examples implemented entirely in modern C++.

---

## Why C++ for Scientific Computing?

Scientific applications are often characterized by:

* Large numerical datasets
* Intensive floating-point computations
* Strict memory requirements
* Long execution times

To address these challenges, HPC software relies on languages capable of producing highly optimized machine code.

Modern C++ provides:

* Compile-time optimization
* Automatic vectorization
* Fine-grained memory control
* Generic programming through templates
* Access to optimized numerical libraries

As a result, C++ is widely used in computational physics, computational fluid dynamics, finite element analysis, machine learning infrastructures, and large-scale simulation frameworks.

---

## Memory Layout and Numerical Arrays

Efficient memory access is one of the key factors affecting performance.

A contiguous memory layout improves:

* Cache utilization
* Memory bandwidth efficiency
* SIMD vectorization opportunities

Example using `std::vector`:

```cpp
#include <vector>

std::vector<double> x = {1.0, 2.0, 3.0};
```

The elements are stored contiguously in memory, making the structure suitable for numerical kernels and MPI communications.

Raw arrays are also frequently used:

```cpp
double x[3] = {1.0, 2.0, 3.0};
```

Both representations can be passed directly to MPI communication routines.

---

## Numerical Libraries

Modern scientific applications rarely implement numerical algorithms from scratch.

Instead, they rely on highly optimized libraries such as:

| Library   | Purpose                                |
| --------- | -------------------------------------- |
| BLAS      | Basic linear algebra operations        |
| LAPACK    | Dense linear algebra                   |
| ScaLAPACK | Distributed linear algebra             |
| FFTW      | Fast Fourier Transform                 |
| PETSc     | Scientific computing framework         |
| Trilinos  | Numerical simulation framework         |
| Eigen     | Header-only C++ linear algebra library |
| Armadillo | High-level linear algebra library      |

Example using Eigen:

```cpp
#include <Eigen/Dense>

Eigen::Matrix2d A;

A << 4, -2,
     1,  5;

Eigen::EigenSolver<Eigen::Matrix2d> solver(A);

auto eigenvalues = solver.eigenvalues();
```

These libraries provide performance comparable to hand-written implementations while significantly reducing development effort.

---

## Fortran and C++ in HPC

Fortran remains one of the most important languages in scientific computing due to its long history and extensive ecosystem of numerical software.

Modern C++, however, achieves comparable performance while offering additional abstraction mechanisms and software engineering tools.

### Memory Ordering

A notable difference concerns multidimensional arrays:

| Language | Memory Layout |
| -------- | ------------- |
| Fortran  | Column-major  |
| C/C++    | Row-major     |

Fortran:

```fortran
real :: A(100,100)
A(i,j)
```

C++:

```cpp
double A[100][100];
A[i][j];
```

Understanding memory ordering is important because traversal patterns directly influence cache efficiency.

### Performance

For numerical kernels, performance is typically determined more by:

* Compiler quality
* Optimization flags
* Memory access patterns
* Vectorization

than by the language itself.

Common optimization flags include:

```bash
-O3 -march=native
```

Under equivalent optimization conditions, modern Fortran and C++ implementations often exhibit comparable performance.

---

## MPI and Distributed-Memory Parallelism

While shared-memory parallelism relies on threads, MPI enables communication between independent processes, potentially distributed across multiple nodes of a cluster.

The MPI execution model is based on:

* Multiple processes
* Explicit message passing
* Distributed memory
* SPMD (Single Program Multiple Data)

Every process executes the same source code while operating on different portions of the data.

Example:

```cpp
#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank;
    int size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "Process "
              << rank
              << " of "
              << size
              << std::endl;

    MPI_Finalize();

    return 0;
}
```

MPI forms the foundation of most large-scale HPC applications and remains the dominant standard for distributed-memory parallel programming.

---

## Repository Roadmap

The material is organized according to increasing levels of complexity:

```text
Introduction
      ↓
Blocking Point-to-Point Communication
      ↓
Non-Blocking Point-to-Point Communication
      ↓
Collective Communication
      ↓
Virtual Topologies
      ↓
Domain Decomposition
      ↓
Distributed Jacobi Solver
      ↓
Performance Analysis
```

The final modules combine communication primitives and numerical algorithms to demonstrate realistic HPC workflows based on domain decomposition and iterative solvers.

---

## Learning Objectives

After completing this section, the reader should be able to:

* Understand the role of C++ in High Performance Computing
* Recognize the importance of memory locality and data layout
* Use numerical libraries commonly adopted in scientific applications
* Understand the differences between Fortran and C++ in HPC environments
* Interpret the MPI programming model
* Prepare for the distributed-memory examples presented in the subsequent modules
