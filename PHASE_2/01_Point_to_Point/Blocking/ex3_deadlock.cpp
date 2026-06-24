// =============================================================
// ESERCIZIO 3 — Deadlock: causa e soluzioni
// =============================================================
// Dimostra il deadlock classico e tre modi per risolverlo.
// Decommenta la versione da testare (una per volta).
//
// Compilazione:  mpicxx -O2 -Wall -o ex3_deadlock ex3_deadlock.cpp
// Esecuzione:    mpirun -np 2 ./ex3_deadlock
// =============================================================

#include <mpi.h>
#include <iostream>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// VERSIONE 1 — DEADLOCK (NON decommettare in produzione!)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Entrambi i processi tentano di inviare prima di ricevere.
// MPI_Send può bloccarsi se il buffer interno è pieno.
// → Deadlock garantito per messaggi grandi (oltre il buffer eager).
void versione_deadlock(int rank) {
    double buf_send = rank;
    double buf_recv = -1.0;
    int altro = 1 - rank;  // 0→1, 1→0

    MPI_Send(&buf_send, 1, MPI_DOUBLE, altro, 0, MPI_COMM_WORLD);  // ← BLOCCA?
    MPI_Recv(&buf_recv, 1, MPI_DOUBLE, altro, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    std::cout << "Processo " << rank << " ha ricevuto: " << buf_recv << std::endl;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// VERSIONE 2 — SOLUZIONE: Ordinare Send/Recv
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// Un processo fa Send prima, l'altro fa Recv prima.
// Non c'è più circolarità → nessun deadlock.
void versione_ordinata(int rank) {
    double buf_send = static_cast<double>(rank) * 10.0;
    double buf_recv = -1.0;
    int altro = 1 - rank;

    if (rank == 0) {
        // Processo 0: prima invia, poi riceve
        MPI_Send(&buf_send, 1, MPI_DOUBLE, altro, 0, MPI_COMM_WORLD);
        MPI_Recv(&buf_recv, 1, MPI_DOUBLE, altro, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        // Processo 1: prima riceve, poi invia
        MPI_Recv(&buf_recv, 1, MPI_DOUBLE, altro, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&buf_send, 1, MPI_DOUBLE, altro, 0, MPI_COMM_WORLD);
    }

    std::cout << "[Ordinata] Processo " << rank
              << " ha ricevuto: " << buf_recv
              << " (atteso: " << static_cast<double>(altro) * 10.0 << ")"
              << std::endl;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// VERSIONE 3 — SOLUZIONE: MPI_Sendrecv
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// MPI_Sendrecv combina Send e Recv in un'unica chiamata atomica.
// L'implementazione MPI gestisce internamente l'ordine → no deadlock.
void versione_sendrecv(int rank) {
    double buf_send = static_cast<double>(rank) * 10.0;
    double buf_recv = -1.0;
    int altro = 1 - rank;

    MPI_Sendrecv(sendbuf, sendcount, sendtype, dest,   sendtag,
                 recvbuf, recvcount, recvtype, source, recvtag,
                 comm, status)
    MPI_Sendrecv(
        &buf_send, 1, MPI_DOUBLE, altro, 0,    // send
        &buf_recv, 1, MPI_DOUBLE, altro, 0,    // recv
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );

    std::cout << "[Sendrecv] Processo " << rank
              << " ha ricevuto: " << buf_recv
              << " (atteso: " << static_cast<double>(altro) * 10.0 << ")"
              << std::endl;
}


int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0)
            std::cerr << "ERRORE: Usa esattamente 2 processi!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) std::cout << "\n--- Versione con Send/Recv ordinati ---\n";
    MPI_Barrier(MPI_COMM_WORLD);
    versione_ordinata(rank);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) std::cout << "\n--- Versione con MPI_Sendrecv ---\n";
    MPI_Barrier(MPI_COMM_WORLD);
    versione_sendrecv(rank);

    MPI_Finalize();
    return 0;
}
