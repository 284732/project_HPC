// =============================================================
// EXERCISE 2 — 2D Halo Exchange
// =============================================================
// Each process manages a local subdomain (with ghost cells).
// The "halo exchange" updates ghost cells using data from neighbors.
// Used in every iteration of Jacobi, Gauss-Seidel, finite differences, etc.
//
// Local memory layout (4x4 with ghost cells):
//
//   g g g g g g     ← North ghost row
//   g * * * * g
//   g * * * * g     * = real cells
//   g * * * * g     g = ghost cells (updated by neighbors)
//   g * * * * g
//   g g g g g g     ← South ghost row
//
// Compilation:  mpicxx -O2 -Wall -o ex2_halo ex2_halo_exchange.cpp
// Execution:    mpirun -np 4 ./ex2_halo
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <iomanip>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Process grid: assume a 1D decomposition for simplicity
    // (only North-South exchange, each process owns a strip of rows)
    int dims[2]    = {size, 1};  // size rows, 1 column
    int periods[2] = {0, 0};
    int reorder    = 0;

    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &comm_cart);

    int rank_cart, coords[2];
    MPI_Comm_rank(comm_cart, &rank_cart);
    MPI_Cart_coords(comm_cart, rank_cart, 2, coords);

    // North and South neighbors
    int north_neighbor, south_neighbor, dummy;
    MPI_Cart_shift(comm_cart, 0, -1, &dummy, &north_neighbor);
    MPI_Cart_shift(comm_cart, 0, +1, &dummy, &south_neighbor);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: Initialize the local domain (with ghost rows)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    const int N_LOCAL_ROWS = 4;  // real rows per process
    const int N_COLS       = 6;  // columns (same for all processes)

    // With ghosts: (N_LOCAL_ROWS + 2) total rows
    int total_rows = N_LOCAL_ROWS + 2;

    // Layout: u[i][j], i=0 north ghost, i=total_rows-1 south ghost
    std::vector<std::vector<double>> u(total_rows, std::vector<double>(N_COLS, 0.0));

    // Initialize real cells with the process rank
    for (int i = 1; i <= N_LOCAL_ROWS; i++)
        for (int j = 0; j < N_COLS; j++)
            u[i][j] = static_cast<double>(rank_cart);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: Halo Exchange (exchange boundary rows)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Send our first real row to the NORTH neighbor.
    // Receive from the NORTH neighbor into our north ghost row (row 0).
    // Similarly for the SOUTH neighbor.

    const int TAG_NS = 1;  // tag for North-to-South exchange
    const int TAG_SN = 2;  // tag for South-to-North exchange

    // Exchange with the NORTH neighbor
    // Send u[1] north, receive from north into u[0]
    MPI_Sendrecv(
        u[1].data(), N_COLS, MPI_DOUBLE, north_neighbor, TAG_NS,
        u[0].data(), N_COLS, MPI_DOUBLE, north_neighbor, TAG_SN,
        comm_cart, MPI_STATUS_IGNORE
    );

    // Exchange with the SOUTH neighbor
    // Send u[N_LOCAL_ROWS] south, receive from south into u[N_LOCAL_ROWS+1]
    MPI_Sendrecv(
        u[N_LOCAL_ROWS].data(),   N_COLS, MPI_DOUBLE, south_neighbor, TAG_SN,
        u[N_LOCAL_ROWS+1].data(), N_COLS, MPI_DOUBLE, south_neighbor, TAG_NS,
        comm_cart, MPI_STATUS_IGNORE
    );

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: Verification and output
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    MPI_Barrier(comm_cart);

    if (rank_cart == 0) {
        std::cout << "\nAfter halo exchange, subdomain of process " << rank_cart
                  << " (north_neighbor=" << (north_neighbor == MPI_PROC_NULL ? -1 : north_neighbor)
                  << ", south_neighbor=" << (south_neighbor == MPI_PROC_NULL ? -1 : south_neighbor) << "):\n";

        for (int i = 0; i < total_rows; i++) {
            std::string label = (i == 0) ? " [ghost N]" :
                                (i == total_rows - 1) ? " [ghost S]" : " [real    ]";

            std::cout << label << " row " << i << ": ";
            for (double v : u[i]) std::cout << std::setw(4) << v;
            std::cout << "\n";
        }

        std::cout << "(Ghost rows contain -1 if the neighbor does not exist)\n";
    }

    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}
