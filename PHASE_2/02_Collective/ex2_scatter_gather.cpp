// =============================================================
// EXERCISE 2 — MPI_Scatter + MPI_Gather: Distributed Sum
// =============================================================
// The root process owns a vector of N elements
// (N must be a multiple of size).
// It scatters the vector: each process receives N/size elements.
// Each worker computes the sum of its own portion.
// The root collects the results using Gather.
//
// Compilation:  mpicxx -O2 -Wall -o ex2_scatter ex2_scatter_gather.cpp
// Execution:    mpirun -np 4 ./ex2_scatter
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <numeric>   // for std::iota and std::accumulate
#include <iomanip>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int N = 12;  // total size (must be a multiple of size)

    if (N % size != 0) {
        if (rank == 0)
            std::cerr << "ERROR: N=" << N
                      << " is not a multiple of size="
                      << size << std::endl;
        MPI_Finalize();
        return 1;
    }

    int chunk = N / size;  // number of elements received by each process

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 1: The root initializes the global vector
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::vector<double> global_vector;  // only the root fills it

    if (rank == 0) {
        global_vector.resize(N);

        for (int i = 0; i < N; i++)
            global_vector[i] = i + 1.0;  // 1,2,...,N

        std::cout << "Global vector: ";
        for (double x : global_vector) std::cout << x << " ";
        std::cout << "\n";
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 2: Scatter → each process receives 'chunk' elements
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::vector<double> my_part(chunk);

    // MPI_Scatter(sendbuf, sendcount, sendtype,
    //             recvbuf, recvcount, recvtype, root, comm)
    // - sendcount: number of elements sent TO EACH process
    // - recvcount: number of elements received by each process (= sendcount)
    MPI_Scatter(
        global_vector.data(), chunk, MPI_DOUBLE,  // source (used only by root)
        my_part.data(),       chunk, MPI_DOUBLE,  // destination (all processes)
        0, MPI_COMM_WORLD
    );

    std::cout << "[Process " << rank << "] received: ";
    for (double x : my_part) std::cout << x << " ";
    std::cout << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 3: Each process processes its own portion
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double local_sum = 0.0;
    for (double x : my_part)
        local_sum += x * x;  // sum of squares

    std::cout << "[Process " << rank
              << "] local sum of squares = "
              << local_sum << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STEP 4: Gather → root collects all results
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::vector<double> results(size);  // only the root uses it

    // MPI_Gather: each process sends 1 double,
    // the root receives 'size' doubles
    MPI_Gather(
        &local_sum, 1, MPI_DOUBLE,          // what each process sends
        results.data(), 1, MPI_DOUBLE,      // where the root collects
        0, MPI_COMM_WORLD
    );

    if (rank == 0) {
        double total = 0.0;

        std::cout << "\n[Root] Collected results:\n";

        for (int i = 0; i < size; i++) {
            std::cout << "  Process "
                      << i
                      << ": "
                      << results[i]
                      << "\n";
            total += results[i];
        }

        std::cout << "Total sum of squares = "
                  << total << "\n";

        // Verification:
        // sum(i^2) for i = 1..N = N(N+1)(2N+1)/6
        double expected =
            static_cast<double>(N) * (N + 1) * (2 * N + 1) / 6.0;

        std::cout << "Expected value       = "
                  << expected << "\n";

        std::cout
            << (std::abs(total - expected) < 1e-9
                    ? " CORRECT"
                    : " ERROR")
            << "\n";
    }

    MPI_Finalize();
    return 0;
}
