// =============================================================
// EXERCISE 3 — MPI_Dims_create: Automatic Domain Decomposition
// =============================================================
// MPI_Dims_create finds the most "square" factorization possible
// to distribute 'size' processes over a 2D grid.
//
// Examples:
//   size=4  → dims = [2, 2]
//   size=6  → dims = [2, 3]
//   size=8  → dims = [2, 4]
//   size=12 → dims = [3, 4]
//
// Compilation:  mpicxx -O2 -Wall -o ex3_dims ex3_dims_create.cpp
// Execution:    mpirun -np 6 ./ex3_dims
// =============================================================

#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MPI_Dims_create: find optimal dimensions
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // dims must be initialized to 0 for dimensions that are free.
    // If a value > 0 is provided, that dimension is fixed.
    int dims[2] = {0, 0};  // both dimensions are free: MPI determines them
    MPI_Dims_create(size, 2, dims);

    if (rank == 0) {
        std::cout << "Total processes: " << size << "\n";
        std::cout << "Optimal grid: " << dims[0] << " x " << dims[1] << "\n\n";
    }

    // Create the topology using the computed dimensions
    int periods[2] = {0, 0};
    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &comm_cart);

    int rank_cart, coords[2];
    MPI_Comm_rank(comm_cart, &rank_cart);
    MPI_Cart_coords(comm_cart, rank_cart, 2, coords);

    MPI_Barrier(comm_cart);
    std::cout << "Process " << rank_cart
              << " → position in the grid: row=" << coords[0]
              << ", column=" << coords[1] << std::endl;

    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}
