// =============================================================
// ESERCIZIO 3 — MPI_Waitall con comunicazioni multiple
// =============================================================
// Il processo 0 (master) invia N messaggi distinti a N worker
// usando MPI_Isend per tutti, poi aspetta con MPI_Waitall.
// I worker usano MPI_Irecv + MPI_Wait.
//
// Compilazione:  mpicxx -O2 -Wall -o ex3_waitall ex3_waitall.cpp
// Esecuzione:    mpirun -np 4 ./ex3_waitall
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
        // MASTER: lancia tutti gli Isend in sequenza
        // -------------------------------------------------------
        int n_worker = size - 1;

        // Preparo i dati da inviare a ciascun worker
        std::vector<double> dati(n_worker);
        for (int i = 0; i < n_worker; i++)
            dati[i] = (i + 1) * 100.0;

        // Vettore di request: una per ogni Isend
        std::vector<MPI_Request> requests(n_worker);

        std::cout << "[Master] Avvio " << n_worker << " Isend non bloccanti...\n";
        for (int w = 0; w < n_worker; w++) {
            MPI_Isend(&dati[w], 1, MPI_DOUBLE, w + 1, TAG, MPI_COMM_WORLD, &requests[w]);
        }

        std::cout << "[Master] Faccio altro lavoro mentre i dati viaggiano...\n";
        // (simulazione di computazione utile)
        double somma = 0.0;
        for (int i = 0; i < 1000000; i++) somma += i * 0.001;
        std::cout << "[Master] Lavoro completato (somma fittiza = " << somma << ")\n";

        // Aspetta che TUTTI gli Isend siano completati
        // MPI_Waitall(count, array_of_requests, array_of_statuses)
        MPI_Waitall(n_worker, requests.data(), MPI_STATUSES_IGNORE);
        std::cout << "[Master] MPI_Waitall: tutte le comunicazioni completate.\n";

    } else {
        // -------------------------------------------------------
        // WORKER: riceve il proprio dato dal master
        // -------------------------------------------------------
        double valore;
        MPI_Request req;

        // Avvio ricezione non bloccante
        MPI_Irecv(&valore, 1, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD, &req);

        // Potrei fare altro qui...

        // Aspetto che la ricezione sia completa
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        std::cout << "[Worker " << rank << "] Ricevuto: " << valore
                  << " (atteso: " << rank * 100.0 << ")"
                  << std::endl;
    }

    MPI_Finalize();
    return 0;
}
