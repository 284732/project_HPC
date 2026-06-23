// =============================================================
// ESERCIZIO 2 — Halo Exchange su griglia 2D
// =============================================================
// Ogni processo gestisce un sotto-dominio locale (con celle ghost).
// Lo "halo exchange" aggiorna le celle ghost dai vicini.
// Usato in ogni iterazione di Jacobi, Gauss-Seidel, FD, ecc.
//
// Schema memoria locale (4x4 con ghost):
//
//   g g g g g g     ← riga ghost Nord
//   g * * * * g
//   g * * * * g     * = celle reali
//   g * * * * g     g = celle ghost (aggiornate dai vicini)
//   g * * * * g
//   g g g g g g     ← riga ghost Sud
//
// Compilazione:  mpicxx -O2 -Wall -o ex2_halo ex2_halo_exchange.cpp
// Esecuzione:    mpirun -np 4 ./ex2_halo
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <iomanip>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Griglia di processi: assumiamo decomposizione 1D per semplicità
    // (solo scambio Nord-Sud, ogni processo ha una striscia di righe)
    int dims[2]    = {size, 1};  // size righe, 1 colonna
    int periods[2] = {0, 0};
    int reorder    = 0;

    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &comm_cart);

    int rank_cart, coords[2];
    MPI_Comm_rank(comm_cart, &rank_cart);
    MPI_Cart_coords(comm_cart, rank_cart, 2, coords);

    // Vicini Nord e Sud
    int vicino_nord, vicino_sud, dummy;
    MPI_Cart_shift(comm_cart, 0, -1, &dummy, &vicino_nord);
    MPI_Cart_shift(comm_cart, 0, +1, &dummy, &vicino_sud);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 1: Inizializza il dominio locale (con righe ghost)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    const int N_ROWS_LOCALE = 4;  // righe reali per processo
    const int N_COLS        = 6;  // colonne (uguale per tutti)
    // Con ghost: (N_ROWS_LOCALE + 2) righe totali
    int n_rows_totale = N_ROWS_LOCALE + 2;

    // Layout: u[i][j], i=0 ghost nord, i=n_rows_totale-1 ghost sud
    std::vector<std::vector<double>> u(n_rows_totale, std::vector<double>(N_COLS, 0.0));

    // Inizializza le celle reali con il rank del processo
    for (int i = 1; i <= N_ROWS_LOCALE; i++)
        for (int j = 0; j < N_COLS; j++)
            u[i][j] = static_cast<double>(rank_cart);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 2: Halo Exchange (scambio righe di bordo)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Inviamo la nostra prima riga reale al vicino NORD
    // Riceviamo dal vicino NORD nella nostra ghost row nord (riga 0)
    // E viceversa per SUD.

    const int TAG_NS = 1;  // tag per scambio Nord-Sud
    const int TAG_SN = 2;  // tag per scambio Sud-Nord

    // Scambio con il vicino NORD
    // Mando u[1] a nord, ricevo da nord in u[0]
    MPI_Sendrecv(
        u[1].data(), N_COLS, MPI_DOUBLE, vicino_nord, TAG_NS,
        u[0].data(), N_COLS, MPI_DOUBLE, vicino_nord, TAG_SN,
        comm_cart, MPI_STATUS_IGNORE
    );

    // Scambio con il vicino SUD
    // Mando u[N_ROWS_LOCALE] a sud, ricevo da sud in u[N_ROWS_LOCALE+1]
    MPI_Sendrecv(
        u[N_ROWS_LOCALE].data(),   N_COLS, MPI_DOUBLE, vicino_sud, TAG_SN,
        u[N_ROWS_LOCALE+1].data(), N_COLS, MPI_DOUBLE, vicino_sud, TAG_NS,
        comm_cart, MPI_STATUS_IGNORE
    );

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 3: Verifica e stampa
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    MPI_Barrier(comm_cart);

    if (rank_cart == 0) {
        std::cout << "\nDopo l'halo exchange, sottodominio del processo " << rank_cart
                  << " (vicino_nord=" << (vicino_nord == MPI_PROC_NULL ? -1 : vicino_nord)
                  << ", vicino_sud=" << (vicino_sud == MPI_PROC_NULL ? -1 : vicino_sud) << "):\n";

        for (int i = 0; i < n_rows_totale; i++) {
            std::string label = (i == 0) ? " [ghost N]" :
                                (i == n_rows_totale-1) ? " [ghost S]" : " [reale  ]";
            std::cout << label << " riga " << i << ": ";
            for (double v : u[i]) std::cout << std::setw(4) << v;
            std::cout << "\n";
        }
        std::cout << "(Le ghost rows valgono -1 se il vicino non esiste)\n";
    }

    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}
