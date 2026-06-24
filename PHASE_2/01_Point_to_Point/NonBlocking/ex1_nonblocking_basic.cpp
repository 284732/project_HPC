// =============================================================
// EXERCISE 1 — Basic Non-Blocking Communication: Isend/Irecv
// =============================================================
// Reimplementation of the ping-pong example using non-blocking
// operations.
// Demonstrates how to use MPI_Request and MPI_Wait.
//
// Compilation:  mpicxx -O2 -Wall -o ex1_nonblocking ex1_nonblocking_basic.cpp
// Execution:    mpirun -np 2 ./ex1_nonblocking
// =============================================================

#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) std::cerr << "Use 2 processes!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    const int N_ITER = 5;
    const int TAG = 7;
    int other = 1 - rank;

    double buf_send, buf_recv;

    for (int i = 0; i < N_ITER; i++) {

        // Each process prepares its value to send
        buf_send = rank * 100.0 + i;

        // MPI_Request: handle for an ongoing operation
        MPI_Request req_send, req_recv;

        // --- Start non-blocking send and receive ---
        // Note: both operations start immediately without blocking!
        MPI_Isend(&buf_send, 1, MPI_DOUBLE, other, TAG, MPI_COMM_WORLD, &req_send);
        MPI_Irecv(&buf_recv, 1, MPI_DOUBLE, other, TAG, MPI_COMM_WORLD, &req_recv);

        // *** Useful work could be performed here! ***
        // (There is no extra work in this example, but the structure is correct)

        // --- Wait until BOTH operations have completed
        //     (one send and one receive) ---
        MPI_Wait(&req_send, MPI_STATUS_IGNORE);
        MPI_Wait(&req_recv, MPI_STATUS_IGNORE);

        // Now buf_recv contains the value received from the other process
        std::cout << "Iteration " << i
                  << " | Process " << rank
                  << " | sent: " << buf_send
                  << " | received: " << buf_recv
                  << std::endl;
    }

    MPI_Finalize();
    return 0;
}

