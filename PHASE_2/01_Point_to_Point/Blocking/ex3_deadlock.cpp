// =============================================================
// EXERCISE 3 — Deadlock: Causes and Solutions
// =============================================================
// Demonstrates the classic deadlock and three ways to solve it.
// Uncomment the version you want to test (one at a time).
//
// Compilation:  mpicxx -O2 -Wall -o ex3_deadlock ex3_deadlock.cpp
// Execution:    mpirun -np 2 ./ex3_deadlock
// =============================================================

#include <mpi.h>
#include <iostream>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// VERSION 1 — DEADLOCK (DO NOT uncomment in production!)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Both processes attempt to send before receiving.
// MPI_Send may block if the internal buffer is full.
// → Deadlock guaranteed for large messages (beyond the eager buffer).
void version_deadlock(int rank) {
    double buf_send = rank;
    double buf_recv = -1.0;
    int other = 1 - rank;  // 0→1, 1→0

    MPI_Send(&buf_send, 1, MPI_DOUBLE, other, 0, MPI_COMM_WORLD);  // ← BLOCKS?
    MPI_Recv(&buf_recv, 1, MPI_DOUBLE, other, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    std::cout << "Process " << rank << " received: " << buf_recv << std::endl;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// VERSION 2 — SOLUTION: Order Send/Recv
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// One process performs Send first, the other performs Recv first.
// No circular dependency remains → no deadlock.
void version_ordered(int rank) {
    double buf_send = static_cast<double>(rank) * 10.0;
    double buf_recv = -1.0;
    int other = 1 - rank;

    if (rank == 0) {
        // Process 0: send first, then receive
        MPI_Send(&buf_send, 1, MPI_DOUBLE, other, 0, MPI_COMM_WORLD);
        MPI_Recv(&buf_recv, 1, MPI_DOUBLE, other, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        // Process 1: receive first, then send
        MPI_Recv(&buf_recv, 1, MPI_DOUBLE, other, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&buf_send, 1, MPI_DOUBLE, other, 0, MPI_COMM_WORLD);
    }

    std::cout << "[Ordered] Process " << rank
              << " received: " << buf_recv
              << " (expected: " << static_cast<double>(other) * 10.0 << ")"
              << std::endl;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// VERSION 3 — SOLUTION: MPI_Sendrecv
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// MPI_Sendrecv combines Send and Recv into a single atomic call.
// The MPI implementation handles the ordering internally → no deadlock.
void version_sendrecv(int rank) {
    double buf_send = static_cast<double>(rank) * 10.0;
    double buf_recv = -1.0;
    int other = 1 - rank;

    MPI_Sendrecv(sendbuf, sendcount, sendtype, dest,   sendtag,
                 recvbuf, recvcount, recvtype, source, recvtag,
                 comm, status)
    MPI_Sendrecv(
        &buf_send, 1, MPI_DOUBLE, other, 0,    // send
        &buf_recv, 1, MPI_DOUBLE, other, 0,    // receive
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );

    std::cout << "[Sendrecv] Process " << rank
              << " received: " << buf_recv
              << " (expected: " << static_cast<double>(other) * 10.0 << ")"
              << std::endl;
}


int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0)
            std::cerr << "ERROR: Use exactly 2 processes!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) std::cout << "\n--- Version with ordered Send/Recv ---\n";
    MPI_Barrier(MPI_COMM_WORLD);
    version_ordered(rank);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) std::cout << "\n--- Version with MPI_Sendrecv ---\n";
    MPI_Barrier(MPI_COMM_WORLD);
    version_sendrecv(rank);

    MPI_Finalize();
    return 0;
}
