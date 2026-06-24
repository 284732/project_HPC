// =============================================================
// EXERCISE 3 — MPI_Allreduce: L2 Norm of a Distributed Vector
// =============================================================
// Each process owns a portion of a global vector.
// Compute the L2 norm: ||v|| = sqrt( sum(v_i^2) )
// With Allreduce, ALL processes obtain the final result.
// Useful when every process needs the global result
// (e.g., convergence tests in Jacobi).
//
// Compilation:  mpicxx -O2 -Wall -o ex3_allreduce ex3_allreduce.cpp
// Execution:    mpirun -np 4 ./ex3_allreduce
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int N_LOCAL = 5;  // elements per process

    // Each process initializes its own portion of the vector
    // Process k owns: [k*N_LOCAL+1, ..., k*N_LOCAL+N_LOCAL]
    std::vector<double> local_vector(N_LOCAL);
    for (int i = 0; i < N_LOCAL; i++)
        local_vector[i] = static_cast<double>(rank * N_LOCAL + i + 1);

    std::cout << "[Process " << rank << "] local portion: ";
    for (double x : local_vector) std::cout << x << " ";
    std::cout << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: Compute the local contribution to the squared norm
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double local_norm_sq = 0.0;
    for (double x : local_vector)
        local_norm_sq += x * x;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: Allreduce → everyone obtains the global norm
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MPI_Reduce would return the result only to the root.
    // MPI_Allreduce returns it to EVERYONE → no extra Bcast needed!
    double global_norm_sq = 0.0;
    MPI_Allreduce(&local_norm_sq, &global_norm_sq,
                  1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    double norm = std::sqrt(global_norm_sq);

    // Every process has the result → all can print it
    std::cout << "[Process " << rank << "] global L2 norm = "
              << std::setprecision(6) << norm << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // BONUS: MPI_Allreduce to find the global maximum
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double local_max = *std::max_element(local_vector.begin(), local_vector.end());
    double global_max = 0.0;

    MPI_Allreduce(&local_max, &global_max,
                  1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    if (rank == 0)
        std::cout << "\n[Root] Global maximum = " << global_max
                  << " (expected: "
                  << static_cast<double>(size * N_LOCAL)
                  << ")"
                  << std::endl;

    MPI_Finalize();
    return 0;
}