// =============================================================
// FULL 2D JACOBI — 2D Decomposition with Cartesian Topology
// =============================================================
// Complete version featuring:
//  - MPI_Cart_create (2D process grid)
//  - MPI_Dims_create (automatic decomposition)
//  - Halo exchange on all 4 sides
//  - Convergence check with MPI_Allreduce
//
// Each process manages a rectangular subdomain.
//
// Compilation:  mpicxx -O2 -Wall -o jacobi2d jacobi_2d_full.cpp
// Execution:    mpirun -np 4 ./jacobi2d
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank_world, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_world);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Global parameters
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    const int N        = 128;   // global N x N grid
    const double EPS   = 1e-5;
    const int MAX_ITER = 50000;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 2D Cartesian topology
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int dims[2]    = {0, 0};   // computed by MPI_Dims_create
    int periods[2] = {0, 0};   // non-periodic boundaries

    MPI_Dims_create(size, 2, dims);

    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &comm_cart);

    int rank;
    MPI_Comm_rank(comm_cart, &rank);

    int coords[2];
    MPI_Cart_coords(comm_cart, rank, 2, coords);

    int my_row = coords[0];  // row position in the process grid
    int my_col = coords[1];  // column position

    // Number of rows/columns handled by each process
    // (for simplicity, assume N is divisible by dims)
    int local_rows = N / dims[0];
    int local_cols = N / dims[1];

    // First global cell handled by this process
    int global_row_start = my_row * local_rows;
    int global_col_start = my_col * local_cols;

    if (rank == 0) {
        std::cout << "Global grid: " << N << "x" << N << "\n";
        std::cout << "Processes: " << dims[0] << "x" << dims[1]
                  << " (" << size << " total)\n";
        std::cout << "Subdomain per process: "
                  << local_rows << "x" << local_cols << "\n\n";
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Neighbors (may be MPI_PROC_NULL on boundaries)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int north, south, west, east, dummy;

    MPI_Cart_shift(comm_cart, 0, -1, &dummy, &north);
    MPI_Cart_shift(comm_cart, 0, +1, &dummy, &south);
    MPI_Cart_shift(comm_cart, 1, -1, &dummy, &west);
    MPI_Cart_shift(comm_cart, 1, +1, &dummy, &east);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Allocation: subdomain + ghost cells
    // (1 ghost cell on each side)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int nr = local_rows + 2;  // rows including ghosts
    int nc = local_cols + 2;  // columns including ghosts

    std::vector<double> u(nr * nc, 0.0);
    std::vector<double> u_new(nr * nc, 0.0);

    auto idx = [&](int i, int j) { return i * nc + j; };

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Boundary conditions: bottom boundary = 1.0
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    bool has_south_boundary = (my_row == dims[0] - 1);

    if (has_south_boundary) {
        for (int j = 1; j <= local_cols; j++)
            u[idx(local_rows, j)] = 1.0;
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MPI datatype for columns (horizontal halo exchange)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Column data is not contiguous in memory,
    // so we create a derived MPI datatype.
    MPI_Datatype col_type;
    MPI_Type_vector(local_rows, 1, nc, MPI_DOUBLE, &col_type);
    MPI_Type_commit(&col_type);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Jacobi loop
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double t0 = MPI_Wtime();
    int iter = 0;
    double global_change = 1.0;

    const int TAG1 = 1, TAG2 = 2, TAG3 = 3, TAG4 = 4;

    while (global_change > EPS && iter < MAX_ITER) {

        // --- North-South Halo Exchange (rows) ---
        MPI_Sendrecv(&u[idx(1, 1)],          local_cols, MPI_DOUBLE, north, TAG1,
                     &u[idx(0, 1)],          local_cols, MPI_DOUBLE, north, TAG2,
                     comm_cart, MPI_STATUS_IGNORE);

        MPI_Sendrecv(&u[idx(local_rows, 1)], local_cols, MPI_DOUBLE, south, TAG2,
                     &u[idx(nr - 1, 1)],     local_cols, MPI_DOUBLE, south, TAG1,
                     comm_cart, MPI_STATUS_IGNORE);

        // --- East-West Halo Exchange (columns using derived datatype) ---
        MPI_Sendrecv(&u[idx(1, 1)],          1, col_type, west, TAG3,
                     &u[idx(1, nc - 1)],     1, col_type, east, TAG3,
                     comm_cart, MPI_STATUS_IGNORE);

        MPI_Sendrecv(&u[idx(1, local_cols)], 1, col_type, east, TAG4,
                     &u[idx(1, 0)],          1, col_type, west, TAG4,
                     comm_cart, MPI_STATUS_IGNORE);

        // --- Jacobi Update ---
        double local_change = 0.0;

        for (int i = 1; i <= local_rows; i++) {
            int ig = global_row_start + (i - 1);

            for (int j = 1; j <= local_cols; j++) {
                int jg = global_col_start + (j - 1);

                // Preserve boundary conditions
                if (ig == 0 || ig == N - 1 || jg == 0 || jg == N - 1) {
                    u_new[idx(i, j)] = u[idx(i, j)];
                    continue;
                }

                u_new[idx(i, j)] = 0.25 * (
                    u[idx(i - 1, j)] + u[idx(i + 1, j)] +
                    u[idx(i, j - 1)] + u[idx(i, j + 1)]
                );

                double diff = std::abs(u_new[idx(i, j)] - u[idx(i, j)]);
                local_change = std::max(local_change, diff);
            }
        }

        // --- Global Convergence Check ---
        MPI_Allreduce(&local_change, &global_change,
                      1, MPI_DOUBLE, MPI_MAX, comm_cart);

        std::swap(u, u_new);
        iter++;

        if (rank == 0 && iter % 500 == 0) {
            std::cout << "[Iter " << std::setw(5) << iter
                      << "] max_diff = "
                      << std::scientific
                      << global_change << "\n";
        }
    }

    double t1 = MPI_Wtime();

    if (rank == 0) {
        std::cout << "\n"
                  << (global_change <= EPS ? "✓ CONVERGENCE" : "⚠ MAX_ITER")
                  << " after " << iter << " iterations\n"
                  << "  Final change: " << global_change << "\n"
                  << "  Total time: " << std::fixed << std::setprecision(3)
                  << (t1 - t0) << " s\n";
    }

    MPI_Type_free(&col_type);
    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}
