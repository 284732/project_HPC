// =============================================================
// EXERCISE 1 — MPI_Cart_create: 2D Grid and Neighbor Discovery
// =============================================================
// Creates a virtual 2D Cartesian topology.
// Each process prints its coordinates and the ranks of its
// neighbors (North, South, East, West).
// Non-periodic boundaries → MPI_PROC_NULL.
//
// Compilation:  mpicxx -O2 -Wall -o ex1_cart ex1_cart_create.cpp
// Execution:    mpirun -np 6 ./ex1_cart
// =============================================================

#include <mpi.h>
#include <iostream>
#include <string>

// Utility function: converts a rank into a readable string
std::string rank_str(int r) {
    if (r == MPI_PROC_NULL) return "NULL";
    return std::to_string(r);
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: Define the grid dimensions
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // With 6 processes, let's try a 2x3 grid
    int ndims = 2;
    int dims[2]    = {2, 3};   // rows=2, columns=3
    int periods[2] = {0, 0};   // non-periodic ("physical" boundaries)
    int reorder    = 1;        // MPI may optimize the mapping

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: Create the Cartesian communicator
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, reorder, &comm_cart);

    // Rank in the new communicator
    // (may differ from MPI_COMM_WORLD if reorder=1)
    int rank_cart;
    MPI_Comm_rank(comm_cart, &rank_cart);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: Get the coordinates of the current process
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int coords[2];
    MPI_Cart_coords(comm_cart, rank_cart, ndims, coords);

    // coords[0] = row, coords[1] = column

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 4: Find neighbors with MPI_Cart_shift
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MPI_Cart_shift(comm, direction, disp, &rank_src, &rank_dst)
    //
    // - direction=0 → dimension 0 (rows = vertical = North/South)
    // - direction=1 → dimension 1 (columns = horizontal = East/West)
    // - disp=+1     → move forward in the dimension (South / East)
    // - disp=-1     → move backward in the dimension (North / West)
    //
    // If the neighbor does not exist
    // (non-periodic boundary), rank = MPI_PROC_NULL

    int north_neighbor, south_neighbor;

    MPI_Cart_shift(comm_cart, 0, -1,
                   &south_neighbor, &north_neighbor);  // dim 0, disp=-1: move up

    MPI_Cart_shift(comm_cart, 0, +1,
                   &north_neighbor, &south_neighbor);  // dim 0, disp=+1: move down

    // Note: Cart_shift(comm, dir, disp, &src, &dst)
    //
    //   src = who would send data to me if I performed a Recv
    //         (= the neighbor I come from)
    //
    //   dst = who receives data from me if I perform a Send
    //         (= the neighbor I move toward)
    //
    // To find North (the process above me):
    // I "move" by -1 along direction 0.
    //
    //   → source = the neighbor I approach by moving
    //   → dest   = the neighbor I move away from
    //
    // This is somewhat counterintuitive,
    // so let's use the explicit version:

    int north, south, east, west;
    int dummy;  // the "source" returned by Cart_shift, unused here

    // NORTH neighbor (row - 1): shift -1 in direction 0
    MPI_Cart_shift(comm_cart, 0, -1, &dummy, &north);

    // SOUTH neighbor (row + 1): shift +1 in direction 0
    MPI_Cart_shift(comm_cart, 0, +1, &dummy, &south);

    // WEST neighbor (col - 1): shift -1 in direction 1
    MPI_Cart_shift(comm_cart, 1, -1, &dummy, &west);

    // EAST neighbor (col + 1): shift +1 in direction 1
    MPI_Cart_shift(comm_cart, 1, +1, &dummy, &east);

    // Synchronize output to improve readability
    MPI_Barrier(comm_cart);

    std::cout << "P" << rank_cart
              << " coords(" << coords[0] << "," << coords[1] << ")"
              << " | N=" << rank_str(north)
              << " S=" << rank_str(south)
              << " W=" << rank_str(west)
              << " E=" << rank_str(east)
              << std::endl;

    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}
