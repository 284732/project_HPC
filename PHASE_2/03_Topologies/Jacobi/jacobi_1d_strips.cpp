// =============================================================
// JACOBI 1D — Iterative Solver with Strip Domain Decomposition
// =============================================================
// Solves the Laplace equation on an N x N grid.
// Each process manages N/size rows + 2 ghost rows.
//
// Boundary conditions:
//   - Bottom boundary (row N-1): u = 1.0  (hot)
//   - All other boundaries:      u = 0.0
//
// Compilation:  mpicxx -O2 -Wall -o jacobi1d jacobi_1d_strips.cpp
// Execution:    mpirun -np 4 ./jacobi1d
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>  // for std::max

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Problem parameters
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    const int N     = 64;       // global grid size (N x N)
    const double EPS = 1e-6;    // convergence threshold
    const int MAX_ITER = 10000;

    if (N % size != 0) {
        if (rank == 0)
            std::cerr << "ERROR: N=" << N
                      << " is not divisible by size="
                      << size << std::endl;
        MPI_Finalize();
        return 1;
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Domain decomposition
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int local_rows = N / size;          // actual rows per process
    int row_start  = rank * local_rows; // first global row
    int row_end    = row_start + local_rows - 1; // last global row

    // Local array: actual_rows + 2 ghost rows = local_rows + 2 total
    // Indices: 0 = north ghost, 1..local_rows = actual rows,
    //          local_rows+1 = south ghost
    int n_rows_loc = local_rows + 2;
    std::vector<double> u(n_rows_loc * N, 0.0);      // current solution
    std::vector<double> u_new(n_rows_loc * N, 0.0);  // next iteration

    // Macro for 2D access: u[i][j] → u[i*N + j]
    auto idx = [&](int i, int j) { return i * N + j; };

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Boundary conditions (Dirichlet)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Global bottom boundary (row N-1): u = 1.0
    // Determine which process owns row N-1
    if (row_end == N - 1) {
        for (int j = 0; j < N; j++)
            u[idx(local_rows, j)] = 1.0;  // last actual row = hot boundary
    }

    // Left and right boundaries: u = 0.0 (already initialized)
    // Top boundary (row 0): u = 0.0 (already initialized)

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Neighbor identification
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int north_neighbor = (rank > 0)        ? rank - 1 : MPI_PROC_NULL;
    int south_neighbor = (rank < size - 1) ? rank + 1 : MPI_PROC_NULL;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Jacobi loop
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double t_start = MPI_Wtime();
    int iter = 0;
    double global_change = 1.0;

    const int TAG_NS = 10, TAG_SN = 11;

    while (global_change > EPS && iter < MAX_ITER) {

        // --- 1. Halo Exchange ---
        // Send first actual row north, receive north ghost row
        MPI_Sendrecv(
            &u[idx(1, 0)],           N, MPI_DOUBLE, north_neighbor, TAG_NS,
            &u[idx(0, 0)],           N, MPI_DOUBLE, north_neighbor, TAG_SN,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );

        // Send last actual row south, receive south ghost row
        MPI_Sendrecv(
            &u[idx(local_rows, 0)],     N, MPI_DOUBLE, south_neighbor, TAG_SN,
            &u[idx(local_rows + 1, 0)], N, MPI_DOUBLE, south_neighbor, TAG_NS,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );

        // --- 2. Jacobi Update ---
        double local_change = 0.0;

        for (int i = 1; i <= local_rows; i++) {
            int global_i = row_start + (i - 1);

            for (int j = 0; j < N; j++) {

                // Skip left and right boundaries
                if (j == 0 || j == N - 1) {
                    u_new[idx(i, j)] = u[idx(i, j)];
                    continue;
                }

                // Skip the global top boundary
                if (global_i == 0) {
                    u_new[idx(i, j)] = 0.0;
                    continue;
                }

                // Skip the global bottom boundary (fixed temperature)
                if (global_i == N - 1) {
                    u_new[idx(i, j)] = 1.0;
                    continue;
                }

                // 5-point Jacobi stencil
                u_new[idx(i, j)] = 0.25 * (
                    u[idx(i - 1, j)] +  // north
                    u[idx(i + 1, j)] +  // south
                    u[idx(i, j - 1)] +  // west
                    u[idx(i, j + 1)]    // east
                );

                double diff = std::abs(u_new[idx(i, j)] - u[idx(i, j)]);
                local_change = std::max(local_change, diff);
            }
        }

        // --- 3. Global Convergence Check ---
        // Allreduce with MPI_MAX:
        // everyone obtains the maximum global change
        MPI_Allreduce(&local_change, &global_change,
                      1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        // --- 4. Swap u and u_new ---
        std::swap(u, u_new);

        iter++;

        // Print progress every 100 iterations (process 0 only)
        if (rank == 0 && iter % 100 == 0) {
            std::cout << "[Iter " << std::setw(5) << iter << "] "
                      << "max change = "
                      << std::scientific << std::setprecision(4)
                      << global_change << std::endl;
        }
    }

    double t_end = MPI_Wtime();

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Final results
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    if (rank == 0) {
        if (global_change <= EPS) {
            std::cout << "\n CONVERGENCE reached in "
                      << iter
                      << " iterations!\n";
        } else {
            std::cout << "\n MAX_ITER reached without convergence.\n";
        }

        std::cout << "  Final change:      "
                  << global_change << "\n";

        std::cout << "  Total time:        "
                  << std::fixed << std::setprecision(3)
                  << (t_end - t_start)
                  << " seconds\n";

        std::cout << "  Processes used:    "
                  << size << "\n";
    }

    MPI_Finalize();
    return 0;
}
