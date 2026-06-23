// =============================================================
// ESERCIZIO 3 — MPI_Dims_create: decomposizione automatica
// =============================================================
// MPI_Dims_create trova la fattorizzazione più "quadrata" possibile
// per distribuire 'size' processi su una griglia 2D.
//
// Esempi:
//   size=4  → dims = [2, 2]
//   size=6  → dims = [2, 3]
//   size=8  → dims = [2, 4]
//   size=12 → dims = [3, 4]
//
// Compilazione:  mpicxx -O2 -Wall -o ex3_dims ex3_dims_create.cpp
// Esecuzione:    mpirun -np 6 ./ex3_dims
// =============================================================

#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MPI_Dims_create: trova dims ottimale
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // dims deve essere inizializzato a 0 per le dimensioni libere.
    // Se metti un valore >0, quella dimensione è fissa.
    int dims[2] = {0, 0};  // entrambe libere: MPI le determina
    MPI_Dims_create(size, 2, dims);

    if (rank == 0) {
        std::cout << "Processi totali: " << size << "\n";
        std::cout << "Griglia ottimale: " << dims[0] << " x " << dims[1] << "\n\n";
    }

    // Crea la topologia con le dimensioni trovate
    int periods[2] = {0, 0};
    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &comm_cart);

    int rank_cart, coords[2];
    MPI_Comm_rank(comm_cart, &rank_cart);
    MPI_Cart_coords(comm_cart, rank_cart, 2, coords);

    MPI_Barrier(comm_cart);
    std::cout << "Processo " << rank_cart
              << " → posizione nella griglia: riga=" << coords[0]
              << ", colonna=" << coords[1] << std::endl;

    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}
