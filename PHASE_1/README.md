# The C++ language in High-Performance Computing

## Project overview (Topic 2)

This project discusses the use of the **C++** language in the context of High-Performance
Computing (HPC). C++ is a compiled, statically typed language that extends C with
object-oriented programming, templates and a rich standard library (STL). It is widely
used in high-performance scientific computing because it combines the low-level control
typical of C with high-level abstractions.

The Topic 2 subjects are: **I/O, conditional and iterative constructs, procedures, arrays
and MPI** (point-to-point and collective communications), together with a case study in
which the language is used to solve a problem in parallel.

The work is organized in phases.


## My part: Phase 1 - Core Fundamentals

This repository contains **Phase 1**, which I worked on (Giacomo Michelini). The phase
introduces the C++ fundamentals needed to understand the rest of the project and covers in
particular the three subjects assigned to me:

1. **I/O** – input/output with the standard library streams (`std::cin`/`std::cout`,
   `std::ifstream`/`std::ofstream`, ...).
2. **Conditional and iterative constructs** – `if`/`else`, `switch`, the ternary
   operator, `for`, range-based `for`, `while`, `do-while`, `break`/`continue`.
3. **Procedures (functions)** – definition and prototypes, passing by value, by
   reference (`&`) and by pointer, `const` parameters, overloading, default arguments,
   recursion, scope and storage classes.

Each subject is documented in a Markdown file (in English) and accompanied by real,
compilable and runnable `.cpp` source files. Details on how to compile and run the
examples can be found in the Phase 1 README.

## Quick build

All examples can be compiled with a standard C++ compiler (e.g. `g++`). From the
`Phase 1 - Core Fundamentals` directory:

```bash
make        # build all examples
make run    # build and run the non-interactive examples
make clean  # remove the executables
```
