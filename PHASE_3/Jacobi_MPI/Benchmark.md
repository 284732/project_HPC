# Benchmark: Jacobi 2D Solver — C++ vs Fortran

Comparison of two equivalent MPI implementations of the 2D Jacobi heat diffusion solver with Cartesian topology decomposition.

---

## Files

| File | Language | Compiler |
|------|----------|----------|
| `jacobi_2d_full.cpp` | C++ | `mpicxx` |
| `jacobi_2d_full.f90` | Fortran 90 | `mpif90` |

---

## Problem Setup

| Parameter | Value |
|-----------|-------|
| Grid size | 128 × 128 |
| EPS (convergence threshold) | 1e-5 |
| MAX_ITER | 10000 |
| Boundary condition | bottom row = 1.0, rest = 0.0 |
| Iterations to convergence | **8242** (identical in both languages) |

---

## Compilation

```bash
# C++
mpicxx -O2 -Wall -o jacobi2d_cpp jacobi_2d_full.cpp

# Fortran
mpif90 -O2 -Wall -o jacobi2d_f jacobi_2d_full.f90
```

> Same `-O2` flag used for both — required for a fair comparison.

---

## How to Run the Benchmark

### Automated timing script (`run_benchmark.sh`)

```bash
#!/bin/bash
PROCS=(1 2 4)
OUTFILE="benchmark_results.csv"

echo "lang,np,time_s,iters" > $OUTFILE

for NP in "${PROCS[@]}"; do
    for LANG in cpp f; do
        BIN="./jacobi2d_${LANG}"
        OUTPUT=$(mpirun -np $NP $BIN 2>/dev/null)

        TIME=$(echo  "$OUTPUT" | grep "Total time"   | grep -oP '[0-9]+\.[0-9]+')
        ITERS=$(echo "$OUTPUT" | grep -oP '(?<=after )[0-9]+')

        echo "${LANG},${NP},${TIME},${ITERS}" >> $OUTFILE
        echo "[${LANG}] np=${NP}  time=${TIME}s  iters=${ITERS}"
    done
done

echo ""
echo "Risultati salvati in $OUTFILE"
```

```bash
chmod +x run_benchmark.sh
./run_benchmark.sh
```

> **Note:** Only `np = 1, 2, 4` are tested. `np=3` is excluded because N=128
> is not divisible by 3, causing unequal subdomain sizes and unreliable results.
> Values above 4 require more physical cores than available on this machine.

### Speedup analysis (`analyze.py`)

```python
import csv

results = {}
with open("benchmark_results.csv") as f:
    for row in csv.DictReader(f):
        lang = row["lang"]
        np   = int(row["np"])
        t    = float(row["time_s"])
        results[(lang, np)] = t

print(f"{'lang':>6} {'np':>4} {'time':>8} {'speedup':>9} {'efficiency':>12}")
for (lang, np), t in sorted(results.items()):
    t1 = results[(lang, 1)]
    sp = t1 / t
    eff = sp / np * 100
    print(f"{lang:>6} {np:>4} {t:>8.3f} {sp:>9.2f} {eff:>11.1f}%")
```

```bash
python3 analyze.py
```

---

## Results (local machine, 4 physical cores)

### Wall-clock time

| np | C++ time (s) | Fortran time (s) | Fortran faster by |
|----|-------------|-----------------|-------------------|
| 1  | 0.258       | 0.206           | 20%               |
| 2  | 0.149       | 0.108           | 28%               |
| 4  | 0.117       | 0.134           | C++ faster by 13% |

### Speedup and parallel efficiency

| lang | np | time (s) | speedup | efficiency |
|------|----|----------|---------|------------|
| cpp  | 1  | 0.258    | 1.00    | 100.0%     |
| cpp  | 2  | 0.149    | 1.73    |  86.6%     |
| cpp  | 4  | 0.117    | 2.21    |  55.1%     |
| f    | 1  | 0.206    | 1.00    | 100.0%     |
| f    | 2  | 0.108    | 1.91    |  95.4%     |
| f    | 4  | 0.134    | 1.54    |  38.4%     |

---

## Analysis

### Correctness
Both implementations converge in exactly **8242 iterations** with identical
`max_diff` values at every checkpoint — confirms the two codes are numerically
equivalent.

### Serial performance (np=1)
Fortran is ~20% faster than C++ at np=1. The likely cause is memory layout:
Fortran is column-major, so the inner loop over rows (`i`) accesses `u(i,j)`
contiguously in memory. C++ is row-major, so the equivalent inner loop over
columns accesses `u[i*nc + j]` contiguously — but the Jacobi stencil accesses
both `u[i-1][j]` and `u[i+1][j]` (column neighbours), which are strided in
C++ and contiguous in Fortran. This gives Fortran a cache advantage in the
update loop.

### Scaling to np=2
Both scale well. Fortran maintains its serial advantage (108ms vs 149ms).
Fortran efficiency at np=2 is 95.4% vs C++ 86.6% — Fortran scales better here.

### Scaling to np=4
The trend reverses: C++ reaches 0.117s while Fortran slows to 0.134s.
C++ efficiency (55.1%) exceeds Fortran (38.4%). A likely explanation is the
halo exchange strategy: at np=4 each process has a smaller subdomain (64×64),
so communication overhead becomes relatively more significant. The derived
`MPI_Type_vector` used in Fortran for row exchange (strided in column-major
layout) may carry higher latency than the contiguous row sends in C++.

### Key takeaway
- **Fortran wins at low process counts** due to better cache behaviour in the
  stencil update.
- **C++ wins at np=4** due to more efficient halo communication layout.
- On a real cluster with np=8/16 and larger grids (N=512, N=1024), the
  communication overhead dominates and the two languages are expected to
  converge in performance.

---

## Key Differences Between C++ and Fortran Implementations

### Memory layout (the most important difference)

| Aspect | C++ | Fortran |
|--------|-----|---------|
| Array layout | **Row-major** | **Column-major** |
| Rows in memory | Contiguous | Strided |
| Columns in memory | Strided | Contiguous |
| Derived type needed for | Columns (`MPI_Type_vector`) | Rows (`MPI_Type_vector`) |
| East-West exchange | Uses `MPI_Type_vector` | Plain `MPI_DOUBLE_PRECISION` |
| North-South exchange | Plain `MPI_DOUBLE` | Uses `MPI_Type_vector` |

> This inversion is the critical point: **each language uses a derived MPI
> datatype for the direction that is strided in its own memory layout.**

### Array swap

| C++ | Fortran |
|-----|---------|
| `std::swap(u, u_new)` — O(1) pointer swap | `u = u_new` — full array copy |

In Fortran, O(1) swap is possible with pointer arrays:
```fortran
real(8), pointer :: u(:,:), u_new(:,:), tmp(:,:)
tmp => u;  u => u_new;  u_new => tmp
```

### Indexing

| C++ | Fortran |
|-----|---------|
| 0-based, manual `idx(i,j) = i*nc + j` | 1-based, native 2D `u(i,j)` |
| Ghost cells at rows 0 and nr-1 | Ghost cells at rows 0 and `local_rows+1` |

### Type strictness with `use mpi`

Fortran with `use mpi` enforces explicit interfaces — all logical MPI arguments
(`periods`, `reorder`) must be declared as `LOGICAL`, not `INTEGER`. Passing
`0`/`1` integers compiles with `include 'mpif.h'` but fails with `use mpi`.