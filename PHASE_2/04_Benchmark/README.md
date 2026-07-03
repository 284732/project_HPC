# C++ vs Fortran for HPC: Benchmark and Comparative Analysis

> This document answers a concrete question: **is it worth rewriting Fortran code in C++,
> and what is gained or lost in terms of performance?**

---

## 1. Context: Why the Comparison Is Not Straightforward

C++ and Fortran are both **compiled, statically typed languages with direct memory access**.
Given the same algorithm and compiler, the raw performance difference is often **less than
5–10%** — and depends far more on the **type of problem** than on the language itself.

However, structural differences exist that become significant in certain scenarios.

---

## 2. Fortran's Structural Advantage: The Aliasing Problem

This is the most frequently cited reason in the literature for why Fortran can be
**faster than C/C++ for numerical computation**.

### What Is Aliasing

In C/C++, two different pointers can point to the same memory region (alias).
The compiler **cannot know** whether this happens, so it must be conservative:

```cpp
// C++: the compiler CANNOT reorder these operations
// because it does not know whether a, b, and c point to the same memory
void add(double* a, double* b, double* c, int n) {
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];  // what if c == a? The compiler must assume it might be
}
```

In Fortran, the language **guarantees by standard** that subroutine arguments do not
overlap in memory (no aliasing). The compiler can therefore:
- Freely reorder instructions
- Automatically vectorize loops using SIMD
- Keep values in registers longer

```fortran
! Fortran: the compiler KNOWS that a, b, c do not overlap
subroutine add(a, b, c, n)
    real(8), intent(in)  :: a(n), b(n)
    real(8), intent(out) :: c(n)
    integer, intent(in)  :: n
    integer :: i
    do i = 1, n
        c(i) = a(i) + b(i)  ! the compiler can vectorize freely
    end do
end subroutine
```

### The C++ Solution: `__restrict__`

C++ provides the `__restrict__` keyword (non-standard, but supported by GCC, Clang,
and Intel compilers):

```cpp
void add(double* __restrict__ a, double* __restrict__ b,
         double* __restrict__ c, int n) {
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];  // the compiler can now vectorize
}
```

With `__restrict__`, C++ performance matches Fortran for these loops.
In Fortran this is the **default behavior**; in C++ it is opt-in and requires discipline.

**Source:** <https://beza1e1.tuxen.de/articles/faster_than_C.html> —
*"Fortran semantics say that function arguments never alias [...] This is why Fortran
is often faster than C. This is why numerical libraries are still written in Fortran."*

---

## 3. Real Benchmarks from the Literature (2019–2024)

### 3.1 — Array-Heavy Computation (Nuclear Physics)

A 2024 paper (arXiv:2409.06837) compares CMF (legacy Fortran) with CMF++ (modern C++
with Eigen) on the same equation-of-state algorithm in 1D, 2D, and 3D:

| Problem size        | Fortran runtime | C++ runtime | C++ speedup |
|---------------------|-----------------|-------------|-------------|
| 1D (10k systems)    | ~10 s           | ~0.3 s      | **~33×**    |
| 2D (10M systems)    | ~10⁴ s (est.)   | ~20 s       | **~500×**   |
| 3D (100M systems)   | extrapolated    | ~200 s      | **>1000×**  |

> ⚠️ Important caveat: the C++ advantage here does not come from the language itself
> but from a **different algorithmic complexity** (O(n) vs O(n log n)) and from the
> elimination of temporary arrays via Eigen. This is modern C++ vs **poorly written
> legacy Fortran** — not a fair language-to-language comparison.

### 3.2 — Simple Numerical Loops (Knapsack Algorithm)

An empirical paper (arXiv:1903.08936) compares identical implementations of the same
algorithm:

| Algorithm | Fortran (average) | C++ (average) | Winner    |
|-----------|-------------------|---------------|-----------|
| MTU1      | 59 s              | 30 s          | C++ ~2×   |
| MTU2      | orders of magnitude apart | —   | depends on sorting |

### 3.3 — Parallel Heat Equation (LLM & HPC Benchmark, 2024)

The paper arXiv:2504.03665 compares C++ and Fortran on AMD EPYC 7763 and Intel Xeon
with MPI scaling:

```
Scaling from 1 to 64 cores (heat equation, 10M nodes):
  C++:     scales well with core count
  Fortran: speedup up to ~5 cores, then stagnation

Matrix multiplication scaling (10k×10k, Intel Xeon):
  C++:     irregular behavior
  Fortran: irregular behavior (same issue)

DGEMM (Arm A64FX):
  Fortran: ~0.02 GFLOP/s (LLM-generated, unoptimized code)
  C++:     performance increases with problem size
```

> Note: these benchmarks use **LLM-generated code**, not hand-optimized implementations.
> The results reflect how easy or difficult it is to produce performant code in each
> language without expert tuning.

### 3.4 — QCD Libraries (Particle Physics)

The Grid project (arXiv:1512.03487) shows that C++ with explicit SIMD intrinsics
achieves **65% of theoretical peak** on Intel Core i7, **outperforming Fortran** for
SU(3) matrix lattice computations:

```
Peak performance on L2 cache (single core, Intel i7-3615QM):
  C++ (Grid with SIMD intrinsics): ~24 GFLOP/s  (~65% of peak)
  Equivalent Fortran:              ~18–20 GFLOP/s
```

