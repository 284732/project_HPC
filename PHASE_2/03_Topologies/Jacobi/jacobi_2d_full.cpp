// =============================================================
// JACOBI 2D FULL — Decomposizione 2D con topologia cartesiana
// =============================================================
// Versione completa con:
//  - MPI_Cart_create (griglia 2D di processi)
//  - MPI_Dims_create (decomposizione automatica)
//  - Halo exchange su tutti e 4 i lati
//  - Convergenza con MPI_Allreduce
//
// Ogni processo gestisce un sotto-dominio rettangolare.
//
// Compilazione:  mpicxx -O2 -Wall -o jacobi2d jacobi_2d_full.cpp
// Esecuzione:    mpirun -np 4 ./jacobi2d
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank_world, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_world);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Parametri globali
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    const int N       = 128;    // griglia globale N x N
    const double EPS  = 1e-5;
    const int MAX_ITER = 5000;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Topologia cartesiana 2D
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int dims[2]    = {0, 0};    // MPI_Dims_create li calcola
    int periods[2] = {0, 0};   // bordi non periodici
    MPI_Dims_create(size, 2, dims);

    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &comm_cart);

    int rank;
    MPI_Comm_rank(comm_cart, &rank);

    int coords[2];
    MPI_Cart_coords(comm_cart, rank, 2, coords);
    int my_row = coords[0];  // posizione riga nella griglia di processi
    int my_col = coords[1];  // posizione colonna

    // Numero di righe/colonne per ogni processo (divisione equa)
    // (per semplicità assumiamo N divisibile per dims)
    int local_rows = N / dims[0];
    int local_cols = N / dims[1];

    // Prima cella globale gestita da questo processo
    int global_row_start = my_row * local_rows;
    int global_col_start = my_col * local_cols;

    if (rank == 0) {
        std::cout << "Griglia globale: " << N << "x" << N << "\n";
        std::cout << "Processi: " << dims[0] << "x" << dims[1]
                  << " (" << size << " totali)\n";
        std::cout << "Sotto-dominio per processo: " << local_rows << "x" << local_cols << "\n\n";
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Vicini (possono essere MPI_PROC_NULL ai bordi)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int nord, sud, ovest, est, dummy;
    MPI_Cart_shift(comm_cart, 0, -1, &dummy, &nord);
    MPI_Cart_shift(comm_cart, 0, +1, &dummy, &sud);
    MPI_Cart_shift(comm_cart, 1, -1, &dummy, &ovest);
    MPI_Cart_shift(comm_cart, 1, +1, &dummy, &est);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Allocazione: sotto-dominio + ghost (1 cella per lato)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int nr = local_rows + 2;  // righe con ghost
    int nc = local_cols + 2;  // colonne con ghost
    std::vector<double> u(nr * nc, 0.0);
    std::vector<double> u_new(nr * nc, 0.0);

    auto idx = [&](int i, int j) { return i * nc + j; };

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Condizioni al bordo: bordo inferiore = 1.0
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    bool has_south_boundary = (my_row == dims[0] - 1);
    if (has_south_boundary) {
        for (int j = 1; j <= local_cols; j++)
            u[idx(local_rows, j)] = 1.0;
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Tipi MPI per colonne (halo orizzontale)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // I dati di una colonna non sono contigui in memoria → creiamo un tipo MPI
    MPI_Datatype col_type;
    MPI_Type_vector(local_rows, 1, nc, MPI_DOUBLE, &col_type);
    MPI_Type_commit(&col_type);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Loop di Jacobi
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double t0 = MPI_Wtime();
    int iter = 0;
    double variazione_globale = 1.0;

    const int TAG1=1, TAG2=2, TAG3=3, TAG4=4;

    while (variazione_globale > EPS && iter < MAX_ITER) {

        // --- Halo Exchange Nord-Sud (righe) ---
        MPI_Sendrecv(&u[idx(1, 1)],          local_cols, MPI_DOUBLE, nord, TAG1,
                     &u[idx(0, 1)],          local_cols, MPI_DOUBLE, nord, TAG2,
                     comm_cart, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&u[idx(local_rows, 1)], local_cols, MPI_DOUBLE, sud,  TAG2,
                     &u[idx(nr-1, 1)],       local_cols, MPI_DOUBLE, sud,  TAG1,
                     comm_cart, MPI_STATUS_IGNORE);

        // --- Halo Exchange Est-Ovest (colonne con tipo derivato) ---
        MPI_Sendrecv(&u[idx(1, 1)],           1, col_type, ovest, TAG3,
                     &u[idx(1, nc-1)],        1, col_type, est,   TAG3,
                     comm_cart, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&u[idx(1, local_cols)],  1, col_type, est,   TAG4,
                     &u[idx(1, 0)],           1, col_type, ovest, TAG4,
                     comm_cart, MPI_STATUS_IGNORE);

        // --- Aggiornamento Jacobi ---
        double var_locale = 0.0;

        for (int i = 1; i <= local_rows; i++) {
            int ig = global_row_start + (i - 1);
            for (int j = 1; j <= local_cols; j++) {
                int jg = global_col_start + (j - 1);

                // Mantieni le condizioni al bordo
                if (ig == 0 || ig == N-1 || jg == 0 || jg == N-1) {
                    u_new[idx(i,j)] = u[idx(i,j)];
                    continue;
                }

                u_new[idx(i,j)] = 0.25 * (
                    u[idx(i-1,j)] + u[idx(i+1,j)] +
                    u[idx(i,j-1)] + u[idx(i,j+1)]
                );

                double diff = std::abs(u_new[idx(i,j)] - u[idx(i,j)]);
                var_locale = std::max(var_locale, diff);
            }
        }

        // --- Convergenza globale ---
        MPI_Allreduce(&var_locale, &variazione_globale, 1, MPI_DOUBLE, MPI_MAX, comm_cart);

        std::swap(u, u_new);
        iter++;

        if (rank == 0 && iter % 500 == 0) {
            std::cout << "[Iter " << std::setw(5) << iter << "] max_diff = "
                      << std::scientific << variazione_globale << "\n";
        }
    }

    double t1 = MPI_Wtime();

    if (rank == 0) {
        std::cout << "\n" << (variazione_globale <= EPS ? "✓ CONVERGENZA" : "⚠ MAX_ITER")
                  << " dopo " << iter << " iterazioni\n"
                  << "  Variazione finale: " << variazione_globale << "\n"
                  << "  Tempo totale: " << std::fixed << std::setprecision(3)
                  << (t1 - t0) << " s\n";
    }

    MPI_Type_free(&col_type);
    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}
