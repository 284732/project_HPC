# 00 — Introduction: C++ for High Performance Computing

> Introductory chapter of the repository.

---

## Table of Contents

1. [Motivation and context](#1-motivation-and-context)
2. [Why C++ for scientific computing](#2-why-c-for-scientific-computing)
3. [Memory organization and numerical arrays](#3-memory-organization-and-numerical-arrays)
4. [Numerical libraries](#4-numerical-libraries)
5. [Fortran and C++ in HPC](#5-fortran-and-c-in-hpc)
6. [Compiler-level optimization](#6-compiler-level-optimization)
7. [MPI and distributed-memory parallelism](#7-mpi-and-distributed-memory-parallelism)
8. [Repository roadmap](#8-repository-roadmap)

---

## 1. Motivation and context

**High Performance Computing (HPC)** applications require the efficient use of the hardware resources available on modern computing systems: multicore CPUs, vector computing units (SIMD), accelerators (GPUs, and more generally devices with massively parallel architectures), and distributed-memory clusters made up of hundreds or thousands of compute nodes interconnected by high-speed, low-latency networks (Infiniband, proprietary torus or dragonfly networks, etc.).

The term "high performance" should not be understood in a vague sense: in the HPC context it typically refers to an application's ability to **scale**, that is, to reduce its execution time (or increase the size of the problem that can be handled in the same amount of time) in a way that is proportional, or as close to proportional as possible, to the increase in computing resources employed. This property — scalability — is the underlying goal that drives most of the design choices discussed in this repository, from the choice of programming language to the MPI communication patterns covered in later chapters.

Achieving good performance and good scalability requires simultaneous attention at multiple, interdependent levels:

* **Algorithmic level**: the computational and communication complexity of the chosen algorithm (for example, as discussed in chapter 03a, the choice between a targeted P2P communication pattern and a collective one depends on the structure of the algorithm's data dependencies).
* **Implementation level**: how the source code leverages (or fails to leverage) the features of the language and the available libraries to produce efficient machine code.
* **Hardware level**: how data is laid out in memory and accessed, so as to exploit the cache hierarchy, memory bandwidth, and the processor's vector computing units.
* **Distributed-system level**: how processes cooperate and exchange data across the interconnection network, the central topic of all subsequent chapters of this repository.

This introductory chapter focuses on the first three levels, providing the context needed to understand **why** certain implementation choices (memory contiguity, choice of language, use of optimized libraries) are relevant even before distributed communication is introduced.

## 2. Why C++ for scientific computing

Scientific applications are typically characterized by:

* Large numerical datasets, often multidimensional arrays on the order of millions or billions of elements in size.
* Intensive floating-point computations, often repeated an extremely large number of times within iterative loops (such as, for example, the iterations of the Jacobi method discussed in chapter 03a).
* Tight memory constraints, since the size of tractable problems is often limited by the RAM available per process/node, not just by computation time.
* Long execution times (hours or days, even on large clusters), which cause every percentage-point inefficiency in the code to be multiplied into a significant absolute cost in terms of machine time and energy consumption.

To address these constraints, HPC relies on languages capable of producing highly optimized code, with minimal or zero runtime overhead. Modern C++ (starting with the C++11 standard and later) offers, in this respect, a set of particularly relevant features:

* **Compile-time optimization.** The C++ compiler has complete information about types and program structure already at compile time, allowing aggressive optimizations (function inlining, dead code elimination, constant propagation, loop unrolling) with no runtime overhead, unlike languages with dynamic type resolution or just-in-time compilation.
* **Automatic vectorization.** The compiler can automatically transform loops operating on contiguous data into SIMD (Single Instruction, Multiple Data — see section 3) instructions, which perform the same arithmetic operation simultaneously on multiple data elements in a single clock cycle, provided the source code does not introduce obstacles to vectorization (dependencies between iterations, non-contiguous memory accesses, undeclared pointer aliasing).
* **Fine-grained memory control.** Unlike languages with a garbage collector, C++ lets the programmer explicitly control when and how memory is allocated, laid out, and released (through RAII — Resource Acquisition Is Initialization — and containers such as `std::vector`), avoiding non-deterministic garbage-collection pauses that would be unacceptable in high-performance numerical code with predictable timing constraints.
* **Access to optimized numerical libraries.** As discussed in detail in section 4, the C++ ecosystem includes direct bindings or header-only wrappers for the main linear algebra and numerical computing libraries used in HPC.

Thanks to this combination of features, C++ is widely used in computational physics, computational fluid dynamics (CFD), finite element analysis (FEA), high-performance machine learning infrastructure, and large-scale simulation frameworks.

## 3. Memory organization and numerical arrays

Memory access efficiency is one of the most decisive factors for the performance of a numerical program, often more relevant than asymptotic algorithmic complexity alone when operating on real hardware.

### 3.1 Memory hierarchy and cache

Modern processors do not access main RAM directly for every operation: they interpose a hierarchy of cache memories (typically L1, L2, L3), progressively larger but slower as one moves further from the compute core. When a piece of data is accessed, the entire **cache line** containing it (typically 64 bytes on current x86_64 processors, corresponding to 8 consecutive `double` values) is loaded into cache, not just the single requested value. This behavior has a direct and fundamental consequence for numerical code: **accessing data laid out contiguously in memory, in the same order in which it is physically arranged, maximizes the reuse of what has already been loaded into cache**, drastically reducing the number of accesses to main RAM (which, in terms of clock cycles, is one or two orders of magnitude slower than L1 cache).

A **contiguous** memory layout therefore simultaneously improves:

* **Cache utilization**: successive accesses more often find the data already available in cache (cache hit) instead of having to wait for it to be loaded from RAM (cache miss).
* **Memory bandwidth efficiency**: the transfer of contiguous blocks saturates the available bandwidth between RAM and processor more effectively than scattered transfers (gather/scatter), which require separate, not fully utilized memory transactions.

### 3.2 Contiguous containers in C++: `std::vector` and raw arrays

```cpp
#include <vector>

std::vector<double> x = {1.0, 2.0, 3.0};
```

`std::vector<T>` guarantees, by specification of the C++ standard, that its elements are stored in a **contiguous block of memory**, exactly like a traditional C array: `&x[0]`, `&x[1]`, `&x[2]` are consecutive addresses (aside from any padding introduced by the alignment of type `T`, not relevant for `double`). This is why `std::vector` is the reference data structure for numerical kernels and MPI communications: a pointer to the first element (`x.data()`, or `&x[0]`) and the size (`x.size()`) are sufficient to interface with both C-style numerical libraries and MPI communication routines, which operate on contiguous buffers identified by a base pointer and an element count (exactly the `buf`/`count` parameters seen in the prototypes of `MPI_Send`, `MPI_Bcast`, etc. in later chapters).

Raw arrays are just as frequently used, with identical contiguity properties:

```cpp
double x[3] = {1.0, 2.0, 3.0};
```

Both representations can be used directly in MPI calls because MPI does not know the C++ type of the container (for example std::vector, static arrays, or dynamically allocated memory). MPI routines simply receive a memory address (void*) and interpret the data based on the information the user provides through the datatype and count parameters.
For this reason, what matters is not the container used, but the fact that the data is stored contiguously in memory. Furthermore, the specified MPI type (for example MPI_INT, MPI_DOUBLE, etc.) and the declared number of elements must correspond exactly to the actual layout of the data in the buffer, so that MPI can correctly read or write the memory.

## 4. Numerical libraries

Modern scientific applications rarely reimplement fundamental numerical algorithms from scratch: instead, they rely on extremely optimized libraries, often developed and refined over decades, with implementations specific to the target hardware architecture.

| Library | Purpose | Technical notes |
| --- | --- | --- |
| **BLAS** (Basic Linear Algebra Subprograms) | Basic linear algebra operations | Organized into three "levels": Level 1 (vector-vector operations, e.g. dot product), Level 2 (matrix-vector operations), Level 3 (matrix-matrix operations, with the highest arithmetic intensity and therefore the greatest optimization potential). Highly hardware-specific optimized implementations exist (OpenBLAS, Intel MKL, ATLAS) |
| **LAPACK** (Linear Algebra PACKage) | Dense linear algebra (factorizations, eigenvalues, linear systems) | Built on top of BLAS primitives; provides routines for LU, Cholesky, and QR factorization, eigenvalue/eigenvector decomposition, and SVD |
| **ScaLAPACK** (Scalable LAPACK) | Distributed dense linear algebra | Extension of LAPACK to distributed memory via MPI; matrices are partitioned among processes according to a block-cyclic distribution scheme, designed to balance the computational load across processes even for algorithms with non-uniform access patterns (e.g. Gaussian elimination with pivoting) |
| **FFTW** (Fastest Fourier Transform in the West) | Fast Fourier Transform (FFT) | Includes distributed-memory parallel variants based on MPI, which typically require redistributing data among processes via an all-to-all communication pattern, conceptually analogous to `MPI_Alltoall` (chapter 02, section 7) |
| **PETSc** (Portable, Extensible Toolkit for Scientific Computation) | Framework for distributed scientific computing | Provides distributed data structures for sparse matrices and vectors, and solvers for linear and nonlinear systems (including Krylov-type iterative methods, preconditioners), with an abstraction over the underlying MPI communication |
| **Trilinos** | Framework for numerical simulation | Modular ecosystem of packages for distributed linear algebra, solvers, discretizations, and optimization, with goals partly overlapping those of PETSc, developed primarily at Sandia National Laboratories |
| **Eigen** | Header-only C++ linear algebra library | Requires no separate compilation/linking (the entire library is distributed as C++ template headers); uses expression templates to automatically fuse chained operations (e.g. `A*B + C`) into a single computation loop, avoiding the allocation of intermediate temporary results |
| **Armadillo** | High-level C++ linear algebra library | Syntax inspired by MATLAB/Octave, designed to favor rapid prototyping while maintaining competitive performance by delegating, where available, to underlying BLAS/LAPACK |

Example usage of Eigen, to compute the eigenvalues of a 2×2 matrix:

```cpp
#include <Eigen/Dense>

Eigen::Matrix2d A;

A << 4, -2,
     1,  5;

Eigen::EigenSolver<Eigen::Matrix2d> solver(A);

auto eigenvalues = solver.eigenvalues();
```

The main advantage of relying on these libraries, instead of reimplementing the corresponding algorithms, is twofold: on one hand, the implementations are typically optimized to a level (cache blocking, manual vectorization, microarchitecture-specific tuning) that is difficult to replicate within reasonable development time for a single application project; on the other hand, the numerical correctness of these algorithms (in particular their numerical stability with respect to floating-point rounding errors) has been extensively validated over decades of production use, a level of reliability that is hard to achieve for an implementation written from scratch.

## 5. Fortran and C++ in HPC

Fortran remains, even today, one of the most relevant languages in scientific computing, owing to its long history (dating back to the late 1950s) and the vast ecosystem of legacy numerical software written in this language, still widely used and maintained in many production HPC codes. Modern C++ achieves, under comparable conditions, performance equivalent to Fortran, additionally offering abstraction mechanisms (templates, object-oriented programming).

### 5.1 Memory ordering: row-major vs column-major

A relevant technical difference, and a common source of silent performance bugs when porting code from one language to another, concerns the storage order of multidimensional arrays:

| Language | Memory layout |
| --- | --- |
| Fortran | Column-major (by columns) |
| C/C++ | Row-major (by rows) |

```fortran
real :: A(100,100)
A(i,j)
```

```cpp
double A[100][100];
A[i][j];
```

In a **row-major** two-dimensional array (C/C++), the elements of the same row are contiguous in memory: `A[i][j]` and `A[i][j+1]` differ by a single element in address, while `A[i][j]` and `A[i+1][j]` differ by an entire row (100 elements, in the example above). In a **column-major** two-dimensional array (Fortran), the exact opposite holds: it is the elements of the same column that are contiguous, so `A(i,j)` and `A(i+1,j)` differ by a single element, while `A(i,j)` and `A(i,j+1)` differ by an entire column.

This difference is not a purely academic detail: it directly determines which **traversal order** (loop nesting) of a multidimensional array produces contiguous memory accesses, and therefore good cache performance, and which order instead produces accesses with a large **stride** (memory jumps of a size equal to an entire row/column at every iteration of the inner loop), heavily penalizing performance due to the resulting increase in cache misses:

```cpp
// C/C++, row-major: the INNER loop must iterate over the LAST index
// to obtain contiguous memory accesses
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        A[i][j] = ...;   // contiguous access: j varies fastest,
                          // consistent with the row-major layout
```

```fortran
! Fortran, column-major: the INNER loop must iterate over the FIRST index
do j = 1, N
  do i = 1, N
    A(i,j) = ...   ! contiguous access: i varies fastest,
                    ! consistent with the column-major layout
  end do
end do
```

Reversing the nesting order relative to the language's convention (for example, in C++, iterating over the row index `i` in the innermost loop) produces code that is **functionally correct** but with a high-stride access pattern, which generally degrades performance significantly, all the more so as the array size exceeds the capacity of the faster caches: each access risks requiring the loading of a new cache line, instead of reusing the one already loaded by the previous access. This effect is typically observable and measurable already for moderately sized matrices (hundreds of elements per side), and becomes increasingly pronounced as the problem size grows.

This consideration is not purely theoretical in the context of this repository: in chapter 03a (Jacobi solver), the 5-point stencil applied to a two-dimensional grid systematically accesses the four cardinal neighbors of each node; the choice of which grid dimension to decompose among processes (rows, as in the strip decomposition presented in that chapter, or columns) and the order in which the local update loops are nested in the C++ code have a direct impact on the performance of the local computation kernel, independent of the cost of the associated MPI communication.

### 5.2 Performance: language vs implementation

For portions of code that carry out an algorithm's fundamental mathematical computation (numerical kernels), actual performance is typically determined more by:

* **Compiler quality** and its optimization infrastructure (the two main compilers used in HPC, GCC and LLVM/Clang, as well as the Fortran compilers associated with them or proprietary compilers such as Intel oneAPI, largely share the same low-level optimization techniques).
* **Optimization flags** chosen at compile time (section 6).
* **Memory access patterns** in the source code (sections 3 and 5.1).
* **Vectorization**, automatic or assisted, of the computation loop.

than by the programming language chosen per se. Under equivalent optimization conditions (same flags, same memory access pattern, same level of vectorization achieved), modern implementations in Fortran and in C++ tend to show comparable performance for the same class of numerical kernels, since both are compiled to machine code by the same type of compiler optimization infrastructure. The performance differences observed in practice between two implementations of the same algorithm are therefore, in the vast majority of cases, attributable to differences in the memory access pattern actually written by the programmer (consistent or not with the language's native layout, section 5.1) rather than to intrinsic limitations of one language or the other.

## 6. Compiler-level optimization

Commonly used optimization flags include:

```bash
-O3 
```

* **`-O3`** enables the most aggressive optimization level among those standardly offered by the compiler (higher than `-O2`, which is instead used as the more conservative reference setting in this repository's compilation templates, see later chapters): it includes optimizations such as automatic loop unrolling, more aggressive function inlining, and — a point particularly relevant for numerical code — a more extensive attempt at automatic loop vectorization (auto-vectorization), transforming scalar loops into sequences of SIMD instructions where the code's data dependencies allow it. `-O3`, in some cases, can produce larger code and, for non-numerical code patterns, does not always guarantee an improvement over `-O2`; for dense numerical kernels, however, it is typically the preferred choice.

Under equivalent optimization conditions, modern implementations in Fortran and in C++ therefore tend to show comparable performance, as already noted in section 5.2: the choice of compilation flags is, in practice, a performance-impacting factor comparable to the choice of language itself, and should always be explicitly documented when reporting performance comparisons between different implementations of the same algorithm.

## 7. MPI and distributed-memory parallelism

While shared-memory parallelism (for example via threads, with models such as OpenMP) relies on execution units that share the same address space and can therefore directly access the same variables in memory, **MPI** enables communication between **independent** processes, each with its own private memory space, potentially distributed across physically separate nodes of a cluster and lacking any mechanism for direct access to each other's memory.

The MPI execution model is built on:

* **Multiple processes**, each a self-contained instance of the program, with its own memory, its own stack, and its own local variables.
* **Explicit message passing** as the sole communication mechanism between processes: no data is implicitly shared, every transfer of information must be explicitly coded by the programmer through a call to an MPI communication routine (the central topic of all subsequent chapters).
* **Distributed memory**: the total memory available across all processes of an MPI job can exceed that of a single physical node, making it possible to tackle problems whose data size exceeds the RAM available on a single machine — the primary practical motivation for adopting MPI in many HPC applications, in addition to parallelizing computation time.
* **SPMD (Single Program, Multiple Data)**: a single executable is repeatedly launched as distinct processes, each operating on a different portion of the problem's overall data, following the model already described in the repository's index file.

Each process executes the same source code, but operates on different portions of the data, typically distinguishing its own behavior through its own **rank** (the unique integer identifier assigned to each process within a communicator, a concept explored in depth starting from chapter 01a):

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

MPI forms the foundation of most large-scale HPC applications and remains, to this day, the dominant standard for distributed-memory parallel programming, adopted almost universally on world-class high-performance computing systems (from departmental clusters to exascale systems), often in combination with shared-memory parallelism models at the single-node level.

## 8. Repository roadmap

The material is organized according to increasing levels of complexity:

```text
Introduction (00)
      ↓
Blocking Point-to-Point Communication (01a)
      ↓
Non-Blocking Point-to-Point Communication (01b)
      ↓
Collective Communication (02)
      ↓
Distributed Jacobi Solver (03a)
      ↓
Virtual Topologies (03b)
```
 
