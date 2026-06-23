// =============================================================
// ESERCIZIO 2 — Overlap computazione/comunicazione
// =============================================================
// Ogni processo ha un vettore locale. Invia il proprio vettore
// al vicino mentre contemporaneamente calcola la somma locale.
// Confronta il tempo con la versione bloccante sequenziale.
//
// Compilazione:  mpicxx -O2 -Wall -o ex2_overlap ex2_overlap.cpp
// Esecuzione:    mpirun -np 4 ./ex2_overlap
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <numeric>    // per std::iota
#include <iomanip>

// Simula una computazione pesante: somma un vettore grande
double somma_vettore(const std::vector<double>& v) {
    double s = 0.0;
    for (double x : v) s += x;
    return s;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int N = 100000;  // dimensione del vettore locale
    const int TAG = 5;
    int destra = (rank + 1) % size;
    int sinistra = (rank - 1 + size) % size;

    // Vettore locale: ogni processo ha valori = rank * N + i
    std::vector<double> mio_vettore(N);
    for (int i = 0; i < N; i++)
        mio_vettore[i] = static_cast<double>(rank * N + i);

    std::vector<double> vettore_ricevuto(N);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // VERSIONE NON BLOCCANTE (con overlap)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    MPI_Request reqs[2];
    // Avvio la comunicazione in background
    MPI_Isend(mio_vettore.data(),       N, MPI_DOUBLE, destra,   TAG, MPI_COMM_WORLD, &reqs[0]);
    MPI_Irecv(vettore_ricevuto.data(),  N, MPI_DOUBLE, sinistra, TAG, MPI_COMM_WORLD, &reqs[1]);

    // Mentre la comunicazione avviene, calcolo la somma locale
    double somma_locale = somma_vettore(mio_vettore);

    // Ora aspetto che la comunicazione finisca
    MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);

    double t1 = MPI_Wtime();
    double somma_ricevuta = somma_vettore(vettore_ricevuto);

    if (rank == 0) {
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "\n[Non-bloccante] Tempo: " << (t1 - t0) * 1000.0 << " ms\n";
        std::cout << "  Somma locale:   " << somma_locale << "\n";
        std::cout << "  Somma ricevuta: " << somma_ricevuta << "\n";
    }

    MPI_Finalize();
    return 0;
}