---

## 4. What Actually Determines Performance in HPC

### 4.1 — When Fortran Wins

```
Scenario                                  Fortran advantage
────────────────────────────────────      ──────────────────
Dense loops over N-dimensional arrays     +5% to +20% typical
Code written 20+ years ago                Compiler knows the patterns well
No pointer usage                          No aliasing → aggressive optimization
Column-major array access                 Native layout, cache-friendly by default
```

**Direct quote** (arXiv:1910.06415, BACKUS paper, 2019):
> *"Fortran is almost best in terms of performances (secondary only to machine code),
> and in the realm of C and C++ in terms of programmer productivity."*

**Direct quote** (arXiv:2301.02432, "Myths and Legends in HPC"):
> *"It seems hard to replace Fortran with C or other languages and outperform it or
> even achieve the same baseline. This may be due to [...] the limited language features
> (e.g., no pointer aliasing) that enable more powerful optimizations."*

### 4.2 — When C++ Wins

```
Scenario                                  C++ advantage
────────────────────────────────────      ──────────────────
Complex data structures (graphs, trees)   No Fortran equivalent
Sorting and hashing algorithms            Optimized STL (sort, unordered_map)
Template metaprogramming                  Eigen, lazy evaluation, zero-copy
GPU and accelerator interoperability      Native CUDA, SYCL, HIP support
New code written from scratch             Modern compilers (Clang, NVHPC)
```

**Concrete data:** The TBPLaS 2.0 paper (arXiv:2509.26309) shows that C++ with
temporary array elimination via lazy evaluation (Eigen) can be
*"several times or even an order of magnitude faster than Fortran"* for algorithms
that Fortran handles with implicit temporary arrays inside functions.

### 4.3 — It Depends on the Compiler, Not Just the Language

From the observations in arXiv:2504.03665 (benchmarks on real architectures):

```
Architecture    | Language | MPI Scaling
────────────────|──────────|─────────────────────────
AMD EPYC 7763   | C++      | scales to 64 cores    ✓
AMD EPYC 7763   | Fortran  | scales to ~5 cores    ✗
Intel Xeon 8358 | C++      | anomalous behavior    ✗
Intel Xeon 8358 | Fortran  | anomalous behavior    ✗
Arm A64FX       | C++      | DGEMM grows with size ✓
Arm A64FX       | Fortran  | DGEMM ~0.02 GFLOP/s  ✗ (LLM code)
```

> Results change **drastically** with the target architecture.
> There is no universal winner.

---

## 5. Practical Comparison: MPI in C++ vs MPI in Fortran

From a pure MPI standpoint, the two APIs are **equivalent in performance** — MPI is
a C library, and both the Fortran and C++ bindings are wrappers with negligible overhead.

```
MPI Operation          | C++ vs Fortran overhead
───────────────────────|─────────────────────────
MPI_Send / MPI_Recv    | identical (same underlying library)
MPI_Bcast              | identical
MPI_Allreduce          | identical
MPI_Cart_create        | identical
Halo exchange          | identical
```

Performance differences in MPI code come from the **surrounding algorithm**, not from
the MPI calls themselves. For example, the memory layout of a subdomain (row-major in
C++ vs column-major in Fortran) can affect cache miss rates during halo exchange.

### The Memory Layout Problem in 2D Jacobi

```cpp
// C++: row-major arrays. u[i][j] → row i, column j
// Iterating over columns is cache-UNFRIENDLY:
for (int j = 0; j < N; j++)
    for (int i = 0; i < N; i++)   // ← BAD: jumps in memory
        u[i*N + j] = ...;

// Iterating over rows is cache-FRIENDLY:
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)   // ← GOOD: sequential access
        u[i*N + j] = ...;
```

```fortran
! Fortran: column-major arrays. A(i,j) → column j, row i
! Iterating column-by-column is cache-FRIENDLY in Fortran:
do j = 1, N
    do i = 1, N   ! ← GOOD in Fortran: sequential column access
        u(i,j) = ...
    end do
end do
```

> Porting Fortran code to C++ without inverting the loop order leads to cache
> performance degradation. This is **the most common bug** in Fortran-to-C++ translations.

---

## 6. Summary

### In One Sentence

> **Fortran is unmatched for dense loops over regular arrays written by experts.**
> **C++ wins in everything else, and with `__restrict__` plus a modern compiler it
> matches Fortran even on numerical loops.**

---

## References

- arXiv:2409.06837 — Phase Stability in Chiral Mean-Field Model (C++ vs Fortran benchmark)
- arXiv:1903.08936 — Knapsack algorithm: C++ vs Fortran empirical comparison
- arXiv:2504.03665 — LLM & HPC: Benchmarking on AMD / Intel / ARM (2024)
- arXiv:1512.03487 — Grid: C++ QCD library, SIMD performance vs Fortran
- arXiv:1910.06415 — BACKUS: Modern Fortran performance positioning
- arXiv:2301.02432 — Myths and Legends in HPC (aliasing analysis)
- arXiv:2509.26309 — TBPLaS 2.0: C++ lazy evaluation vs Fortran temporary arrays
- arXiv:2308.13274 — Fortran HLS on FPGAs (C++ vs Fortran HLS comparison)
- <https://beza1e1.tuxen.de/articles/faster_than_C.html> — Aliasing semantics explained
