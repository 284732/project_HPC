// =============================================================
// ESERCIZIO 2 — MPI_Scatter + MPI_Gather: somma distribuita
// =============================================================
// Il root ha un vettore di N elementi (N multiplo di size).
// Lo scattera: ogni processo riceve N/size elementi.
// Ogni worker calcola la somma della propria parte.
// Il root raccoglie i risultati con Gather.
//
// Compilazione:  mpicxx -O2 -Wall -o ex2_scatter ex2_scatter_gather.cpp
// Esecuzione:    mpirun -np 4 ./ex2_scatter
// =============================================================

#include <mpi.h>
#include <iostream>
#include <vector>
#include <numeric>   // per std::iota e std::accumulate
#include <iomanip>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int N = 12;  // dimensione totale (deve essere multiplo di size)

    if (N % size != 0) {
        if (rank == 0)
            std::cerr << "ERRORE: N=" << N << " non è multiplo di size=" << size << std::endl;
        MPI_Finalize();
        return 1;
    }

    int chunk = N / size;  // quanti elementi riceve ogni processo

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 1: Il root inizializza il vettore globale
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::vector<double> vettore_globale;  // solo il root lo riempie
    if (rank == 0) {
        vettore_globale.resize(N);
        for (int i = 0; i < N; i++) vettore_globale[i] = i + 1.0;  // 1,2,...,N

        std::cout << "Vettore globale: ";
        for (double x : vettore_globale) std::cout << x << " ";
        std::cout << "\n";
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 2: Scatter → ogni processo riceve 'chunk' elementi
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::vector<double> mia_parte(chunk);

    // MPI_Scatter(sendbuf, sendcount, sendtype,
    //             recvbuf, recvcount, recvtype, root, comm)
    // - sendcount: quanti elementi inviare A CIASCUN processo
    // - recvcount: quanti elementi riceve ogni processo (= sendcount)
    MPI_Scatter(
        vettore_globale.data(), chunk, MPI_DOUBLE,  // sorgente (solo root usa questo)
        mia_parte.data(),       chunk, MPI_DOUBLE,  // destinazione (tutti)
        0, MPI_COMM_WORLD
    );

    std::cout << "[Processo " << rank << "] ho ricevuto: ";
    for (double x : mia_parte) std::cout << x << " ";
    std::cout << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 3: Ogni processo elabora la propria parte
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    double somma_locale = 0.0;
    for (double x : mia_parte) somma_locale += x * x;  // somma dei quadrati

    std::cout << "[Processo " << rank << "] somma quadrati locale = "
              << somma_locale << std::endl;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 4: Gather → root raccoglie tutti i risultati
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::vector<double> risultati(size);  // solo il root lo usa

    // MPI_Gather: ognuno invia 1 double, il root riceve size double
    MPI_Gather(
        &somma_locale, 1, MPI_DOUBLE,  // cosa invia ciascuno
        risultati.data(), 1, MPI_DOUBLE,  // dove il root raccoglie
        0, MPI_COMM_WORLD
    );

    if (rank == 0) {
        double totale = 0.0;
        std::cout << "\n[Root] Risultati raccolti:\n";
        for (int i = 0; i < size; i++) {
            std::cout << "  Processo " << i << ": " << risultati[i] << "\n";
            totale += risultati[i];
        }
        std::cout << "Somma totale quadrati = " << totale << "\n";

        // Verifica: somma(i^2) per i=1..N = N(N+1)(2N+1)/6
        double atteso = static_cast<double>(N) * (N+1) * (2*N+1) / 6.0;
        std::cout << "Valore atteso          = " << atteso << "\n";
        std::cout << (std::abs(totale - atteso) < 1e-9 ? "✓ CORRETTO" : "✗ ERRORE") << "\n";
    }

    MPI_Finalize();
    return 0;
}
