// =============================================================
// EXERCISE 2 — Computation/Communication Overlap
// =============================================================
// Each process owns a local vector. It sends its vector
// to a neighbor while simultaneously computing the local sum.
// Compare the execution time with the sequential blocking version.
//
// Compilation:  mpicxx -O2 -Wall -o ex2_overlap ex2_overlap.cpp
// Execution:    mpirun -np 4 ./ex2_overlap
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <numeric>    // for std::iota
#include <iomanip>

// Simulates a heavy computation: sum a large vector
double sum_vector(const std::vector<double>& v) {
    double s = 0.0;
    for (double x : v) s += x;
    return s;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int N = 100000;  // local vector size
    const int TAG = 5;
    int right = (rank + 1) % size;
    int left  = (rank - 1 + size) % size;

    // Local vector: each process owns values = rank * N + i
    std::vector<double> my_vector(N);
    for (int i = 0; i < N; i++)
        my_vector[i] = static_cast<double>(rank * N + i);

    std::vector<double> received_vector(N);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // NON-BLOCKING VERSION (with overlap)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    MPI_Request reqs[2];

    // Start communication in the background
    MPI_Isend(my_vector.data(),      N, MPI_DOUBLE, right, TAG, MPI_COMM_WORLD, &reqs[0]);
    MPI_Irecv(received_vector.data(), N, MPI_DOUBLE, left,  TAG, MPI_COMM_WORLD, &reqs[1]);

    // While communication is in progress, compute the local sum
    double local_sum = sum_vector(my_vector);

    // Now wait for communication to complete
    MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);

    double t1 = MPI_Wtime();
    double received_sum = sum_vector(received_vector);

    if (rank == 0) {
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "\n[Non-blocking] Time: " << (t1 - t0) * 1000.0 << " ms\n";
        std::cout << "  Local sum:      " << local_sum << "\n";
        std::cout << "  Received sum:   " << received_sum << "\n";
    }

    MPI_Finalize();
    return 0;
}
