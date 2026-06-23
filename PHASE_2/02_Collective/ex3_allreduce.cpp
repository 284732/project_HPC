// =============================================================
// ESERCIZIO 3 — MPI_Allreduce: norma L2 di un vettore distribuito
// =============================================================
// Ogni processo ha una parte di un vettore globale.
// Calcola la norma L2: ||v|| = sqrt( sum(v_i^2) )
// Con Allreduce TUTTI i processi ottengono il risultato finale.
// Utile quando ogni processo ha bisogno del risultato globale
// (es: test di convergenza in Jacobi).
//
// Compilazione:  mpicxx -O2 -Wall -o ex3_allreduce ex3_allreduce.cpp
// Esecuzione:    mpirun -np 4 ./ex3_allreduce
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int N_LOCALE = 5;  // elementi per processo

    // Ogni processo inizializza la propria parte del vettore
    // Processo k ha: [k*N_LOCALE+1, ..., k*N_LOCALE+N_LOCALE]
    std::vector<double> v_locale(N_LOCALE);
    for (int i = 0; i < N_LOCALE; i++)
        v_locale[i] = static_cast<double>(rank * N_LOCALE + i + 1);

    std::cout << "[Processo " << rank << "] parte locale: ";
    for (double x : v_locale) std::cout << x << " ";
    std::cout << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 1: calcola il contributo locale alla norma al quadrato
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double norma_locale_sq = 0.0;
    for (double x : v_locale)
        norma_locale_sq += x * x;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 2: Allreduce → tutti ottengono la norma globale
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MPI_Reduce darebbe il risultato solo al root.
    // MPI_Allreduce lo dà a TUTTI → nessuno deve fare una Bcast aggiuntiva!
    double norma_globale_sq = 0.0;
    MPI_Allreduce(&norma_locale_sq, &norma_globale_sq,
                  1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    double norma = std::sqrt(norma_globale_sq);

    // Ogni processo ha il risultato → tutti possono stamparlo
    std::cout << "[Processo " << rank << "] norma L2 globale = "
              << std::setprecision(6) << norma << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // BONUS: MPI_Allreduce per trovare il massimo globale
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double max_locale = *std::max_element(v_locale.begin(), v_locale.end());
    double max_globale = 0.0;
    MPI_Allreduce(&max_locale, &max_globale, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    if (rank == 0)
        std::cout << "\n[Root] Massimo globale = " << max_globale
                  << " (atteso: " << static_cast<double>(size * N_LOCALE) << ")"
                  << std::endl;

    MPI_Finalize();
    return 0;
}
