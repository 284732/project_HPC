// =============================================================
// JACOBI 1D — Solver iterativo con decomposizione a strisce
// =============================================================
// Risolve l'equazione di Laplace su griglia N x N.
// Ogni processo gestisce N/size righe + 2 ghost rows.
//
// Condizioni al bordo:
//   - Bordo inferiore (riga N-1): u = 1.0  (caldo)
//   - Tutti gli altri bordi:      u = 0.0
//
// Compilazione:  mpicxx -O2 -Wall -o jacobi1d jacobi_1d_strips.cpp
// Esecuzione:    mpirun -np 4 ./jacobi1d
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>  // per std::max

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Parametri del problema
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    const int N     = 64;       // dimensione globale della griglia (N x N)
    const double EPS = 1e-6;   // soglia di convergenza
    const int MAX_ITER = 10000;

    if (N % size != 0) {
        if (rank == 0)
            std::cerr << "ERRORE: N=" << N << " non divisibile per size=" << size << std::endl;
        MPI_Finalize();
        return 1;
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Decomposizione del dominio
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int rows_locale = N / size;      // righe reali per processo
    int row_start   = rank * rows_locale;   // prima riga globale
    int row_end     = row_start + rows_locale - 1;  // ultima riga globale

    // Array locale: righe_reali + 2 ghost rows = rows_locale+2 totali
    // Indici: 0 = ghost nord, 1..rows_locale = reali, rows_locale+1 = ghost sud
    int n_rows_loc = rows_locale + 2;
    std::vector<double> u(n_rows_loc * N, 0.0);     // soluzione corrente
    std::vector<double> u_new(n_rows_loc * N, 0.0); // nuova iterazione

    // Macro per accesso 2D: u[i][j] → u[i*N + j]
    auto idx = [&](int i, int j) { return i * N + j; };

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Condizioni al bordo (Dirichlet)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Bordo inferiore globale (riga N-1): u = 1.0
    // Calcoliamo quale processo "possiede" la riga N-1
    if (row_end == N - 1) {
        for (int j = 0; j < N; j++)
            u[idx(rows_locale, j)] = 1.0;  // ultima riga reale = bordo caldo
    }

    // Bordo sinistro e destro: u = 0.0 (già inizializzati)
    // Bordo superiore (riga 0): u = 0.0 (già inizializzati)

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Identificazione vicini
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int vicino_nord = (rank > 0)        ? rank - 1 : MPI_PROC_NULL;
    int vicino_sud  = (rank < size - 1) ? rank + 1 : MPI_PROC_NULL;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Loop di Jacobi
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double t_start = MPI_Wtime();
    int iter = 0;
    double variazione_globale = 1.0;

    const int TAG_NS = 10, TAG_SN = 11;

    while (variazione_globale > EPS && iter < MAX_ITER) {

        // --- 1. Halo Exchange ---
        // Invio prima riga reale a Nord, ricevo ghost nord da Nord
        MPI_Sendrecv(
            &u[idx(1, 0)],           N, MPI_DOUBLE, vicino_nord, TAG_NS,
            &u[idx(0, 0)],           N, MPI_DOUBLE, vicino_nord, TAG_SN,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );
        // Invio ultima riga reale a Sud, ricevo ghost sud da Sud
        MPI_Sendrecv(
            &u[idx(rows_locale, 0)],   N, MPI_DOUBLE, vicino_sud, TAG_SN,
            &u[idx(rows_locale+1, 0)], N, MPI_DOUBLE, vicino_sud, TAG_NS,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );

        // --- 2. Aggiornamento Jacobi ---
        double variazione_locale = 0.0;

        for (int i = 1; i <= rows_locale; i++) {
            int i_globale = row_start + (i - 1);

            for (int j = 0; j < N; j++) {
                // Salta i bordi sinistro e destro
                if (j == 0 || j == N - 1) {
                    u_new[idx(i, j)] = u[idx(i, j)];
                    continue;
                }
                // Salta il bordo superiore globale
                if (i_globale == 0) {
                    u_new[idx(i, j)] = 0.0;
                    continue;
                }
                // Salta il bordo inferiore globale (temperatura fissa)
                if (i_globale == N - 1) {
                    u_new[idx(i, j)] = 1.0;
                    continue;
                }

                // Stencil di Jacobi a 5 punti
                u_new[idx(i, j)] = 0.25 * (
                    u[idx(i-1, j)] +  // nord
                    u[idx(i+1, j)] +  // sud
                    u[idx(i, j-1)] +  // ovest
                    u[idx(i, j+1)]    // est
                );

                double diff = std::abs(u_new[idx(i, j)] - u[idx(i, j)]);
                variazione_locale = std::max(variazione_locale, diff);
            }
        }

        // --- 3. Controllo convergenza globale ---
        // Allreduce con MPI_MAX: tutti ottengono la variazione massima globale
        MPI_Allreduce(&variazione_locale, &variazione_globale,
                      1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        // --- 4. Scambia u e u_new ---
        std::swap(u, u_new);

        iter++;

        // Stampa progress ogni 100 iterazioni (solo il processo 0)
        if (rank == 0 && iter % 100 == 0) {
            std::cout << "[Iter " << std::setw(5) << iter << "] "
                      << "variazione max = " << std::scientific << std::setprecision(4)
                      << variazione_globale << std::endl;
        }
    }

    double t_end = MPI_Wtime();

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Risultati finali
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    if (rank == 0) {
        if (variazione_globale <= EPS) {
            std::cout << "\n✓ CONVERGENZA raggiunta in " << iter << " iterazioni!\n";
        } else {
            std::cout << "\n⚠ MAX_ITER raggiunto senza convergenza.\n";
        }
        std::cout << "  Variazione finale: " << variazione_globale << "\n";
        std::cout << "  Tempo totale:      " << std::fixed << std::setprecision(3)
                  << (t_end - t_start) << " secondi\n";
        std::cout << "  Processi usati:    " << size << "\n";
    }

    MPI_Finalize();
    return 0;
}
