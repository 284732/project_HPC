// =============================================================
// ESERCIZIO 1 — MPI_Cart_create: griglia 2D e stampa vicini
// =============================================================
// Crea una topologia virtuale cartesiana 2D.
// Ogni processo stampa le proprie coordinate e i rank dei vicini
// (Nord, Sud, Est, Ovest). Bordi non periodici → MPI_PROC_NULL.
//
// Compilazione:  mpicxx -O2 -Wall -o ex1_cart ex1_cart_create.cpp
// Esecuzione:    mpirun -np 6 ./ex1_cart
// =============================================================

#include <mpi.h>
#include <iostream>
#include <string>

// Funzione di utilità: converte un rank in stringa leggibile
std::string rank_str(int r) {
    if (r == MPI_PROC_NULL) return "NULL";
    return std::to_string(r);
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 1: Definisci le dimensioni della griglia
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // Con 6 processi proviamo una griglia 2x3
    int ndims = 2;
    int dims[2]    = {2, 3};   // righe=2, colonne=3
    int periods[2] = {0, 0};   // non periodico (bordi "fisici")
    int reorder    = 1;        // MPI può ottimizzare il mapping

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 2: Crea il communicator cartesiano
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, reorder, &comm_cart);

    // Rank nel nuovo communicator (potrebbe differire da MPI_COMM_WORLD se reorder=1)
    int rank_cart;
    MPI_Comm_rank(comm_cart, &rank_cart);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 3: Ottieni le coordinate del processo corrente
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    int coords[2];
    MPI_Cart_coords(comm_cart, rank_cart, ndims, coords);
    // coords[0] = riga, coords[1] = colonna

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PASSO 4: Trova i vicini con MPI_Cart_shift
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MPI_Cart_shift(comm, direction, disp, &rank_src, &rank_dst)
    // - direction=0 → dimensione 0 (righe = verticale = Nord/Sud)
    // - direction=1 → dimensione 1 (colonne = orizzontale = Est/Ovest)
    // - disp=+1     → avanza nella dimensione (Sud / Est)
    // - disp=-1     → arretra nella dimensione (Nord / Ovest)
    // Se il vicino non esiste (bordo non periodico), rank = MPI_PROC_NULL

    int vicino_nord, vicino_sud;
    MPI_Cart_shift(comm_cart, 0, -1, &vicino_sud, &vicino_nord);  // dim 0, disp=-1: vai su
    MPI_Cart_shift(comm_cart, 0, +1, &vicino_nord, &vicino_sud);  // dim 0, disp=+1: vai giù

    // Nota: Cart_shift(comm, dir, disp, &src, &dst)
    //   src = chi mi manderebbe dati se io facessi Recv (= il vicino "da cui vengo")
    //   dst = a chi invio se faccio Send (= il vicino "verso cui vado")
    // Per trovare Nord (chi è sopra): mi "sposto" di -1 nella dir 0
    //   → source = il vicino a cui mi avvicino spostandomi, dest = da cui vengo
    // È un po' controintuitivo: facciamo la versione esplicita:

    int nord, sud, est, ovest;
    int dummy;  // il "source" di Cart_shift che qui non ci serve

    // Vicino NORD (riga - 1): shift di -1 nella direzione 0
    MPI_Cart_shift(comm_cart, 0, -1, &dummy, &nord);
    // Vicino SUD  (riga + 1): shift di +1 nella direzione 0
    MPI_Cart_shift(comm_cart, 0, +1, &dummy, &sud);
    // Vicino OVEST (col - 1): shift di -1 nella direzione 1
    MPI_Cart_shift(comm_cart, 1, -1, &dummy, &ovest);
    // Vicino EST  (col + 1): shift di +1 nella direzione 1
    MPI_Cart_shift(comm_cart, 1, +1, &dummy, &est);

    // Sincronizza l'output per renderlo leggibile
    MPI_Barrier(comm_cart);

    std::cout << "P" << rank_cart
              << " coords(" << coords[0] << "," << coords[1] << ")"
              << " | N=" << rank_str(nord)
              << " S=" << rank_str(sud)
              << " O=" << rank_str(ovest)
              << " E=" << rank_str(est)
              << std::endl;

    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}
