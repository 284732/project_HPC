// =============================================================
// EXERCISE 4 — Ring Communication
// =============================================================
// Each process sends a token to the next one:
//   rank → (rank+1) % size
// Process 0 starts with token=0, and each process increments it.
// At the end, process 0 receives the token after it has passed
// through all processes.
//
// Compilation:  mpicxx -O2 -Wall -o ex4_ring ex4_ring.cpp
// Execution:    mpirun -np 4 ./ex4_ring
// =============================================================

#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Rank of the left neighbor (from whom I receive)
    // and right neighbor (to whom I send)
    int left  = (rank - 1 + size) % size;  // who sends to me
    int right = (rank + 1) % size;         // whom I send to

    int token = 0;
    const int TAG = 99;

    if (rank == 0) {
        // -------------------------------------------------------
        // Process 0: starts with token=0, sends to the right,
        // waits for it to return from the last process
        // (left = size-1)
        // -------------------------------------------------------
        token = 0;
        std::cout << "Process 0: starting with token = " << token
                  << ", sending to process " << right << std::endl;

        MPI_Send(&token, 1, MPI_INT, right, TAG, MPI_COMM_WORLD);
        MPI_Recv(&token, 1, MPI_INT, left, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::cout << "Process 0: final token received = " << token << std::endl;
        std::cout << "Expected value: " << size - 1 << std::endl;
        std::cout << (token == size - 1 ? " CORRECT" : " ERROR") << std::endl;

    } else {
        // -------------------------------------------------------
        // Processes 1..size-1: receive, increment, resend
        // -------------------------------------------------------
        MPI_Recv(&token, 1, MPI_INT, left, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::cout << "Process " << rank << ": received token = " << token
                  << " from process " << left;

        token += 1;  // each process adds 1 to the token

        std::cout << ", sending " << token
                  << " to process " << right << std::endl;

        MPI_Send(&token, 1, MPI_INT, right, TAG, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
