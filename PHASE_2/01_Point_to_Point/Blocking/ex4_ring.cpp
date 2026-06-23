// =============================================================
// ESERCIZIO 4 — Ring Communication (comunicazione ad anello)
// =============================================================
// Ogni processo invia un token al successivo:
//   rank → (rank+1) % size
// Il processo 0 inizia con token=0, ogni passaggio lo incrementa.
// Alla fine, il processo 0 riceve il token passato da tutti.
//
// Compilazione:  mpicxx -O2 -Wall -o ex4_ring ex4_ring.cpp
// Esecuzione:    mpirun -np 4 ./ex4_ring
// =============================================================

#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Rank del vicino a sinistra (da cui ricevo) e a destra (a cui invio)
    int sinistra = (rank - 1 + size) % size;  // chi mi invia
    int destra   = (rank + 1) % size;          // a chi invio

    int token = 0;
    const int TAG = 99;

    if (rank == 0) {
        // -------------------------------------------------------
        // Processo 0: parte con token=0, invia a destra,
        // aspetta che torni dall'ultimo processo (sinistra = size-1)
        // -------------------------------------------------------
        token = 0;
        std::cout << "Processo 0: inizio con token = " << token
                  << ", invio al processo " << destra << std::endl;

        MPI_Send(&token, 1, MPI_INT, destra, TAG, MPI_COMM_WORLD);
        MPI_Recv(&token, 1, MPI_INT, sinistra, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::cout << "Processo 0: token finale ricevuto = " << token << std::endl;
        std::cout << "Valore atteso: " << size - 1 << std::endl;
        std::cout << (token == size - 1 ? "✓ CORRETTO" : "✗ ERRORE") << std::endl;

    } else {
        // -------------------------------------------------------
        // Processi 1..size-1: ricevono, incrementano, reinviano
        // -------------------------------------------------------
        MPI_Recv(&token, 1, MPI_INT, sinistra, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::cout << "Processo " << rank << ": ricevuto token = " << token
                  << " da processo " << sinistra;

        token += 1;  // ogni processo aggiunge 1 al token

        std::cout << ", invio " << token << " al processo " << destra << std::endl;

        MPI_Send(&token, 1, MPI_INT, destra, TAG, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
