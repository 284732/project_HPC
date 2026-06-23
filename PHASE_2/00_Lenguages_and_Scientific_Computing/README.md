# 00 — Languages and Scientific Computing

## Overview

Before introducing MPI, it is useful to understand why C++ is widely adopted in High Performance Computing (HPC) and how it compares to the traditional language of scientific computing: Fortran.

Modern HPC applications are typically written using one of three approaches:

- Fortran + MPI/OpenMP
- C/C++ + MPI/OpenMP
- Python + optimized native libraries

Although Fortran remains dominant in legacy scientific codes, modern C++ offers comparable performance together with better abstraction mechanisms, generic programming and large software ecosystems.

This section introduces:
- compiler optimizations
- vectorization
- cache-friendly memory layouts
- Fortran vs C++ comparison