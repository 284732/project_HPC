# 04a — Virtual Topologies (Cartesian Grid)

## Theory

An MPI **virtual topology** maps processes onto a logical geometric structure (grid,
torus, etc.). It does not change physical performance, but it:

- Simplifies neighbour management (who is left / right / above / below?)
- Makes the code more readable and maintainable
- Allows MPI to optimize the physical rank mapping on certain hardware

---

### MPI_Cart_create

```cpp
int MPI_Cart_create(
    MPI_Comm  comm_old,    // input communicator
    int       ndims,       // number of dimensions (2 for a 2D grid)
    const int dims[],      // size of each dimension: dims[0]=rows, dims[1]=cols
    const int periods[],   // periodicity: 1=toroidal, 0=fixed boundary
    int       reorder,     // 1=MPI may reorder ranks, 0=keep original order
    MPI_Comm* comm_cart    // new Cartesian communicator
);
```

### Key Functions

```cpp
// Get the coordinates of the current process
MPI_Cart_coords(comm_cart, rank, ndims, coords);

// Get the rank corresponding to a set of coordinates
MPI_Cart_rank(comm_cart, coords, &rank);

// Find neighbours along a given dimension
// direction=0 → vertical (rows),   direction=1 → horizontal (columns)
// disp=+1     → forward neighbour, disp=-1     → backward neighbour
// Returns MPI_PROC_NULL if the neighbour is outside the grid (periods=0)
MPI_Cart_shift(comm_cart, direction, disp, &rank_source, &rank_destination);
```

### Layout of a 4×3 Grid

```
          col 0     col 1     col 2
row 0  [P0(0,0)] [P1(0,1)] [P2(0,2)]
row 1  [P3(1,0)] [P4(1,1)] [P5(1,2)]
row 2  [P6(2,0)] [P7(2,1)] [P8(2,2)]
row 3  [P9(3,0)] [P10(3,1)][P11(3,2)]
```

For P4 (rank=4, coords=(1,1)):
- NORTH neighbour: P1  (rank=1)
- SOUTH neighbour: P7  (rank=7)
- EAST  neighbour: P5  (rank=5)
- WEST  neighbour: P3  (rank=3)

### MPI_Cart_sub — Sub-communicators

```cpp
// Extract row or column communicators from the Cartesian topology
int remain_dims_row[] = {0, 1};  // keep column dimension → one comm per row
int remain_dims_col[] = {1, 0};  // keep row dimension    → one comm per column

MPI_Comm comm_row, comm_col;
MPI_Cart_sub(comm_cart, remain_dims_row, &comm_row);
MPI_Cart_sub(comm_cart, remain_dims_col, &comm_col);
```

---

## Exercises

### Exercise 1 — Grid creation and neighbour printing — `ex1_cart_create.cpp`

Creates a 2D Cartesian grid. Each process prints its own coordinates and the ranks
of its four neighbours.

### Exercise 2 — Halo exchange on a grid — `ex2_halo_exchange.cpp`

Each process exchanges boundary values with its four neighbours using `MPI_Sendrecv`.

### Exercise 3 — MPI_Dims_create — `ex3_dims_create.cpp`

Uses `MPI_Dims_create` to automatically find the most balanced process decomposition
for a given number of processes.

---

## Expected Output

### ex1_cart_create (6 processes, 2×3 grid)

```
Process 0 → coords(0,0) | N=MPI_PROC_NULL  S=3  W=MPI_PROC_NULL  E=1
Process 1 → coords(0,1) | N=MPI_PROC_NULL  S=4  W=0              E=2
Process 2 → coords(0,2) | N=MPI_PROC_NULL  S=5  W=1              E=MPI_PROC_NULL
Process 3 → coords(1,0) | N=0              S=MPI_PROC_NULL  W=MPI_PROC_NULL  E=4
...
```
