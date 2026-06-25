# 04b — 2D Jacobi Solver with MPI

## The Physical Problem

We solve the **Poisson equation** on a square domain [0,1]×[0,1]:

```
-∇²u = f(x,y)    inside the domain
 u   = g(x,y)    on the boundary (Dirichlet conditions)
```

Setting `f=0` reduces it to the **Laplace equation**: the solution represents the
steady-state temperature distribution given fixed temperatures on the boundaries.

---

## The Jacobi Method

Finite difference scheme with a 5-point stencil:

```
         u[i-1][j]
             |
u[i][j-1] - u[i][j] - u[i][j+1]
             |
         u[i+1][j]

Update: u_new[i][j] = 0.25 * (u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1])
                               (for f=0)
```

The method iterates until the maximum change between successive solutions drops
below a threshold ε.

---

## Parallelization with MPI

### Domain Decomposition

The global grid (N×N) is divided into horizontal strips:

```
Process 0:   rows  0     ..  N/P-1    + ghost row below
Process 1:   rows  N/P   ..  2N/P-1  + ghost row above and below
...
Process P-1: last  N/P rows           + ghost row above
```

### Per-Iteration Algorithm

```
1. Halo exchange:     swap boundary rows with neighbouring processes
2. Update all cells:  u_new[i][j] = average(neighbours)
3. Compute local change: max|u_new - u_old|
4. MPI_Allreduce(MPI_MAX) → global maximum change
5. If change < ε → converged → exit
```

### Iteration Timeline

```
Iteration k:
  ├── MPI_Sendrecv  (North-South halo exchange)
  ├── Update local stencil
  ├── MPI_Allreduce (maximum change)
  └── Convergence check
```

---

## Exercises

### Exercise 1 — 1D Jacobi (strips) — `jacobi_1d_strips.cpp`

Simplified version: decomposition into horizontal strips, North-South exchange only.

### Exercise 2 — Full 2D Jacobi — `jacobi_2d_full.cpp`

Full 2D decomposition using a Cartesian topology, halo exchange on all four sides,
and non-trivial boundary conditions.

---

## Expected Output

### jacobi_1d_strips (4 processes, 64×64 grid)

```
[Iter    0] max change = 1.00000e+00
[Iter  100] max change = 1.23456e-02
[Iter  500] max change = 2.45e-04
[Iter 1847] CONVERGED! change = 9.87e-07 < eps = 1e-06
Total time: 0.342 seconds
```
