# 03a — Jacobi Solver in MPI

> Reference chapter on the Jacobi iterative method applied in a distributed-memory environment. Assumes the previous chapters have been read (01a/01b — point-to-point communication, 02 — collective communication): here `MPI_Allreduce` and `MPI_Allgather` are reused without being redefined from scratch, while `MPI_Sendrecv` is introduced from scratch, as this is its first occurrence.
>
> This module presents two structurally distinct applications of the Jacobi method: the first applies the method to the numerical solution of a partial differential equation (PDE) through grid discretization, with **spatial** parallelization of the domain; the second applies the same method to a small, dense linear system, with **algebraic** parallelization at the level of the individual unknowns. The comparison between the two parallelization strategies is the central didactic-engineering objective of this chapter.

---

## Table of Contents

1. [The Jacobi method: general mathematical foundations](#1-the-jacobi-method-general-mathematical-foundations)
2. [Exercise 1 — Heat diffusion on a grid (1D strip Jacobi)](#2-exercise-1--heat-diffusion-on-a-grid-1d-strip-jacobi)
   - 2.1 [The physical problem and Laplace's equation](#21-the-physical-problem-and-laplaces-equation)
   - 2.2 [Finite-difference discretization: derivation of the stencil](#22-finite-difference-discretization-derivation-of-the-stencil)
   - 2.3 [Boundary conditions](#23-boundary-conditions)
   - 2.4 [Parallelization strategy: strip decomposition](#24-parallelization-strategy-strip-decomposition)
   - 2.5 [Ghost rows and halo exchange with MPI_Sendrecv](#25-ghost-rows-and-halo-exchange-with-mpi_sendrecv)
   - 2.6 [Global convergence criterion](#26-global-convergence-criterion)
   - 2.7 [Complete per-iteration algorithm](#27-complete-per-iteration-algorithm)
   - 2.8 [Compilation, execution, and expected output](#28-compilation-execution-and-expected-output)
3. [Exercise 2 — Jacobi on a 4×4 linear system](#3-exercise-2--jacobi-on-a-4×4-linear-system)
   - 3.1 [Algebraic formulation and convergence condition](#31-algebraic-formulation-and-convergence-condition)
   - 3.2 [Parallelization strategy: one unknown per process](#32-parallelization-strategy-one-unknown-per-process)
   - 3.3 [Communication pattern: why Allgather and not halo exchange](#33-communication-pattern-why-allgather-and-not-halo-exchange)
   - 3.4 [Complete per-iteration algorithm](#34-complete-per-iteration-algorithm)
   - 3.5 [Compilation, execution, and expected output](#35-compilation-execution-and-expected-output)
4. [Comparison between the two parallelization strategies](#4-comparison-between-the-two-parallelization-strategies)
5. [Common mistakes and how to avoid them](#5-common-mistakes-and-how-to-avoid-them)

---

## 1. The Jacobi method: general mathematical foundations

The Jacobi method is an iterative method for solving linear systems `Ax = b`, belonging to the family of methods based on the **splitting** of the coefficient matrix. `A` is decomposed as:

```
A = D - L - U
```

where `D` is the diagonal matrix containing the diagonal entries of `A`, `-L` is the strictly lower triangular part, and `-U` is the strictly upper triangular part (with a sign such that `L` and `U` have zero diagonal and contain the negatives of the off-diagonal entries). The system `Ax = b` is therefore rewritten as:

```
(D - L - U)x = b   ⟹   Dx = (L + U)x + b   ⟹   x = D⁻¹(L + U)x + D⁻¹b
```

This rewriting naturally suggests a fixed-point iterative scheme:

```
x^(k+1) = D⁻¹(L + U) x^(k) + D⁻¹b
```

where `x^(k)` denotes the vector of unknowns at the `k`-th iteration. The matrix `T = D⁻¹(L + U)` is called the **Jacobi iteration matrix**. Component by component, the update is written explicitly as:

```
x_i^(k+1) = ( b_i - Σ_{j≠i} a_ij · x_j^(k) ) / a_ii
```

that is: the `i`-th unknown is isolated using the `i`-th equation of the original system, substituting into all the other unknowns the values from the **previous** iteration (not the ones already updated in the current iteration — this is the fundamental distinction from the Gauss-Seidel method, which instead immediately reuses the most recently available values, but in doing so introduces a sequential dependency between components that makes the method intrinsically less parallelizable).

**Convergence condition.** The method converges, for any choice of the initial vector `x^(0)`, if and only if the spectral radius of the iteration matrix `T = D⁻¹(L + U)` is strictly less than 1:

```
ρ(T) = max_i |λ_i(T)| < 1
```

where `λ_i(T)` are the eigenvalues of `T`. Directly verifying this condition requires computing the eigenvalues, an expensive operation; however, there is a **sufficient** (but not necessary) condition that is simpler to check, that of **strict row-wise diagonal dominance**:

```
|a_ii| > Σ_{j≠i} |a_ij|    for every row i
```

If this condition holds for every row of the matrix, it can be shown that `ρ(T) < 1` (in the infinity norm, `‖T‖_∞ < 1`, which implies `ρ(T) ≤ ‖T‖_∞ < 1` by the general relationship between the spectral radius and any induced matrix norm), and the method therefore converges regardless of the starting point. Both exercises in this module exploit this property, in different forms: the first inherits it implicitly from the structure of the discrete Laplacian (5-point stencil, diagonal coefficient equal to the sum of the absolute values of the neighbor weights, with equality — not strict dominance — compensated by the Dirichlet boundary conditions, which fix known values at the boundary), the second verifies it explicitly on the system's 4×4 matrix (section 3.1).

**Practical stopping criterion.** Since theoretical convergence is asymptotic (the method reaches the exact solution only as `k → ∞`), in practice the iteration is stopped when a measure of the difference between successive iterations drops below a tolerance threshold `ε` fixed in advance:

```
‖x^(k+1) - x^(k)‖ < ε
```

Both exercises in this module use the infinity norm (maximum pointwise absolute difference) as the criterion, both for implementation simplicity and for its direct correspondence with the physical/numerical meaning of the monitored quantity.

## 2. Exercise 1 — Heat diffusion on a grid (1D strip Jacobi)

### 2.1 The physical problem and Laplace's equation

We solve **Laplace's equation** on a square domain `[0,1]×[0,1]`:

```
-∇²u = 0    inside the domain
 u   = g    on the boundary (Dirichlet conditions)
```

where `∇²u = ∂²u/∂x² + ∂²u/∂y²` is the Laplace operator (Laplacian) applied to the scalar field `u(x,y)`. This equation describes the temperature distribution in **steady state** (no time dependence: `∂u/∂t = 0`, a condition obtained as the limiting case of the parabolic heat equation `∂u/∂t = α∇²u` once the transient has died out) on a flat plate, given fixed temperature values on the boundary of the domain (**Dirichlet-type** boundary conditions, in which it is the value of the function, not its normal derivative, that is specified on the boundary).

The boundary conditions used in this exercise are:

```
Top boundary    (row 0):     u = 0.0  (cold)
Left boundary   (column 0):  u = 0.0
Right boundary  (column N-1): u = 0.0
Bottom boundary (row N-1):   u = 1.0  (hot)
```

Physically, this corresponds to a square plate with three sides held at temperature 0 and one side held at temperature 1: the steady-state solution `u(x,y)` represents the equilibrium temperature at every interior point of the domain, the result of heat diffusing from the hot boundary toward the rest of the plate.

### 2.2 Finite-difference discretization: derivation of the stencil

To solve the equation numerically, the continuous domain is discretized on a regular N×N grid of points, with grid spacing `h = 1/(N-1)` in both directions. The value `u(x,y)` is approximated by the discrete values `u[i][j] ≈ u(i·h, j·h)` at the grid nodes.

The second partial derivatives are approximated using the second-order **centered finite-difference** scheme:

```
∂²u/∂x² ≈ ( u[i-1][j] - 2·u[i][j] + u[i+1][j] ) / h²
∂²u/∂y² ≈ ( u[i][j-1] - 2·u[i][j] + u[i][j+1] ) / h²
```

This approximation is derived from the Taylor series expansion of `u` around the point `(i,j)` in the two directions, truncated at second order: summing the expansions of `u[i+1][j]` and `u[i-1][j]` (or analogously for the `j` direction), the odd-order terms cancel out by symmetry, leaving a local truncation error of `O(h²)`.

Substituting both approximations into the equation `-∇²u = 0`, i.e. `∂²u/∂x² + ∂²u/∂y² = 0`, gives:

```
( u[i-1][j] - 2u[i][j] + u[i+1][j] ) / h² + ( u[i][j-1] - 2u[i][j] + u[i][j+1] ) / h² = 0
```

Multiplying by `h²` and collecting the terms in `u[i][j]` (which appear with coefficient `-4`):

```
u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1] - 4·u[i][j] = 0
```

from which, isolating `u[i][j]`:

```
u[i][j] = 0.25 · ( u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1] )
```

This is exactly the fixed-point relation that the Jacobi method iterates, applied **locally** at every interior node of the grid: the value at each point, at convergence, is the arithmetic mean of the four cardinal neighbors (north, south, east, west). This 5-point access pattern (the node itself plus its 4 neighbors) is known as the **5-point stencil**:

```
             u[i-1][j]
                 |
u[i][j-1] --- u[i][j] --- u[i][j+1]
                 |
             u[i+1][j]

Update: u_new[i][j] = 0.25 · (u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1])
```

Note the direct correspondence with the general case from section 1: the matrix `A` implicit in this formulation (if the two-dimensional grid were "unrolled" into a one-dimensional vector of unknowns, as would be done for a direct solver) would have `-4` on the diagonal and `+1` in the four positions corresponding to the stencil's neighbors — a sparse, block-pentadiagonal matrix, with diagonal dominance that is not strict but is made effectively strict "almost everywhere" by the nodes adjacent to the boundary, where some of the four neighbors are actually known boundary values (constants), not unknowns.

The method iterates the update over all the **interior** nodes of the grid (the boundary nodes remain fixed at the Dirichlet conditions and are never updated) until the maximum pointwise change between two successive iterations drops below a threshold `ε`:

```
max_{i,j} |u_new[i][j] - u[i][j]| < ε
```

### 2.3 Boundary conditions

The boundary conditions are imposed by fixing the values of `u` on the boundary rows/columns of the global domain and **never updating them** during the iteration: row 0 (top boundary) and column 0/column N-1 (side boundaries) remain at `0.0`; the last row (bottom boundary) remains at `1.0`. Only the nodes strictly interior to the grid (`1 ≤ i ≤ N-2`, `1 ≤ j ≤ N-2` in global numbering) are subject to the stencil update described above.

### 2.4 Parallelization strategy: strip decomposition

The global N×N grid is partitioned into **contiguous horizontal strips of rows**, one per process, following a 1D **domain decomposition** scheme:

```
Process 0:    rows  0      ..  N/P-1     + south ghost row
Process 1:    rows  N/P    ..  2N/P-1    + north and south ghost rows
...
Process P-1:  rows (P-1)N/P .. N-1       + north ghost row
```

Each process holds in local memory its assigned rows **plus** one or two additional rows called **ghost rows**, which locally replicate the values of the boundary rows of the neighboring processes. This replication is necessary because the 5-point stencil, applied to a node on the upper or lower boundary of a process's local strip, requires the value of the north or south neighbor, which physically resides in the memory of **another** process. Without ghost rows, each process would have to perform a point-to-point communication for every boundary access at every single node update, with prohibitive communication overhead; with ghost rows, the communication is instead **batched**: a single pair of row exchanges per neighbor, for each complete iteration of the stencil over all the interior nodes of the local strip.

The process of rank `r` (with `0 < r < P-1`, i.e. not at the edges of the decomposition) has process `r-1` as its north neighbor and process `r+1` as its south neighbor. The processes of rank `0` and `P-1` have only one neighbor (only south and only north, respectively), since their opposite boundary rows coincide with the physical boundary of the global domain (a known Dirichlet condition, not a ghost row from other processes).

### 2.5 Ghost rows and halo exchange with MPI_Sendrecv

At every iteration, before it can apply the stencil to the boundary nodes of its own local strip, each process must update its ghost rows with the current values of the neighbors' boundary rows. This operation is known as **halo exchange**.

The typical implementation uses `MPI_Sendrecv`, a **blocking** primitive that combines a simultaneous send and receive into a single call, explicitly avoiding the deadlock risk described in chapter 01a (section 6) for symmetric send/receive patterns between neighboring processes:

```cpp
int MPI_Sendrecv(
    const void*  sendbuf,     // buffer to send to the neighbor
    int          sendcount,   // number of elements to send
    MPI_Datatype sendtype,
    int          dest,        // rank of the process to send to
    int          sendtag,     // tag of the outgoing message
    void*        recvbuf,     // buffer in which to receive from the neighbor
                               // (DISTINCT from sendbuf: MPI_Sendrecv is not in-place)
    int          recvcount,   // maximum capacity of the receive buffer
    MPI_Datatype recvtype,
    int          source,      // rank of the process to receive from
    int          recvtag,     // expected tag of the incoming message
    MPI_Comm     comm,
    MPI_Status*  status
);
```

The crucial point of `MPI_Sendrecv` is that the MPI implementation **internally manages** the ordering of the send and receive operations so as to avoid the deadlock that would arise from manually writing two consecutive blocking `MPI_Send` calls between processes exchanging data symmetrically (exactly the scenario discussed in chapter 01a, section 6.1). Semantically, `MPI_Sendrecv` is equivalent to performing an `MPI_Isend` and an `MPI_Irecv` followed by an `MPI_Waitall` on both (chapter 01b), but without having to explicitly manage `MPI_Request` handles: the management of the internal asynchrony is delegated entirely to the implementation.

In the context of halo exchange, each process performs **two** `MPI_Sendrecv` calls per iteration (one for the north neighbor, one for the south neighbor), each of which sends its own boundary row and simultaneously receives the corresponding ghost row:

```cpp
// Exchange with the south neighbor: I send my last local row,
// I receive its first local row into my south ghost row
MPI_Sendrecv(
    &u[last_local_row][0], N, MPI_DOUBLE, south_neighbor, TAG_S,
    &u[ghost_row_south][0], N, MPI_DOUBLE, south_neighbor, TAG_N,
    MPI_COMM_WORLD, MPI_STATUS_IGNORE
);

// Exchange with the north neighbor: I send my first local row,
// I receive its last local row into my north ghost row
MPI_Sendrecv(
    &u[first_local_row][0], N, MPI_DOUBLE, north_neighbor, TAG_N,
    &u[ghost_row_north][0], N, MPI_DOUBLE, north_neighbor, TAG_S,
    MPI_COMM_WORLD, MPI_STATUS_IGNORE
);
```

The processes at the edges of the decomposition (rank 0 and rank P-1), lacking a neighbor on one side, typically handle the absence of that neighbor with a conditional (`if (rank > 0) ...` / `if (rank < size-1) ...`) around the corresponding `MPI_Sendrecv`, or — a more elegant and less error-prone solution — by using `MPI_PROC_NULL` as the value of `dest`/`source` for the missing side: MPI calls to/from `MPI_PROC_NULL` are guaranteed by the standard to be no-ops (they return immediately with no effect), eliminating the need for explicit conditional logic in the communication code.

### 2.6 Global convergence criterion

Each process, after applying the Jacobi stencil to all the interior nodes of its own local strip, computes its own **local** maximum change:

```cpp
double local_max_change = 0.0;
for (/* every local interior node i,j */) {
    double diff = std::fabs(u_new[i][j] - u[i][j]);
    local_max_change = std::max(local_max_change, diff);
}
```

This local value, however, is not sufficient to decide the algorithm's global convergence: the stopping criterion requires the maximum over **all** the nodes of the domain, distributed across all the processes. `MPI_Allreduce` with the `MPI_MAX` operator (chapter 03, section 6) is therefore used to aggregate the local maxima into a single global maximum, made available to **all** processes (necessary because every process must be able to decide independently, with the same outcome, whether to exit the iteration loop — an `MPI_Reduce` that only notifies the root would require a subsequent `MPI_Bcast` to communicate the stopping decision to all the other processes):

```cpp
double global_max_change;
MPI_Allreduce(&local_max_change, &global_max_change, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

if (global_max_change < EPS) break;  // exit condition, identical on every process
```

### 2.7 Complete per-iteration algorithm

```
Iteration k:
  ├── MPI_Sendrecv × 2   (north-south halo exchange with neighboring processes)
  ├── Jacobi stencil update (local computation, interior nodes)
  ├── Compute local_max_change (local reduction, no communication)
  ├── MPI_Allreduce (MPI_MAX)  (global reduction, convergence criterion)
  └── std::swap(u, u_new)     (swap of pointers/buffers, no copying)
```

Note the use of `std::swap(u, u_new)` at the end of each iteration, instead of an element-by-element copy from `u_new` to `u`: since the Jacobi stencil (unlike Gauss-Seidel) explicitly requires that all updates within an iteration use only "frozen" values from the previous iteration, two distinct buffers (`u` and `u_new`) must be kept during the stencil computation, to prevent a node updated first from influencing the computation of a node updated later in the same iteration. Swapping the pointers to the two buffers, instead of copying their contents, is an elementary but performance-important optimization: it reduces the per-iteration cost from `O(local_rows × N)` copy operations to `O(1)` (a swap of two pointers).

### 2.8 Compilation, execution, and expected output

```bash
mpicxx -O2 -Wall -o jacobi1d jacobi_1d_strips.cpp
mpirun -np 4 ./jacobi1d
```

Expected output:

```text
[Iter  100] max change = 1.2346e-02
[Iter  200] max change = 6.2134e-03
...
✓ CONVERGENCE reached in 1847 iterations!
  Final change:      9.87e-07
  Total time:        0.342 seconds
  Processes used:    4
```

The `max change` value, printed periodically (typically every 100 iterations, so as not to flood the output with one line per iteration), is the result of the `MPI_Allreduce` from section 2.6: since it is identical on all processes at the end of the reduction, the periodic printing is typically conditioned on `if (rank == 0)` to avoid P identical redundant lines at every checkpoint. The decreasing monotonicity of the sequence (`1.2346e-02` → `6.2134e-03` → ...) is expected and consistent with the nature of the Jacobi method for this problem: since `ρ(T) < 1` (section 1), the error decreases geometrically at each iteration with an asymptotic factor equal to the spectral radius of the iteration matrix. The number of iterations needed to reach the threshold `ε` depends both on the grid size `N` (refining the grid worsens the convergence speed of the Jacobi method applied to the discrete Laplacian, since `ρ(T) → 1` as `h → 0`) and on the chosen value of `ε`.

## 3. Exercise 2 — Jacobi on a 4×4 linear system

### 3.1 Algebraic formulation and convergence condition

This exercise applies the Jacobi method described abstractly in section 1 to a dense linear system of small, fixed size (4 equations, 4 unknowns), solved **exactly** (up to the convergence tolerance) rather than through the discretization of a continuous problem. The system is:

```
 10 x₀ -  x₁ + 2 x₂          =   6
- x₀  + 11 x₁ -  x₂ + 3 x₃   =  25
 2 x₀ -  x₁ + 10 x₂ -  x₃    = -11
        3 x₁ -  x₂ + 8 x₃    =  15
```

Verification of strict row-wise diagonal dominance (sufficient convergence condition, section 1), row by row:

```
Row 0: |10| > |-1| + |2|           →  10 > 3    ✓
Row 1: |11| > |-1| + |-1| + |3|    →  11 > 5    ✓
Row 2: |10| > |2| + |-1| + |-1|    →  10 > 4    ✓
Row 3: |8|  > |3| + |-1|           →   8 > 4    ✓
```

All rows satisfy the condition, guaranteeing `ρ(D⁻¹(L+U)) < 1` and therefore convergence of the method regardless of the chosen initial vector (typically `x^(0) = 0` in the absence of a better estimate).

Isolating each unknown on its own equation (equivalent to writing out `x = D⁻¹(L+U)x + D⁻¹b` component by component, section 1):

```
x₀^(k+1) = ( 6  + x₁^(k) - 2x₂^(k)          ) / 10
x₁^(k+1) = ( 25 + x₀^(k) + x₂^(k) - 3x₃^(k) ) / 11
x₂^(k+1) = (-11 - 2x₀^(k) + x₁^(k) + x₃^(k) ) / 10
x₃^(k+1) = ( 15 - 3x₁^(k) + x₂^(k)          ) / 8
```

Note, comparing with section 2.2, that the structure is identical in nature (each unknown is a linear function of the others, weighted by the off-diagonal coefficients and normalized by the diagonal coefficient), but here the matrix is **dense** (nearly all off-diagonal coefficients are non-zero) rather than sparse with regular structure as in the case of the 5-point stencil: each unknown depends, in general, on **all** the others, not just on a small subset of topological "neighbors".

### 3.2 Parallelization strategy: one unknown per process

Unlike exercise 1, where the parallelization is **spatial** (each process owns a contiguous portion of the physical domain), here the parallelization is **algebraic**: each process is responsible for computing a single unknown of the system, independently of any notion of spatial proximity (which, in an abstract dense linear system, is not even defined):

```
Process 0 → computes x₀_new
Process 1 → computes x₁_new
Process 2 → computes x₂_new
Process 3 → computes x₃_new
```

This 1:1 correspondence between unknowns and processes imposes a rigid constraint on the number of processes with which the executable can be launched: **exactly 4**, equal to the size of the system. Unlike exercise 1, where the strip decomposition naturally adapts to an arbitrary number of processes P (provided `P ≤ N`, and ideally `N` divisible by `P` to balance the load), here the degree of parallelism is structurally limited by the size of the problem itself: it makes no sense to launch this executable with a number of processes other than 4, either more (there would be no additional unknowns to assign) or fewer (there would not be enough processes to cover all the unknowns, under the implementation assumption of 1 process = 1 unknown used here).

Each process typically determines which equation/unknown it is responsible for through a `switch(rank)` or an equivalent conditional structure, since the coefficients of each equation are structurally different (there is no uniform closed-form formula parameterized solely by the rank, unlike exercise 1 where the exact same stencil is applied by every process, with only the range of rows changing):

```cpp
double x_new;
switch (rank) {
    case 0: x_new = ( 6.0  + x[1] - 2.0*x[2]          ) / 10.0; break;
    case 1: x_new = ( 25.0 + x[0] + x[2] - 3.0*x[3]    ) / 11.0; break;
    case 2: x_new = (-11.0 - 2.0*x[0] + x[1] + x[3]    ) / 10.0; break;
    case 3: x_new = ( 15.0 - 3.0*x[1] + x[2]           ) / 8.0;  break;
}
```

### 3.3 Communication pattern: why Allgather and not halo exchange

In exercise 1, each process needs, in order to update its own nodes, exclusively the values held by the **two** topologically adjacent processes (north and south): the communication pattern is **local**, and the volume of data exchanged per process per iteration is constant with respect to the total number of processes P (it depends only on the grid width N, not on P). This is why P2P halo exchange (`MPI_Sendrecv`) is the appropriate choice: it would make no sense, nor would it be efficient, to force a collective communication when each process structurally interacts with only a small, fixed subset of the others.

In exercise 2, by contrast, the update formula for **each** unknown (section 3.1) involves, in general, **all** the other unknowns of the system (dense matrix, section 3.1): process 0, to compute `x₀_new`, needs the current values of `x₁` and `x₂` (held by other processes); process 1 needs `x₀`, `x₂`, and `x₃`; and so on. The data dependency pattern is therefore **complete** (all-to-all): every process must make its updated value available to **all** the other processes before the next iteration, and conversely must receive the updated value from all the others.

Implementing this pattern with explicit P2P communications would require, for each process, sending its own value to each of the other P-1 processes (and the symmetric receive), for a total of P·(P-1) P2P messages in the communicator — exactly the general pattern that the `MPI_Alltoall` collective primitive (chapter 03, section 7) is designed to handle efficiently. In this specific case, however, every process sends the **same** scalar value (its own `x_new`) to all the others, rather than distinct values for each recipient as in the general case of `MPI_Alltoall`: this is exactly the semantics of `MPI_Allgather` (chapter 03, section 5), which is therefore the correct and most efficient collective primitive for this pattern, preferable both to an `MPI_Alltoall` (semantically more general than necessary) and to a manually written sequence of P2P communications:

```cpp
double x_new;   // value computed locally by THIS process (section 3.2)
double x[4];    // complete vector of unknowns, replicated on EVERY process

MPI_Allgather(&x_new, 1, MPI_DOUBLE, x, 1, MPI_DOUBLE, MPI_COMM_WORLD);
// After this call, x[i] on EVERY process contains the value x_new
// computed by the process of rank i, for i = 0..3
```

At the end of the `MPI_Allgather`, every process holds an identical, complete local copy of the updated vector `x[]`, needed to compute its own unknown at the next iteration (which, as seen in section 3.1, in general depends on all the components of the vector).

### 3.4 Complete per-iteration algorithm

```
Iteration k:
  ├── Each process computes its own x_new (switch(rank), section 3.2)
  ├── Compute local diff = |x_new - x_old|            (no communication)
  ├── MPI_Allreduce (MPI_MAX)  → global max_diff       (convergence criterion)
  ├── MPI_Allgather            → distribution of x_new to all processes
  └── Check: if max_diff < ε, exit the loop (identical on every process)
```

Note that, unlike exercise 1 where `MPI_Allreduce` is the only collective involved, here **two** distinct collectives are needed per iteration: `MPI_Allreduce` for the global stopping criterion (exactly the same role as in section 2.6, applied here to a single scalar per process rather than to a maximum over a grid strip) and `MPI_Allgather` for redistributing the vector of unknowns (which has no direct analogue in exercise 1: there, the necessary "redistribution" is only toward the two topological neighbors, covered by P2P halo exchange, not by a collective).

```cpp
double diff = std::fabs(x_new - x[rank]);
double max_diff;
MPI_Allreduce(&diff, &max_diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

MPI_Allgather(&x_new, 1, MPI_DOUBLE, x, 1, MPI_DOUBLE, MPI_COMM_WORLD);

if (max_diff < EPS) break;
```

The relative order of the two calls should be noted: the computation of `diff` compares `x_new` (the local value just computed by this process) with `x[rank]` (the value of the same unknown from the previous iteration, still present in the vector `x[]` before the `MPI_Allgather` overwrites it) — it is therefore necessary to compute `diff` **before** the `MPI_Allgather` that updates `x[]`, otherwise the comparison would incorrectly be between `x_new` and itself.

### 3.5 Compilation, execution, and expected output

```bash
mpicxx -O2 -Wall -o jacobi_ls Jacobi_linear_system.cpp
mpirun -np 4 ./jacobi_ls
```

> ⚠️ This exercise requires **exactly 4 processes** (one per unknown), for the structural reasons discussed in section 3.2. Launching the executable with a different number of processes produces behavior undefined by the program itself (typically an access outside the cases handled by the `switch(rank)`, or incorrect sizing of the `MPI_Allgather` buffers), not an error automatically detected by MPI at runtime.

The program writes the solution to `output.dat` (handled by process 0, the sole process responsible for file I/O, to avoid concurrent writes to the same file from multiple processes):

```text
x = 1.00...
y = 2.00...
z = -1.00...
t = 1.00...
N of iterations : 26
```

The exact solution of the system is `x₀=1, x₁=2, x₂=-1, x₃=1`, verifiable by direct substitution into the four original equations of section 3.1. The number of iterations needed for convergence (`26`) is markedly lower than the typical number of iterations in exercise 1 (on the order of thousands): this is expected and consistent with the nature of the two problems, not an indication of a more efficient implementation. The 4×4 system has a markedly smaller spectral radius `ρ(T)` of the iteration matrix ("abundant" diagonal dominance, see the margins computed in section 3.1: for example row 3, `8 > 4`, a 100% margin over the dominance threshold) compared to the discretized Laplacian of exercise 1, whose spectral radius approaches 1 the finer the grid is (section 2.8): the geometric convergence speed of the Jacobi method is directly governed by `ρ(T)`, not by the nominal size of the problem (4 unknowns vs. N² unknowns).

## 4. Comparison between the two parallelization strategies

| Aspect | Exercise 1 (grid) | Exercise 2 (linear system) |
|---|---|---|
| Nature of the partitioning | Spatial (domain decomposition) | Algebraic (decomposition by unknown) |
| Scalability in the number of processes | Arbitrary, `P ≤ N` (ideally `N` divisible by `P`) | Fixed, `P` must equal the size of the system |
| Data dependencies for the local update | Only from the immediate topological neighbors (5-point stencil → 2 neighbors in 1D strips) | From all the other unknowns (dense matrix) |
| Communication pattern | Targeted P2P (halo exchange, `MPI_Sendrecv`) toward a constant number of neighbors | Collective (`MPI_Allgather`) toward all processes |
| Collectives used | Only `MPI_Allreduce` (convergence criterion) | `MPI_Allreduce` (convergence) + `MPI_Allgather` (data redistribution) |

The underlying conceptual point, useful for generalizing beyond these two specific exercises: the choice between targeted P2P communication and collective communication is not arbitrary, but **follows directly from the structure of the algorithm's data dependencies**. When the dependencies are **local/sparse** (each computational unit depends only on a small, fixed subset of other units, typically due to spatial or topological proximity), targeted P2P communication toward only the relevant neighbors is the efficient choice, since it avoids involving processes that have no mutual data dependency. When the dependencies are **global/dense** (each computational unit depends, in general, on all the others), a collective communication involving the entire communicator is instead the correct choice, because the equivalent P2P communication pattern would in any case degenerate into a complete all-to-all, which the collective primitives implement more efficiently than an equivalent hand-written sequence of P2P calls (chapter 03, section 9).

## 5. Common mistakes and how to avoid them

| Mistake | Typical cause | How to avoid it |
|---|---|---|
| Deadlock or corrupted values in the ghost rows | Halo exchange implemented with two separate blocking `MPI_Send`/`MPI_Recv` calls, ordered asymmetrically between neighboring processes (the same scenario as chapter 01a, section 6), instead of using `MPI_Sendrecv` | Use `MPI_Sendrecv` for halo exchange, which internally manages the safe ordering of simultaneous send/receive; alternatively, explicitly order send/recv as described in 01a section 6.2 |
| Crash due to out-of-bounds array access on the processes at the edges of the decomposition (rank 0 or rank P-1) | Halo exchange code that unconditionally assumes the existence of both the north and south neighbors, without handling the edge case of boundary processes | Guard the calls relating to the missing neighbor with a conditional on the rank, or use `MPI_PROC_NULL` as the substitute rank for the non-existent neighbor (section 2.5) |
| The global convergence criterion never triggers, or triggers inconsistently on different processes | Use of `MPI_Reduce` (root only) instead of `MPI_Allreduce` for the stopping criterion, with the loop-exit decision conditioned only on the root process, while the other processes keep iterating indefinitely, waiting for communications that no longer arrive | Always use `MPI_Allreduce` (not `MPI_Reduce`) when the decision to terminate the loop must be made identically and independently by every process (section 2.6) |
| Exercise 2 launched with a number of processes other than 4 produces meaningless results or silent crashes | The "one process per unknown" constraint (section 3.2) is not explicitly verified by the program at runtime | Add, during development, an explicit check at the start of the program (`if (size != 4) { ...terminate with an error message... }`) instead of relying on documentation alone |
| Concurrent writing to `output.dat` from multiple processes, with corrupted or truncated file contents | Every process attempts to write the full output to file, instead of a single designated process | Delegate full responsibility for file output to a single process (typically rank 0), guarding the write block with `if (rank == 0) { ... }` |
