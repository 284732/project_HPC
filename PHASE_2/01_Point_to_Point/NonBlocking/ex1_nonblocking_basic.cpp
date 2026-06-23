// =============================================================
// ESERCIZIO 1 — Comunicazione non bloccante: Isend/Irecv base
// =============================================================
// Riscrittura del ping-pong con operazioni non bloccanti.
// Mostra come usare MPI_Request e MPI_Wait.
//
// Compilazione:  mpicxx -O2 -Wall -o ex1_nonblocking ex1_nonblocking_basic.cpp
// Esecuzione:    mpirun -np 2 ./ex1_nonblocking
// =============================================================

#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) std::cerr << "Usa 2 processi!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    const int N_ITER = 5;
    const int TAG = 7;
    int altro = 1 - rank;

    double buf_send, buf_recv;

    for (int i = 0; i < N_ITER; i++) {

        // Ogni processo prepara il suo valore da inviare
        buf_send = rank * 100.0 + i;

        // MPI_Request: handle dell'operazione in corso
        MPI_Request req_send, req_recv;

        // --- Avvio non bloccante di send e recv ---
        // Nota: entrambi avviano subito, senza bloccarsi!
        MPI_Isend(&buf_send, 1, MPI_DOUBLE, altro, TAG, MPI_COMM_WORLD, &req_send);
        MPI_Irecv(&buf_recv, 1, MPI_DOUBLE, altro, TAG, MPI_COMM_WORLD, &req_recv);

        // *** Qui si potrebbe fare del lavoro utile! ***
        // (In questo esempio non c'è lavoro, ma la struttura è quella giusta)

        // --- Aspetta che ENTRAMBE le operazioni siano terminate ---
        MPI_Wait(&req_send, MPI_STATUS_IGNORE);
        MPI_Wait(&req_recv, MPI_STATUS_IGNORE);

        // Ora buf_recv contiene il valore dell'altro processo
        std::cout << "Iterazione " << i
                  << " | Processo " << rank
                  << " | inviato: " << buf_send
                  << " | ricevuto: " << buf_recv
                  << std::endl;
    }

    MPI_Finalize();
    return 0;
}
