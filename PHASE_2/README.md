# 📡 MPI in C++ — Tutorial Progressivo

Un percorso didattico completo sulla programmazione parallela con **MPI (Message Passing Interface)** in C++.  
Il tutorial è strutturato in moduli progressivi: si parte dalla comunicazione base tra due processi e si arriva alle topologie virtuali e al solutore iterativo di Jacobi.

---

## 🗂️ Struttura del Repository

```
PHASE_2/
│
├── 01_Point_to_Point/          ← Comunicazione punto a punto
│   ├── Blocking/               ← Send/Recv bloccanti
│   └── NonBlocking/            ← Isend/Irecv non bloccanti
│
├── 03_Collective/              ← Comunicazione collettiva
│
└── 04_Topologies/              ← Topologie virtuali e Jacobi
    ├── Virtual_Topologies/     ← MPI_Cart_create e derivati
    └── Jacobi/                 ← Solver iterativo 2D
```

---

## 📚 Moduli

| # | Argomento | Contenuto |
|---|-----------|-----------|
| [01a](./01_point_to_point/blocking/) | **Blocking P2P** | `MPI_Send`, `MPI_Recv`, deadlock, ping-pong |
| [01b](./01_point_to_point/non_blocking/) | **Non-Blocking P2P** | `MPI_Isend`, `MPI_Irecv`, `MPI_Wait`, `MPI_Waitall` |
| [03](./03_collective/) | **Collective** | Bcast, Scatter/Gather, Reduce, Allreduce, Alltoall |
| [04a](./04_topologies/virtual_topologies/) | **Virtual Topologies** | `MPI_Cart_create`, `MPI_Cart_shift`, `MPI_Sendrecv` |
| [04b](./04_topologies/jacobi_2d/) | **Jacobi 2D** | Halo exchange, convergenza, decomposizione di dominio |

---

## ⚙️ Prerequisiti

### Compilatore e librerie
```bash
# Ubuntu/Debian
sudo apt install mpich build-essential

# oppure con OpenMPI
sudo apt install libopenmpi-dev openmpi-bin
```

### Compilazione (template generale)
```bash
mpicxx -O2 -Wall -o programma programma.cpp
mpirun -np 4 ./programma
```

> **Nota:** `mpicxx` è il wrapper C++ per MPI. Internamente chiama `g++` aggiungendo automaticamente i flag di include e linking per la libreria MPI.

---

## 🧠 Concetti Fondamentali MPI

### Inizializzazione e terminazione
```cpp
#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);          // Inizializza MPI

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Rank del processo corrente
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // Numero totale di processi

    std::cout << "Sono il processo " << rank
              << " di " << size << std::endl;

    MPI_Finalize();  // Chiude MPI
    return 0;
}
```

### Il modello SPMD
MPI segue il paradigma **SPMD (Single Program, Multiple Data)**:
- **Un solo sorgente**, eseguito da tutti i processi contemporaneamente.
- Ogni processo ha un proprio **rank** (identificatore intero, da 0 a size-1).
- La logica si differenzia con `if (rank == 0) { ... } else { ... }`.

### Tipi di dato MPI più comuni
| Tipo C++     | Tipo MPI          |
|--------------|-------------------|
| `int`        | `MPI_INT`         |
| `double`     | `MPI_DOUBLE`      |
| `float`      | `MPI_FLOAT`       |
| `char`       | `MPI_CHAR`        |
| `long`       | `MPI_LONG`        |

---

## 🗺️ Percorso Consigliato

```
01a Blocking  →  01b Non-Blocking  →  02 Communicators
                                            ↓
                               03 Collective Communication
                                            ↓
                          04a Virtual Topologies  →  04b Jacobi 2D
```

Ogni modulo contiene:
- 📖 **README** con spiegazione teorica
- 💻 **Esercizi commentati** in C++
- ✅ **Output atteso** per verificare la correttezza

---

## 📎 Riferimenti

- [MPI Standard 4.1](https://www.mpi-forum.org/docs/)
- [MPICH Documentation](https://www.mpich.org/documentation/guides/)
- [Open MPI FAQ](https://www.open-mpi.org/faq/)
- Pacheco, *An Introduction to Parallel Programming*, Morgan Kaufmann
