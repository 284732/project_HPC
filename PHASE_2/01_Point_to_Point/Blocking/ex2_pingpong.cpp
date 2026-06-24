// =============================================================
// EXERCISE 2 — Ping-Pong Between Two Processes + Time Measurement
// =============================================================
// Process 0 and process 1 exchange a double N times.
// Measures the average latency using MPI_Wtime().
//
// Use exactly 2 processes:
// Compilation:  mpicxx -O2 -o ex2_pingpong ex2_pingpong.cpp
// Execution:    mpirun -np 2 ./ex2_pingpong
// =============================================================

#include <mpi.h>
#include <iostream>
#include <iomanip>   // for std::setprecision

int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Check: this exercise requires exactly 2 processes
    if (size != 2) {
        if (rank == 0)
            std::cerr << "ERROR: Use exactly 2 processes!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    const int N_ITER = 100;   // number of ping-pongs
    const int TAG_com1 = 10;
    const int TAG_com2 = 20;
    double value = 0.0;       // data traveling back and forth

    double t_start, t_end;

    // Synchronize everyone before starting the measurement
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();  // high-resolution MPI timer

    for (int i = 0; i < N_ITER; i++) {

        if (rank == 0) {
            // --- Process 0: send then receive ---
            value = static_cast<double>(i);    // Convert i from integer to double
            MPI_Send(&value, 1, MPI_DOUBLE, 1, TAG_com1, MPI_COMM_WORLD);
            MPI_Recv(&value, 1, MPI_DOUBLE, 1, TAG_com2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        } else {
            // --- Process 1: receive, increment, resend ---
            MPI_Recv(&value, 1, MPI_DOUBLE, 0, TAG_com1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            value += 1.0;  // modify the value before sending it back
            MPI_Send(&value, 1, MPI_DOUBLE, 0, TAG_com2, MPI_COMM_WORLD);
        }
    }

    t_end = MPI_Wtime();

    // Only process 0 prints the results (it owns the complete timer)
    if (rank == 0) {
        double total_time = t_end - t_start;
        // Each ping-pong is one round trip = 2 messages
        // One-way latency ≈ RTT / 2
        double latency_us = (total_time / N_ITER) * 1e6 / 2.0;

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "\n=== Ping-Pong Results ===\n";
        std::cout << "  Iterations:          " << N_ITER << "\n";
        std::cout << "  Total time:          " << total_time << " s\n";
        std::cout << "  Time per ping-pong:  " << (total_time / N_ITER) * 1e6 << " µs\n";
        std::cout << "  Estimated latency:   " << latency_us << " µs\n";
        std::cout << "  Final value:         "
                  << value
                  << " (expected: "
                  << N_ITER
                  << ")\n";
    }

    MPI_Finalize();
    return 0;
}
