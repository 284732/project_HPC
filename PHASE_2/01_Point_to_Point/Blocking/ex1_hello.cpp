// =============================================================
// ESERCIZIO 1 — Hello con MPI_Send / MPI_Recv
// =============================================================
// Il processo 0 (master) invia una stringa a tutti gli altri.
// Ogni processo slave la riceve e la stampa con il proprio rank.
//
// Compilazione:  mpicxx -O2 -Wall -o ex1_hello ex1_hello.cpp
// Esecuzione:    mpirun -np 4 ./ex1_hello
// =============================================================

#include <mpi.h>
#include <iostream>
#include <cstring>   // per strlen

int main(int argc, char* argv[]) {

    // --- Inizializzazione MPI ---
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // chi sono io?
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // quanti siamo in totale?

    // Buffer per il messaggio (abbastanza grande per tutti i processi)
    const int MAX_LEN = 64;
    char messaggio[MAX_LEN];

    // TAG scelto arbitrariamente per identificare questo tipo di messaggio
    const int TAG_SALUTO = 42;

    if (rank == 0) {
        // -------------------------------------------------------
        // MASTER: costruisce il messaggio e lo invia a tutti
        // -------------------------------------------------------
        snprintf(messaggio, MAX_LEN, "Ciao dal processo 0!");

        for (int dest = 1; dest < size; dest++) {
            // MPI_Send(buffer, count, tipo, destinatario, tag, communicator)
            MPI_Send(
                messaggio,        // cosa inviare
                strlen(messaggio) + 1,  // lunghezza incluso '\0'
                MPI_CHAR,         // tipo degli elementi
                dest,             // rank destinatario
                TAG_SALUTO,       // tag
                MPI_COMM_WORLD    // communicator globale
            );
        }

        std::cout << "[Processo 0] Ho inviato il saluto a "
                  << (size - 1) << " processi." << std::endl;

    } else {
        // -------------------------------------------------------
        // SLAVE: riceve il messaggio dal processo 0
        // -------------------------------------------------------
        MPI_Status status;  // struttura con info sul messaggio ricevuto

        // MPI_Recv(buffer, count_max, tipo, sorgente, tag, comm, &status)
        MPI_Recv(
            messaggio,    // dove salvare i dati
            MAX_LEN,      // dimensione massima del buffer
            MPI_CHAR,
            0,            // ricevo solo dal processo 0
            TAG_SALUTO,   // accetto solo messaggi con questo tag
            MPI_COMM_WORLD,
            &status       // info sul messaggio (mittente, tag, lunghezza)
        );

        // status.MPI_SOURCE dice chi ha inviato il messaggio
        std::cout << "[Processo " << rank << "] Messaggio ricevuto da "
                  << status.MPI_SOURCE << ": \"" << messaggio << "\""
                  << std::endl;
    }

    // --- Sincronizzazione finale (facoltativa ma utile per output pulito) ---
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}
