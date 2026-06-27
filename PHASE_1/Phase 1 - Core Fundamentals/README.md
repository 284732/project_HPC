# Phase 1: Core Fundamentals — The C++ language

## Overview

This phase establishes the C++ fundamentals needed to understand the rest of the Topic 2
project. C++ is a **compiled** and **statically typed** language: the source code is
translated once into optimized machine code, and every variable has a type fixed at
compile time. Compared with C, C++ adds object-oriented programming, templates and a very
broad standard library (the STL), while still allowing the programmer to drop down to a
low level when needed. This balance between abstraction and efficiency makes it widely
used in HPC.

Phase 1 covers the three subjects assigned to me in Topic 2:

1. **I/O** — `01_IO/`
2. **Conditional and iterative constructs** — `02_Conditional_Iterative/`
3. **Procedures (functions)** — `03_Procedures/`

Each folder contains a theoretical Markdown file (in English) and one or more compilable
`.cpp` source files that illustrate the concepts with runnable examples.

## Topics covered

### 1. I/O (`01_IO/`)

- Standard I/O with streams: `std::cout`, `std::cin`, `std::cerr` and the `<<` and `>>`
  operators.
- Format manipulators (`<iomanip>`): `std::setw`, `std::setprecision`, `std::fixed`.
- Strings (`std::string`) and reading lines with `std::getline`.
- Text file I/O: `std::ofstream`/`std::ifstream`, open modes.
- Binary I/O with `write()`/`read()` and `std::ios::binary`.
- Error handling through the **stream state** (`good`, `eof`, `fail`, `bad`) and,
  alternatively, through exceptions.

### 2. Conditional and iterative constructs (`02_Conditional_Iterative/`)

- Selection: `if`, `else if`, `else`, the ternary operator `?:`, `switch`/`case`.
- Native boolean type `bool` and relational/logical operators (short-circuit).
- Iteration: `for` loops, range-based `for` (C++11), `while`, `do-while`.
- Loop flow control: `break`, `continue`.

### 3. Procedures (`03_Procedures/`)

- Function definition, return type and parameters; prototypes.
- Passing by value, by **reference** (`int&`) and by pointer.
- `const` parameters, function overloading, default arguments.
- `std::vector` as a parameter and recursion.
- Variable scope and storage classes (`static`, local/global).

## Building and running

The examples only require a standard C++ compiler. The included `Makefile` builds all
sources with warnings enabled (`-Wall -Wextra -std=c++17`):

```bash
make        # build all examples in the three folders
make run    # run the non-interactive examples and show their output
make clean  # remove the generated executables
```

Alternatively, each file can be compiled individually, for example:

```bash
g++ -Wall -Wextra -std=c++17 01_IO/io_standard.cpp -o io_standard
./io_standard
```

Some examples (in particular `io_standard.cpp` with `std::cin`) read from the keyboard:
in that case input can be provided from the terminal or through redirection, for example
`echo "5 3.14" | ./io_standard`.

## Note on the HPC context

C++ is well suited to HPC because it combines ahead-of-time compilation, static typing and
direct memory access with high-level abstractions (STL containers, references, templates)
that the compiler can optimize aggressively. Compared with C, it offers more safety
(references instead of many pointers, `std::vector` instead of raw arrays, error handling
with exceptions) without giving up performance. These aspects are recalled in the
individual documents where relevant.
