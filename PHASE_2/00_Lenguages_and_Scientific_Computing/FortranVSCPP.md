# Fortran 90 vs C++

## Memory Layout

Both languages store multidimensional arrays contiguously in memory.

However:

- Fortran uses column-major ordering
- C++ uses row-major ordering

Example:

Fortran:
A(i,j)

C++:
A[i][j]

The memory traversal order can significantly affect cache performance.

---

## Scientific Computing

Historically Fortran has been preferred because:

- simple aliasing rules
- aggressive compiler optimizations
- strong numerical ecosystem

Examples:

- LAPACK
- BLAS
- ScaLAPACK
- PETSc

---

## Modern C++

Modern C++ provides:

- templates
- generic programming
- STL containers
- RAII resource management
- object-oriented design

These features simplify the development of large HPC applications.

---

## Performance

For dense numerical kernels:

- Fortran ≈ C ≈ C++

when compiled with

-O3
-march=native

The compiler is usually more important than the language itself.

---

## MPI

MPI support is essentially identical.

Fortran:

call MPI_Send(...)

C++:

MPI_Send(...)

The same MPI library is used underneath.