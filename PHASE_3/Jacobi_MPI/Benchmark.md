# Benchmark: Jacobi 2D Solver — C++ vs Fortran

Comparison of two equivalent MPI implementations of the 2D Jacobi heat diffusion solver with Cartesian topology decomposition.

---

## Files

| File | Language | Compiler |
|------|----------|----------|
| `jacobi_2d_full.cpp` | C++ | `mpicxx` |
| `jacobi_2d_full.f90` | Fortran 90 | `mpif90` |

---

## Compilation

```bash
# C++
mpicxx -O2 -Wall -o jacobi2d_cpp jacobi_2d_full.cpp

# Fortran
mpif90 -O2 -Wall -o jacobi2d_f jacobi_2d_full.f90
```

> Use the **same optimization flag** (`-O2`) for a fair comparison.
> To test also with `-O3`:
> ```bash
> mpicxx -O3 -o jacobi2d_cpp_O3 jacobi_2d_full.cpp
> mpif90 -O3 -o jacobi2d_f_O3  jacobi_2d_full.f90
> ```

---

## How to Run the Benchmark

### Single run

```bash
mpirun -np 4 ./jacobi2d_cpp
mpirun -np 4 ./jacobi2d_f
```

### Sweep over number of processes

```bash
for NP in 1 2 4 8 16; do
    echo "=== C++  np=$NP ==="
    mpirun -np $NP ./jacobi2d_cpp

    echo "=== Fortran np=$NP ==="
    mpirun -np $NP ./jacobi2d_f
done
```

### Automated timing script

Save as `run_benchmark.sh`:

```bash
#!/bin/bash
# run_benchmark.sh — Jacobi 2D benchmark: C++ vs Fortran

PROCS=(1 2 4 8 16)
OUTFILE="benchmark_results.csv"

echo "lang,np,time_s,iters,final_change" > $OUTFILE

for NP in "${PROCS[@]}"; do
    for LANG in cpp f; do
        BIN="./jacobi2d_${LANG}"
        OUTPUT=$(mpirun -np $NP $BIN 2>/dev/null)

        TIME=$(echo "$OUTPUT"  | grep "Total time"   | grep -oP '[0-9]+\.[0-9]+')
        ITERS=$(echo "$OUTPUT" | grep -oP '(?<=after )[0-9]+')
        CHANGE=$(echo "$OUTPUT"| grep "Final change" | grep -oP '[0-9.eE+\-]+' | tail -1)

        echo "${LANG},${NP},${TIME},${ITERS},${CHANGE}" >> $OUTFILE
        echo "[${LANG}] np=${NP}  time=${TIME}s  iters=${ITERS}  change=${CHANGE}"
    done
done

echo ""
echo "Results saved to $OUTFILE"
```

```bash
chmod +x run_benchmark.sh
./run_benchmark.sh
```

---

## Metrics to Compare

| Metric | How to measure |
|--------|----------------|
| **Wall-clock time** | `MPI_Wtime()` already in both codes |
| **Iterations to convergence** | Printed by both codes |
| **Speedup vs serial (np=1)** | `T(1) / T(np)` |
| **Parallel efficiency** | `speedup / np * 100%` |
| **Memory usage** | `valgrind --tool=massif` or `/usr/bin/time -v` |

---

## Key Differences Between C++ and Fortran Implementations

### Memory layout (the most important difference)

| Aspect | C++ | Fortran |
|--------|-----|---------|
| Array layout | **Row-major** | **Column-major** |
| Rows in memory | Contiguous | Strided |
| Columns in memory | Strided | Contiguous |
| Derived type needed for | Columns (`col_type`) | Rows (`col_type`) |
| East-West exchange | Uses `MPI_Type_vector` | Plain `MPI_DOUBLE_PRECISION` |
| North-South exchange | Plain `MPI_DOUBLE` | Uses `MPI_Type_vector` |

> This inversion is the critical point: **each language uses a derived MPI datatype for the direction that is strided in its own memory layout.**

### Array swap

| C++ | Fortran |
|-----|---------|
| `std::swap(u, u_new)` — O(1) pointer swap | `u = u_new` — full array copy |

In Fortran you can achieve O(1) swap with **pointer arrays**:
```fortran
real(8), pointer :: u(:,:), u_new(:,:), tmp(:,:)
! ...
tmp => u; u => u_new; u_new => tmp
```
This can give a measurable speedup for large grids.

### Indexing

| C++ | Fortran |
|-----|---------|
| 0-based, manual `idx(i,j) = i*nc + j` | 1-based, native 2D `u(i,j)` |
| Ghost cells at row 0 and row nr-1 | Ghost cells at row 0 and row local_rows+1 |

---

## Expected Results (reference, N=128, np=4)

> Values are indicative; actual numbers depend on hardware.

| np | C++ time (s) | Fortran time (s) |
|----|-------------|-----------------|
| 1  | ~X.XXX      | ~X.XXX          |
| 2  | ~X.XXX      | ~X.XXX          |
| 4  | ~X.XXX      | ~X.XXX          |
| 8  | ~X.XXX      | ~X.XXX          |

Fill in after running `run_benchmark.sh`.

---

## Speedup and Efficiency Table (fill in after runs)

| np | C++ speedup | C++ efficiency | Fortran speedup | Fortran efficiency |
|----|-------------|----------------|-----------------|-------------------|
| 1  | 1.00        | 100%           | 1.00            | 100%              |
| 2  |             |                |                 |                   |
| 4  |             |                |                 |                   |
| 8  |             |                |                 |                   |
| 16 |             |                |                 |                   |

Formula:
```
speedup(np)   = T(1) / T(np)
efficiency(np) = speedup(np) / np * 100
```

---

## SLURM Job Script (for HPC cluster)

```bash
#!/bin/bash
#SBATCH --job-name=jacobi_bench
#SBATCH --output=jacobi_%j.out
#SBATCH --ntasks=16
#SBATCH --time=00:10:00
#SBATCH --partition=regular
#SBATCH --account=corsohpc8

module load openmpi

cd $SLURM_SUBMIT_DIR

for NP in 1 2 4 8 16; do
    echo "--- C++ np=$NP ---"
    srun --ntasks=$NP ./jacobi2d_cpp

    echo "--- Fortran np=$NP ---"
    srun --ntasks=$NP ./jacobi2d_f
done
```

---

## What to Discuss in the Report

1. **Layout inversion**: why C++ needs `MPI_Type_vector` for columns and Fortran for rows.
2. **Array swap cost**: `std::swap` vs full copy in Fortran; impact at large N.
3. **Convergence equivalence**: both codes must reach the same number of iterations (verifies correctness).
4. **Scaling behaviour**: does efficiency degrade similarly in both languages?
5. **Compiler impact**: `-O2` vs `-O3`, and whether auto-vectorization differs.