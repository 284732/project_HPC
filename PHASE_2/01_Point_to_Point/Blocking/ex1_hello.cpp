// =============================================================
// EXERCISE 1 — Hello with MPI_Send / MPI_Recv
// =============================================================
// Process 0 (master) sends a string to all other processes.
// Each slave process receives it and prints it together with
// its own rank.
//
// Compilation:  mpicxx -O2 -o ex1_hello ex1_hello.cpp
// Execution:    mpirun -np 4 ./ex1_hello
// =============================================================

#include <mpi.h>
#include <iostream>
#include <cstring>   // for strlen

int main(int argc, char* argv[]) {

    // --- MPI initialization ---
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // who am I?
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // total number of processes

    // Message buffer (large enough for all processes)
    const int MAX_LEN = 64;
    char message[MAX_LEN];

    // TAG chosen arbitrarily to identify this message type
    const int TAG_GREETING = 42;

    if (rank == 0) {
        // -------------------------------------------------------
        // MASTER: builds the message and sends it to everyone
        // -------------------------------------------------------
        snprintf(message, MAX_LEN, "Hello from process 0!");

        for (int dest = 1; dest < size; dest++) {
            // MPI_Send(buffer, count, datatype, destination, tag, communicator)
            MPI_Send(
                message,              // data to send
                strlen(message) + 1,  // length including '\0'
                MPI_CHAR,             // element type
                dest,                 // destination rank
                TAG_GREETING,         // tag
                MPI_COMM_WORLD        // global communicator
            );
        }

        std::cout << "[Process 0] I sent the greeting to "
                  << (size - 1) << " processes." << std::endl;

    } else {
        // -------------------------------------------------------
        // SLAVE: receives the message from process 0
        // -------------------------------------------------------
        MPI_Status status;  // structure containing received message info

        // MPI_Recv(buffer, max_count, datatype, source, tag, comm, &status)
        MPI_Recv(
            message,        // where to store the data
            MAX_LEN,        // maximum buffer size
            MPI_CHAR,
            0,              // receive only from process 0
            TAG_GREETING,   // accept only messages with this tag
            MPI_COMM_WORLD,
            &status         // message information (sender, tag, length)
        );

        // status.MPI_SOURCE tells who sent the message
        std::cout << "[Process " << rank << "] Message received from "
                  << status.MPI_SOURCE << ": \"" << message << "\""
                  << std::endl;
    }

    // --- Final synchronization (optional but useful for clean output) ---
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}
