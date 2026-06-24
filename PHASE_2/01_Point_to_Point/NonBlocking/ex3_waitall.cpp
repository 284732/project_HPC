// =============================================================
// EXERCISE 3 — MPI_Waitall with Multiple Communications
// =============================================================
// Process 0 (master) sends N distinct messages to N workers
// using MPI_Isend for all of them, then waits with MPI_Waitall.
// Workers use MPI_Irecv + MPI_Wait.
//
// Compilation:  mpicxx -O2 -Wall -o ex3_waitall ex3_waitall.cpp
// Execution:    mpirun -np 4 ./ex3_waitall
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int TAG = 20;

    if (rank == 0) {
        // -------------------------------------------------------
        // MASTER: launches all Isend operations in sequence
        // -------------------------------------------------------
        int n_worker = size - 1;

        // Prepare the data to send to each worker
        std::vector<double> data(n_worker);
        for (int i = 0; i < n_worker; i++)
            data[i] = (i + 1) * 100.0;

        // Request vector: one request for each Isend
        std::vector<MPI_Request> requests(n_worker);

        std::cout << "[Master] Starting " << n_worker
                  << " non-blocking Isend operations...\n";

        for (int w = 0; w < n_worker; w++) {
            MPI_Isend(&data[w], 1, MPI_DOUBLE, w + 1, TAG,
                      MPI_COMM_WORLD, &requests[w]);
        }

        std::cout << "[Master] Doing other work while the data is in transit...\n";

        // (simulation of useful computation)
        double sum = 0.0;
        for (int i = 0; i < 1000000; i++) sum += i * 0.001;

        std::cout << "[Master] Work completed (dummy sum = "
                  << sum << ")\n";

        // Wait until ALL Isend operations have completed
        // MPI_Waitall(count, array_of_requests, array_of_statuses)
        MPI_Waitall(n_worker, requests.data(), MPI_STATUSES_IGNORE);

        std::cout << "[Master] MPI_Waitall: all communications completed.\n";

    } else {
        // -------------------------------------------------------
        // WORKER: receives its data from the master
        // -------------------------------------------------------
        double value;
        MPI_Request req;

        // Start non-blocking receive
        MPI_Irecv(&value, 1, MPI_DOUBLE, 0, TAG,
                  MPI_COMM_WORLD, &req);

        // Other work could be done here...

        // Wait until the receive operation is complete
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        std::cout << "[Worker " << rank << "] Received: "
                  << value
                  << " (expected: "
                  << rank * 100.0
                  << ")"
                  << std::endl;
    }

    MPI_Finalize();
    return 0;
}
