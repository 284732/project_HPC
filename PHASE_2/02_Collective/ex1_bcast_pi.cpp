// =============================================================
// EXERCISE 1 — MPI_Bcast + MPI_Reduce: Distributed PI Computation
// =============================================================
// Process 0 broadcasts the number of terms N.
// Each process computes a portion of the Leibniz series:
//   PI/4 = 1 - 1/3 + 1/5 - 1/7 + ...
// The partial results are summed using MPI_Reduce.
//
// Compilation:  mpicxx -O2 -Wall -o ex1_pi ex1_bcast_pi.cpp
// Execution:    mpirun -np 4 ./ex1_pi
// =============================================================

#include <mpi.h>
#include <iostream>
#include <cmath>
#include <iomanip>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    long long N = 10000000LL;  // total number of terms in the series

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: The root broadcasts N to everyone
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // All processes call MPI_Bcast: the root sends, the others receive.
    // The root is the process with rank = 0.
    MPI_Bcast(&N, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    if (rank == 0)
        std::cout << "[Root] N = " << N
                  << ", broadcast to all processes.\n";

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: Each process computes its own portion
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Decomposition: process i computes terms
    // i, i+size, i+2*size, ...
    double local_sum = 0.0;

    for (long long k = rank; k < N; k += size) {
        // Term k of the Leibniz series: (-1)^k / (2k+1)
        double term = 1.0 / (2.0 * k + 1.0);
        if (k % 2 == 0)
            local_sum += term;
        else
            local_sum -= term;
    }

    std::cout << "[Process " << rank << "] partial sum = "
              << std::setprecision(10) << local_sum << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: Reduction: sum all partial sums
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double total_sum = 0.0;
    MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double approximated_pi = 4.0 * total_sum;
        double error = std::abs(approximated_pi - M_PI);

        std::cout << "\n=== Result ===\n";
        std::cout << std::setprecision(12);
        std::cout << "Approximated PI = " << approximated_pi << "\n";
        std::cout << "Actual PI       = " << M_PI << "\n";
        std::cout << "Absolute error  = " << error << "\n";
    }

    MPI_Finalize();
    return 0;
}