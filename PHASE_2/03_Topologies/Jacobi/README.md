# 04b — Jacobi Solver with MPI

This module presents two complementary applications of the Jacobi iterative method
in a distributed-memory environment. The two exercises differ in both the physical
problem and the parallelization strategy, making them a useful pair for understanding
how MPI communication patterns depend on the structure of the algorithm.

---

## Exercise 1 — Jacobi 1D Strips: Heat Diffusion on a Grid

### The Physical Problem

We solve the **Laplace equation** on a square domain [0,1]×[0,1]:

```
-∇²u = 0    inside the domain
 u   = g    on the boundary (Dirichlet conditions)
```

The solution represents the steady-state temperature distribution given fixed
temperatures on the boundaries. The boundary conditions used here are:

```
Top boundary    (row 0):   u = 0.0  (cold)
Left boundary   (col 0):   u = 0.0
Right boundary  (col N-1): u = 0.0
Bottom boundary (row N-1): u = 1.0  (hot)
```

### The Jacobi Stencil

Finite difference discretization with a 5-point stencil:

```
         u[i-1][j]
             |
u[i][j-1] - u[i][j] - u[i][j+1]
             |
         u[i+1][j]

Update: u_new[i][j] = 0.25 * (u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1])
```

The method iterates until the maximum pointwise change drops below a threshold ε.

### Parallelization Strategy: Domain Decomposition

The global N×N grid is divided into horizontal strips, one per process:

```
Process 0:   rows  0      ..  N/P-1    + south ghost row
Process 1:   rows  N/P    ..  2N/P-1  + north and south ghost rows
...
Process P-1: last  N/P rows            + north ghost row
```

Each process holds its local rows plus **ghost rows** — extra rows that store the
boundary values of the neighboring processes, updated at each iteration via halo
exchange.

### Per-Iteration Algorithm

```
1. Halo exchange     → MPI_Sendrecv with north and south neighbors
2. Jacobi update     → u_new[i][j] = 0.25 * sum(neighbors)
3. Local change      → max|u_new - u_old| over local rows
4. Global change     → MPI_Allreduce(MPI_MAX) across all processes
5. Convergence check → exit if global_change < EPS
```

### Iteration Timeline

```
Iteration k:
  ├── MPI_Sendrecv  (North-South halo exchange)
  ├── Jacobi stencil update (local computation)
  ├── MPI_Allreduce (global convergence check)
  └── std::swap(u, u_new)
```

### Files

- `jacobi_1d_strips.cpp` — full implementation

### Compilation and Execution

```bash
mpicxx -O2 -Wall -o jacobi1d jacobi_1d_strips.cpp
mpirun -np 4 ./jacobi1d
```

### Expected Output

```
[Iter  100] max change = 1.2346e-02
[Iter  200] max change = 6.2134e-03
...
✓ CONVERGENCE reached in 1847 iterations!
  Final change:      9.87e-07
  Total time:        0.342 seconds
  Processes used:    4
```

---

## Exercise 2 — Jacobi on a Linear System

### The Problem

This exercise applies the Jacobi iterative method to solve a **4×4 linear system**
Ax = b, where each process is responsible for computing one unknown.

The system solved is:

<div align="center">

$$
\begin{cases}
10 x_0 - x_1 + 2 x_2 = 6 \\
-x_0 + 11 x_1 - x_2 + 3 x_3 = 25 \\
2 x_0 - x_1 + 10 x_2 - x_3 = -11\\
3 x_1 - x_2 + 8 x_3 = 15
\end{cases}
$$

</div>

The Jacobi update isolates each unknown on the diagonal:

```
x₀_new = ( 6     +  x₁ - 2x₂      ) / 10
x₁_new = ( 25    +  x₀ +  x₂ - 3x₃) / 11
x₂_new = (-11    - 2x₀ +  x₁ +  x₃) / 10
x₃_new = ( 15    - 3x₁ +  x₂      ) /  8
```

The method converges because the matrix is **strictly diagonally dominant**
(the diagonal entry is larger in absolute value than the sum of all other entries
in the same row).

### Parallelization Strategy: One Unknown per Process

Unlike the grid-based Jacobi, here the parallelization is at the level of the
**unknowns**, not the spatial domain:

```
Process 0 → computes x₀_new
Process 1 → computes x₁_new
Process 2 → computes x₂_new
Process 3 → computes x₃_new
```

Each process needs the current values of **all** unknowns to compute its update,
so after each iteration every process must share its new value with all others.

### Per-Iteration Algorithm

```
1. Each process computes its own x_new (using switch(rank))
2. Compute local diff = |x_new - x_old|
3. MPI_Allreduce(MPI_MAX) → global maximum diff across all processes
4. MPI_Allgather         → distribute all x_new values to all processes
5. Convergence check     → exit if max_diff < epsilon
```

### Key MPI Operations

**`MPI_Allreduce`** is used to find the maximum change across all processes — this
determines whether the iteration has converged:

```cpp
MPI_Allreduce(&diff, &max_diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
```

**`MPI_Allgather`** distributes the newly computed value from each process to all
others, so that every process has the full updated vector `x[]` before the next
iteration:

```cpp
MPI_Allgather(&x_new, 1, MPI_DOUBLE, x, 1, MPI_DOUBLE, MPI_COMM_WORLD);
```

The combination of these two collectives replaces what in the grid-based Jacobi
was handled by point-to-point halo exchange — here the communication pattern is
fully collective because every process needs data from every other process.

### Files

- `Jacobi_linear_system.cpp` — full implementation
- `output.dat` — solution written by process 0 after convergence

### Compilation and Execution

```bash
mpicxx -O2 -Wall -o jacobi_ls Jacobi_linear_system.cpp
mpirun -np 4 ./jacobi_ls
```

> This exercise requires **exactly 4 processes** (one per unknown).

### Expected Output

The program writes the solution to `output.dat`:

```
x = 1.00...
y = 2.00...
z = -1.00...
t = 1.00...
N of iterations : 26
```

The exact solution of the system is x₀=1, x₁=2, x₂=-1, x₃=1.

---
