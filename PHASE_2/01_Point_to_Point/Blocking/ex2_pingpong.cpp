// =============================================================
// ESERCIZIO 2 — Ping-Pong tra due processi + misura del tempo
// =============================================================
// Processo 0 e processo 1 si scambiano un double N volte.
// Misura la latenza media usando MPI_Wtime().
//
// Usa esattamente 2 processi:
// Compilazione:  mpicxx -O2 -Wall -o ex2_pingpong ex2_pingpong.cpp
// Esecuzione:    mpirun -np 2 ./ex2_pingpong
// =============================================================

#include <mpi.h>
#include <iostream>
#include <iomanip>   // per std::setprecision

int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Controllo: questo esercizio richiede esattamente 2 processi
    if (size != 2) {
        if (rank == 0)
            std::cerr << "ERRORE: Usa esattamente 2 processi!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    const int N_ITER = 100;   // numero di ping-pong
    const int TAG    = 10;
    double valore    = 0.0;   // il dato che viaggia avanti e indietro

    double t_start, t_end;

    // Sincronizziamo tutti prima di iniziare la misura
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();  // timer MPI ad alta risoluzione

    for (int i = 0; i < N_ITER; i++) {

        if (rank == 0) {
            // --- Processo 0: invia poi riceve ---
            valore = static_cast<double>(i);
            MPI_Send(&valore, 1, MPI_DOUBLE, 1, TAG, MPI_COMM_WORLD);
            MPI_Recv(&valore, 1, MPI_DOUBLE, 1, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        } else {
            // --- Processo 1: riceve, incrementa, reinvia ---
            MPI_Recv(&valore, 1, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            valore += 1.0;  // modifica il valore prima di rimandarlo
            MPI_Send(&valore, 1, MPI_DOUBLE, 0, TAG, MPI_COMM_WORLD);
        }
    }

    t_end = MPI_Wtime();

    // Solo il processo 0 stampa i risultati (lui ha il timer completo)
    if (rank == 0) {
        double tempo_totale = t_end - t_start;
        // Ogni ping-pong è un "round trip" = 2 messaggi
        // Latenza (one-way) ≈ RTT / 2
        double latenza_us = (tempo_totale / N_ITER) * 1e6 / 2.0;

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "\n=== Risultati Ping-Pong ===\n";
        std::cout << "  Iterazioni:          " << N_ITER << "\n";
        std::cout << "  Tempo totale:        " << tempo_totale << " s\n";
        std::cout << "  Tempo per ping-pong: " << (tempo_totale / N_ITER) * 1e6 << " µs\n";
        std::cout << "  Latenza stimata:     " << latenza_us << " µs\n";
        std::cout << "  Valore finale:       " << valore << " (atteso: " << 2.0*N_ITER - 1 << ")\n";
    }

    MPI_Finalize();
    return 0;
}
