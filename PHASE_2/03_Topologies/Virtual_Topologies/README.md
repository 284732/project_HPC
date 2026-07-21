# 03b — Virtual Topologies in MPI (Cartesian Grid)

> Reference chapter on Cartesian virtual topologies in MPI. Assumes the previous chapters have been read (01a/01b — point-to-point communication, 02 — collective communication, 03a — Jacobi solver, where `MPI_Sendrecv` and the concept of halo exchange were already introduced in detail). Here `MPI_Sendrecv` is reused without being redefined.

---

## Table of Contents

1. [What a virtual topology is and why it exists](#1-what-a-virtual-topology-is-and-why-it-exists)
2. [MPI_Cart_create: creating the Cartesian grid](#2-mpi_cart_create-creating-the-cartesian-grid)
3. [Correspondence between rank and coordinates: row-major ordering](#3-correspondence-between-rank-and-coordinates-row-major-ordering)
4. [MPI_Cart_coords and MPI_Cart_rank](#4-mpi_cart_coords-and-mpi_cart_rank)
5. [MPI_Cart_shift: finding neighbors](#5-mpi_cart_shift-finding-neighbors)
6. [MPI_Dims_create: automatic balancing of dimensions](#6-mpi_dims_create-automatic-balancing-of-dimensions)
7. [Guided exercises](#7-guided-exercises)
8. [Expected output and how to interpret it](#8-expected-output-and-how-to-interpret-it)
9. [Common mistakes and how to avoid them](#9-common-mistakes-and-how-to-avoid-them)

---

## 1. What a virtual topology is and why it exists

A **virtual topology** in MPI is a logical structure (grid, torus, generic graph) that is overlaid onto the set of processes of a communicator, associating each rank with a position within this structure, in addition to the simple sequential integer `0..P-1` already available in an ordinary communicator.

It is essential to clarify right away what a virtual topology **is not**: it does not modify the underlying physical hardware in any way, it does not move processes between compute nodes, and it does not by itself alter communication performance. The term "virtual" is chosen precisely in contrast to "physical": the topology is a purely logical abstraction, built on top of an existing communicator, that simplifies the way the programmer expresses and reasons about adjacency relationships between processes.

The concrete advantages offered by a Cartesian virtual topology are three, each distinct from the others:

* **Simplified neighbor management.** In a 2D grid domain decomposition problem (for example a two-dimensional generalization of the Jacobi problem covered in chapter 04a, where the decomposition there was 1D by strips), each process must determine the ranks of its north/south/east/west neighbors. Without a virtual topology, this requires explicit arithmetic computations on the linear rank (typically based on integer division and modulo), repeated and error-prone at every point in the code where they are needed; with `MPI_Cart_shift` (section 5), this computation is delegated to a single library call, with automatic handling even of edge cases (grid boundaries, periodicity).
* **Code readability and maintainability.** Expressing a process's position as `(row, column)` coordinates rather than as a single rank integer makes the code directly traceable to the logical structure of the problem being parallelized, reducing the likelihood of indexing errors in manual neighbor-rank computations.
* **Potential for physical mapping optimization.** The `reorder` parameter of `MPI_Cart_create` (section 2) allows the MPI implementation to **reassign** ranks within the new Cartesian communicator, potentially in a way that makes the requested logical topology coincide with the actual physical topology of the network interconnect (for example a physical torus network, common in many HPC systems). This optimization is **optional and implementation-dependent**.

## 2. MPI_Cart_create: creating the Cartesian grid

```cpp
int MPI_Cart_create(
    MPI_Comm  comm_old,    // starting communicator (typically MPI_COMM_WORLD)
                            // from which to derive the Cartesian topology
    int       ndims,       // number of dimensions of the grid (2 for a
                            // 2D grid, 3 for a 3D grid, etc.)
    const int dims[],      // size of each axis: dims[0] = number of
                            // rows, dims[1] = number of columns (for ndims=2).
                            // The product of all dims[i] must NOT exceed
                            // the number of processes in comm_old
    const int periods[],   // periodicity for each dimension: periods[i]=1
                            // makes the i-th axis periodic (toroidal topology,
                            // the neighbor "beyond" the last element is the first),
                            // periods[i]=0 makes the axis have a fixed boundary
                            // (processes at the edges have no neighbor on that side)
    int       reorder,     // reorder=1: the MPI implementation MAY reassign
                            // ranks in the new communicator to optimize the
                            // physical mapping (section 1); reorder=0: the ranks
                            // in the new communicator remain identical to those in
                            // comm_old, in the same order
    MPI_Comm* comm_cart    // OUTPUT: new communicator with the associated
                            // Cartesian topology, distinct from comm_old
);
```

Some relevant technical clarifications, often a source of errors for those using this function for the first time:

* If the product `dims[0] × dims[1] × ... × dims[ndims-1]` is **strictly less than** the number of processes available in `comm_old`, the "excess" processes (those whose original rank finds no place in the grid) receive `MPI_COMM_NULL` as the value of `comm_cart` and do not participate in the resulting topology: it is the programmer's responsibility to verify that the number of launched processes matches exactly (or is explicitly handled for the excess case) the product of the requested dimensions.
* If the product of the dimensions **exceeds** the number of available processes, the call is an MPI error (typically resulting in program termination with an error code, depending on the configured error handling).
* `MPI_Cart_create` is itself a **collective operation** (implicitly, given the way communicators are constructed in MPI): it must be invoked by all the processes in `comm_old`, with `ndims`, `dims[]`, and `periods[]` arguments consistent across all callers, for the same consistency reason discussed for generic collectives in chapter 02 (section 1).
* The returned `comm_cart` communicator is a new communicator, **distinct** from `comm_old`: subsequent communication operations (P2P or collective) must be addressed to `comm_cart` if one wishes to operate within the grid's group of processes (which coincides with that of `comm_old` except for the excess-process case discussed above), while `comm_old` remains valid and independently usable.

## 3. Correspondence between rank and coordinates: row-major ordering

When `MPI_Cart_create` builds the grid (with `reorder=0`, the simplest case to reason about explicitly), it assigns the Cartesian coordinates a **row-major** ordering relative to the original linear rank: the rank increases by first moving along the last dimension (the columns, for a 2D grid), and only moves to the next row once each row is finished — exactly the same row-major storage convention used for multidimensional arrays in C/C++.

For a 4×3 grid (`dims = {4, 3}`, i.e. 4 rows and 3 columns, for a total of 12 processes), the correspondence between the linear rank and the `(row, column)` coordinates is:

```
             col 0      col 1      col 2
row 0    [P0 (0,0)] [P1 (0,1)] [P2 (0,2)]
row 1    [P3 (1,0)] [P4 (1,1)] [P5 (1,2)]
row 2    [P6 (2,0)] [P7 (2,1)] [P8 (2,2)]
row 3    [P9 (3,0)] [P10(3,1)] [P11(3,2)]
```

The general relationship, for a grid with `dims[1]` columns, is:

```
rank = coords[0] * dims[1] + coords[1]
```

which is exactly the linearization formula for a 2D array stored in row-major order, where `coords[0]` is the row index and `coords[1]` is the column index. This relationship is the one implemented internally by `MPI_Cart_rank` (coordinates → rank conversion) and its inverse by `MPI_Cart_coords` (rank → coordinates), described in the following section. It should be emphasized that this explicit correspondence holds when `reorder=0`: with `reorder=1`, the MPI implementation may assign ranks in the grid according to a different order (implementation-dependent, oriented toward the physical mapping optimization discussed in section 1), and in that case the programmer must not rely on an explicit row-major ordering based on the original rank — instead, `MPI_Cart_coords`/`MPI_Cart_rank` must always be queried to obtain the actual correspondence, rather than computing it manually.

## 4. MPI_Cart_coords and MPI_Cart_rank

These two functions are the inverse of one another, and allow conversion between the "linear rank" representation and the "Cartesian coordinates" representation of a process within the topology.

```cpp
int MPI_Cart_coords(
    MPI_Comm comm_cart,  // the communicator with the Cartesian topology
    int      rank,       // the rank (in comm_cart) whose coordinates are wanted
    int      ndims,      // number of dimensions of the grid (must match
                          // the one used in MPI_Cart_create)
    int      coords[]    // OUTPUT: array of ndims integers, filled with the
                          // coordinates of the process with the specified rank
);

int MPI_Cart_rank(
    MPI_Comm comm_cart,   // the communicator with the Cartesian topology
    const int coords[],   // the coordinates whose corresponding rank is wanted
    int*     rank         // OUTPUT: the rank corresponding to those coordinates
);
```

A typical use is to obtain one's own coordinates at the start of the program, immediately after creating the topology:

```cpp
int rank, coords[2];
MPI_Comm_rank(comm_cart, &rank);           // rank of THIS process in comm_cart
MPI_Cart_coords(comm_cart, rank, 2, coords); // its own coordinates (row, column)
```

Note that `MPI_Cart_coords` accepts an **arbitrary** rank as a parameter, not necessarily that of the calling process: it is therefore possible, from any process, to query the coordinates of another process whose rank is known, useful for example for diagnostic or centralized logging purposes. Similarly, `MPI_Cart_rank` can be invoked to determine the rank corresponding to arbitrary coordinates, even ones not corresponding to the calling process — an operation particularly useful when a communication needs to be explicitly addressed to a process identified by its logical position in the grid rather than by rank, for example "the process in the opposite corner of the grid" or "the process in the same column but row 0".

## 5. MPI_Cart_shift: finding neighbors

`MPI_Cart_shift` is the central function for neighbor management in a Cartesian topology: for the calling process, it computes the ranks of the neighbors along a specific dimension and in both directions (forward and backward) relative to that dimension, in a single call.

```cpp
int MPI_Cart_shift(
    MPI_Comm comm_cart,        // the communicator with the Cartesian topology
    int      direction,        // the axis along which to look for neighbors:
                                // 0 → vertical (along the rows, i.e.
                                //     varying coords[0]),
                                // 1 → horizontal (along the columns, i.e.
                                //     varying coords[1])
    int      disp,             // displacement to apply:
                                // disp=+1 → "forward" neighbor along the axis
                                //     (coordinate incremented by 1),
                                // disp=-1 → "backward" neighbor along the axis
                                //     (coordinate decremented by 1).
                                // Values |disp|>1 are allowed by the standard
                                // and return the neighbor at distance disp,
                                // but the most common use is ±1 for immediate neighbors
    int*     rank_source,      // OUTPUT: the rank of the process from which THIS
                                // process would receive, if it performed a
                                // shift in the direction OPPOSITE to disp
                                // (useful to pass directly as 'source'
                                // in a subsequent MPI_Sendrecv)
    int*     rank_destination  // OUTPUT: the rank of the process to which THIS
                                // process would send, by shifting in the
                                // direction indicated by disp (useful as
                                // 'dest' in an MPI_Sendrecv)
);
```

The conceptually trickiest point of this function, often misunderstood, is the relationship between `disp` and the two outputs `rank_source`/`rank_destination`: they do **not** simply return "the forward neighbor" and "the backward neighbor" symmetrically and independently, but are explicitly designed to be passed **directly** as the `source` and `dest` parameters of a single `MPI_Sendrecv` call (chapter 03a, section 2.5), achieving a bidirectional data exchange along that direction in a single step:

```cpp
int rank_source, rank_dest;
MPI_Cart_shift(comm_cart, /*direction=*/0, /*disp=*/+1, &rank_source, &rank_dest);

// rank_dest   = the neighbor to send my data to (one step forward, disp=+1)
// rank_source = the neighbor to receive data from (one step backward relative to me,
//               i.e. the process for which I AM the "forward" neighbor)

MPI_Sendrecv(
    my_data, count, MPI_DOUBLE, rank_dest,   TAG,
    recv_buf, count, MPI_DOUBLE, rank_source, TAG,
    comm_cart, MPI_STATUS_IGNORE
);
```

To obtain both the north and south neighbor (or east/west) of a process, **two separate calls** to `MPI_Cart_shift` are therefore needed, one with `disp=+1` and one with `disp=-1` on the same `direction`, exactly as in the example table in the next section.

**Behavior at the grid boundaries.** If `periods[direction]=0` (non-periodic axis) and the requested shift would go beyond the limits of the grid (for example, asking for the "north" neighbor of a process already on the first row), the function does not raise an error: it returns the special constant `MPI_PROC_NULL` for the corresponding output. As already mentioned in chapter 03a (section 2.5), `MPI_PROC_NULL` is a "dummy" rank to/from which every MPI communication operation (including `MPI_Sendrecv`) is guaranteed by the standard to be an immediate no-op, eliminating the need for explicit conditional logic in the application code to handle the boundary case. If instead `periods[direction]=1` (periodic axis), the shift automatically "wraps around" the grid (toroidal topology): the "north" neighbor of the process on the first row is the process on the last row of the same column, and the function returns that rank instead of `MPI_PROC_NULL`.

### Example: the four neighbors of process P4 in a 4×3 grid

For the process of rank 4, coordinates `(1,1)`, in the 4×3 grid from section 3:

```
NORTH (direction=0, disp=-1): P1  (rank=1, coords=(0,1))
SOUTH (direction=0, disp=+1): P7  (rank=7, coords=(2,1))
WEST  (direction=1, disp=-1): P3  (rank=3, coords=(1,0))
EAST  (direction=1, disp=+1): P5  (rank=5, coords=(1,2))
```

Note the correspondence: `direction=0` acts on the first coordinate (`coords[0]`, the row), thus producing neighbors in the vertical direction (north/south); `direction=1` acts on the second coordinate (`coords[1]`, the column), producing neighbors in the horizontal direction (east/west). This `direction ↔ geometric axis` correspondence is an application-level convention (north/south/east/west are labels chosen by the programmer based on how they interpret the grid's two dimensions), not a constraint imposed by the MPI standard, which treats `direction` simply as the index of one of the grid's `ndims` abstract dimensions.

## 6. MPI_Dims_create: automatic balancing of dimensions

Manually choosing `dims[]` for `MPI_Cart_create` requires knowing in advance a factorization of the number of processes P that produces a reasonably balanced grid. `MPI_Dims_create` automates this choice:

```cpp
int MPI_Dims_create(
    int nnodes,   // total number of processes to arrange in the grid
                   // (typically the size of the starting communicator)
    int ndims,    // number of dimensions of the desired grid
    int dims[]    // INPUT/OUTPUT: the dimensions already FIXED by the caller
                   // (value other than 0) are respected and not altered;
                   // the dimensions left at 0 by the caller are computed
                   // automatically by MPI, so that the final product of
                   // all dims[i] equals exactly nnodes
);
```

The function attempts to produce dimensions that are as **balanced as possible relative to one another**, a goal motivated by the fact that, for the same total number of processes, a "square" (or near-square) grid typically minimizes the surface/volume ratio of the subdomains in a 2D spatial decomposition, reducing the halo-exchange communication volume per unit of computational work compared to a heavily "elongated" grid (for example 1×P, which effectively degenerates into a 1D strip decomposition like the one in chapter 03a).

Example usage, with both dimensions left free (`dims[] = {0, 0}` as input):

```cpp
int size;
MPI_Comm_size(MPI_COMM_WORLD, &size);

int dims[2] = {0, 0};        // both dimensions to be determined automatically
MPI_Dims_create(size, 2, dims);
// For size=12: dims becomes {4,3} or {3,4} (depending on the implementation's
// internal factorization strategy), not {12,1} nor {1,12},
// since 4×3 is the most balanced factorization of 12 into two factors

MPI_Comm comm_cart;
int periods[2] = {0, 0};
MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &comm_cart);
```

It is also possible to explicitly **constrain** only one dimension and let `MPI_Dims_create` determine the other, by setting to a non-zero value only the component that is to be fixed:

```cpp
int dims[2] = {2, 0};  // explicitly constrain 2 rows, leave the columns free
MPI_Dims_create(size, 2, dims);
// For size=12: dims becomes {2, 6}, respecting the constraint dims[0]=2
```

If the number of processes `nnodes` does not admit a factorization compatible with the constraints already fixed by the caller (for example requesting `dims[0]=5` with `size=12`, which is not divisible by 5), the call is an MPI error. It is also important to note that, if **all** the dimensions have already been explicitly fixed by the caller (no 0 value on input), `MPI_Dims_create` merely verifies their consistency with respect to `nnodes` (the product must match exactly), without modifying any value.

## 7. Guided exercises

### Exercise 1 — Grid creation and neighbor printing (`ex1_cart_create.cpp`)

Creates a 2D Cartesian grid with `MPI_Cart_create`. Each process determines its own coordinates with `MPI_Cart_coords` and the ranks of its four neighbors (north, south, west, east) with four calls to `MPI_Cart_shift` (two for the vertical axis with `disp=±1`, two for the horizontal axis with `disp=±1`), printing the result.

**Objective:** become familiar with the minimal sequence of calls needed to instantiate a Cartesian topology and query it (`Cart_create` → `Cart_coords` → `Cart_shift`), and directly observe in the output the appearance of `MPI_PROC_NULL` for processes positioned at the edges of the grid, verifying the correctness of the edge-case handling described in section 5.

### Exercise 2 — Halo exchange on a grid (`ex2_halo_exchange.cpp`)

Each process exchanges boundary values with its four neighbors using `MPI_Sendrecv`, applying the same communication pattern already seen in chapter 03a (section 2.5) for 1D halo exchange, but generalized to two dimensions: here each process has up to four neighbors (instead of two), corresponding to the four sides of a two-dimensional portion of the domain, rather than just the north/south sides of a 1D strip.

**Objective:** generalize the domain decomposition and halo exchange pattern from the 1D strip grid (chapter 03a) to a 2D block decomposition, observing how the use of `MPI_Cart_shift` eliminates the need to manually compute the ranks of the four neighbors (a computation that, in a manually indexed 2D block decomposition, would be significantly more error-prone than in the 1D case due to the double row/column indexing).

### Exercise 3 — MPI_Dims_create (`ex3_dims_create.cpp`)

Uses `MPI_Dims_create` to automatically determine the most balanced possible decomposition for a given number of processes, without fixing any dimension in advance (`dims[] = {0, 0}` as input).

**Objective:** observe the behavior of `MPI_Dims_create` on `size` values with different factorizations (numbers with many factors, such as 12 or 16, compared to prime numbers, such as 7 or 13, for which the only possible balanced factorization into two integer factors is trivial, typically `1×P`), gaining an empirical understanding of the relationship between the arithmetic factorization of `size` and the geometric "quality" (aspect ratio) of the resulting grid.

## 8. Expected output and how to interpret it

### ex1_cart_create (run with `-np 6`, 2×3 grid)

```text
Process 0 → coords(0,0) | N=MPI_PROC_NULL  S=3  W=MPI_PROC_NULL  E=1
Process 1 → coords(0,1) | N=MPI_PROC_NULL  S=4  W=0              E=2
Process 2 → coords(0,2) | N=MPI_PROC_NULL  S=5  W=1              E=MPI_PROC_NULL
Process 3 → coords(1,0) | N=0              S=MPI_PROC_NULL  W=MPI_PROC_NULL  E=4
...
```

With `size=6` and a grid `dims={2,3}` (2 rows, 3 columns, consistent with the row-major convention from section 3), ranks 0, 1, 2 occupy row 0 and ranks 3, 4, 5 occupy row 1. Every process in row 0 correctly has `N=MPI_PROC_NULL` (no north neighbor, being on the first row with the grid non-periodic in this example, `periods[0]=0`), while its south neighbor is the process in the same column on row 1 (for example, for process 0 at `(0,0)`, the south neighbor is process 3 at `(1,0)`, consistent with the relationship `rank = coords[0]*dims[1] + coords[1]` from section 3: `1*3+0=3`). Similarly, the processes in column 0 (P0, P3) have `W=MPI_PROC_NULL` and the processes in column 2 (P2, P5, not shown but deducible by symmetry) have `E=MPI_PROC_NULL`, consistent with the absence of periodicity on the horizontal axis as well. Note that every `S`/`N`/`E`/`W` value other than `MPI_PROC_NULL` is symmetric and reciprocal between pairs of adjacent processes: the south neighbor of P0 is P3, and — as expected — the north neighbor of P3 is P0, verifiable in the corresponding line of the output.

## 9. Common mistakes and how to avoid them

| Mistake | Typical cause | How to avoid it |
|---|---|---|
| `MPI_Cart_create` terminates the program with an error | The product of the supplied `dims[]` exceeds the number of processes available in the starting communicator | Verify `dims[0] * dims[1] * ... == size` (or `≤ size`, explicitly handling the excess processes) before the call, typically deriving `dims[]` from `MPI_Dims_create` (section 6) instead of arbitrarily chosen values |
| Neighbor ranks computed "by hand" (without `MPI_Cart_shift`) turn out wrong when `reorder=1` | Implicit assumption that the rank in the Cartesian communicator coincides with the original rank in `comm_old`, violated when the MPI implementation reassigns ranks for physical mapping optimization (section 1) | With `reorder=1`, never manually compute neighbor ranks from the original rank: always use `MPI_Cart_shift`/`MPI_Cart_coords`/`MPI_Cart_rank`, which correctly reflect the actual mapping whatever it may be |
| Confusion between `direction` and the geometric north/south/east/west label used in one's own code | `direction=0` and `direction=1` are abstract indices of the grid's dimensions (section 5); the association with "vertical" or "horizontal", and consequently with north/south or east/west, is a convention chosen by the programmer at design time, not imposed by MPI | Explicitly document, at the start of the code, the chosen convention (e.g. "dims[0]=rows → direction=0 is the vertical axis") and keep it consistent across all `MPI_Cart_shift` calls in the program |
| Communication hangs indefinitely (deadlock) in the 2D halo exchange of exercise 2 | Asymmetric handling of the four neighbors, for example using separate `MPI_Send`/`MPI_Recv` instead of `MPI_Sendrecv` for each pair of opposite directions (the same general scenario already discussed in chapter 01a section 6 and in chapter 03a section 2.5, here replicated across four directions instead of two) | Always use `MPI_Sendrecv` for each pair of opposite direction/way (one call for the vertical axis, one for the horizontal axis), passing the outputs of `MPI_Cart_shift` directly as the `dest`/`source` parameters, as shown in section 5 |
