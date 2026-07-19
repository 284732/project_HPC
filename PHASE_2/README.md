# 📡 MPI in C++ per il Calcolo ad Alte Prestazioni

## Panoramica

Questa repository raccoglie una sequenza di esempi e casi di studio a difficoltà crescente sulla programmazione parallela con **MPI (Message Passing Interface)** in C++.

Il materiale copre i paradigmi di comunicazione fondamentali dei sistemi a memoria distribuita, inclusa la comunicazione point-to-point (bloccante e non bloccante), la comunicazione collettiva, le topologie virtuali, le tecniche di decomposizione del dominio e l'implementazione di un solutore iterativo di Jacobi. Particolare attenzione è dedicata a concetti ricorrenti nelle applicazioni di High Performance Computing (HPC), quali halo exchange, sincronizzazione tra processi e scalabilità degli algoritmi distribuiti.

La repository è organizzata come una sequenza di **capitoli** tematici, ciascuno dedicato a uno specifico aspetto della programmazione MPI, corredato di note teoriche, codice sorgente commentato e output di esecuzione rappresentativi.

---

## 🗂️ Struttura della repository

```text
PHASE_2/
│
├── 00_Intro/                    ← Scientific Computing, concetti di HPC e
│                                  considerazioni Fortran vs C++
│
├── 01_Point_to_Point/           ← Comunicazione point-to-point
│   ├── Blocking/                ← MPI_Send / MPI_Recv
│   └── NonBlocking/             ← MPI_Isend / MPI_Irecv
│
├── 02_Collective/               ← Primitive di comunicazione collettiva
│
└── 03_Topologies/
    ├── Jacobi/                  ← Solutore di Jacobi distribuito
    └── Virtual_Topologies/      ← Comunicatori cartesiani e individuazione dei vicini
```

## 📚 Indice dei capitoli

| Capitolo | Argomento | Concetti principali |
| --- | --- | --- |
| 00 | Introduzione | Fondamenti di HPC, Scientific Computing, confronto Fortran vs C++ |
| 01a | Comunicazione Point-to-Point Bloccante | MPI_Send, MPI_Recv, prevenzione del deadlock, benchmark ping-pong |
| 01b | Comunicazione Point-to-Point Non Bloccante | MPI_Isend, MPI_Irecv, MPI_Wait, MPI_Waitall, sovrapposizione calcolo-comunicazione |
| 02 | Comunicazione Collettiva | Bcast, Scatter, Gather, Reduce, Allreduce, Alltoall |
| 03a | Solutore di Jacobi Distribuito | Decomposizione del dominio, halo exchange, criterio di convergenza |
| 03b | Topologie Virtuali | MPI_Cart_create, MPI_Cart_shift, MPI_Sendrecv, griglie cartesiane |

Non è presente alcun capitolo 4: la sequenza didattica della repository si conclude con il capitolo 3b.

---

## ⚙️ Requisiti software

### Implementazione MPI

Gli esempi possono essere compilati indifferentemente con MPICH oppure OpenMPI.

### Ubuntu / Debian

```bash
sudo apt install mpich build-essential
```

oppure

```bash
sudo apt install libopenmpi-dev openmpi-bin
```

Le due implementazioni sono entrambe conformi allo standard MPI e intercambiabili ai fini di questa repository; non è necessario installarle entrambe contemporaneamente, ed è sconsigliato mescolarle nello stesso ambiente di compilazione per evitare conflitti tra header e librerie di runtime.

---

## Compilazione

Template generico di compilazione:

```bash
mpicxx -O2 -Wall -o program program.cpp
```

Esempio di esecuzione:

```bash
mpirun -np 4 ./program
```

Il wrapper del compilatore `mpicxx` fornisce automaticamente, in fase di compilazione e di linking, i percorsi di include e le librerie MPI necessarie, evitando la necessità di specificarli manualmente. Il flag `-O2` abilita un livello di ottimizzazione moderato del compilatore (bilanciato tra tempo di compilazione e prestazioni del codice generato), mentre `-Wall` attiva l'insieme standard di warning del compilatore, utile in fase di sviluppo per individuare errori comuni (variabili non inizializzate, confronti sospetti, ecc.) prima ancora dell'esecuzione.

---

## Il modello di programmazione MPI

MPI segue il paradigma **SPMD (Single Program, Multiple Data)**:

* Un singolo eseguibile viene lanciato su più processi.
* Ogni processo possiede un identificatore univoco, il **rank**.
* I processi cooperano scambiandosi messaggi tramite le routine di comunicazione MPI.
* Ogni processo esegue lo stesso codice sorgente, ma segue percorsi di esecuzione diversi in base al proprio rank.

Esempio minimale:

```cpp
#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "Process " << rank
              << " of " << size << std::endl;

    MPI_Finalize();

    return 0;
}
```

Ogni programma MPI deve invocare `MPI_Init` prima di qualunque altra chiamata MPI (tipicamente come prima istruzione operativa di `main`, a valle della sola gestione di `argc`/`argv`) e `MPI_Finalize` come ultima chiamata MPI prima della terminazione del processo: nessuna funzione MPI, ad eccezione di un numero molto ristretto di query che lo standard esplicita separatamente, è definita al di fuori di questa finestra di inizializzazione/finalizzazione. Omettere `MPI_Finalize`, o invocare funzioni MPI dopo di essa, produce comportamento indefinito.

---

## Datatype MPI comuni

| Tipo C++ | Datatype MPI |
| --- | --- |
| int | MPI_INT |
| double | MPI_DOUBLE |
| float | MPI_FLOAT |
| char | MPI_CHAR |
| long | MPI_LONG |
| long long | MPI_LONG_LONG |

Questa tabella copre i tipi scalari più ricorrenti nei capitoli della repository; MPI definisce un insieme più ampio di datatype predefiniti (inclusi tipi senza segno, tipi composti come `MPI_DOUBLE_INT` per le operazioni `MPI_MAXLOC`/`MPI_MINLOC` discusse nel capitolo 02, e la possibilità di costruire datatype derivati custom per strutture dati non contigue), non trattati in questa tabella riassuntiva perché non necessari alla comprensione degli esempi base.

---

## Percorso didattico

La repository segue una progressione dai meccanismi di comunicazione elementari agli algoritmi distribuiti più avanzati:

```text
Introduzione (00)
      ↓
Point-to-Point Bloccante (01a)
      ↓
Point-to-Point Non Bloccante (01b)
      ↓
Comunicazione Collettiva (02)
      ↓
Solutore di Jacobi Distribuito (03a)
      ↓
Topologie Virtuali (03b)
```

Da notare che, in questa sequenza, il solutore di Jacobi (03a) precede didatticamente le topologie virtuali (03b): il capitolo 03a introduce la decomposizione del dominio e l'halo exchange usando calcoli manuali dei rank dei vicini (senza topologia cartesiana), mentre il capitolo 03b mostra successivamente come le stesse operazioni di individuazione dei vicini possano essere delegate a `MPI_Cart_shift`, generalizzando il pattern di decomposizione a griglie multidimensionali. Chi affronta i due capitoli in quest'ordine osserva quindi prima il problema (gestione manuale dei vicini in una decomposizione a strisce) e poi lo strumento che ne semplifica la soluzione in casi più generali (topologia cartesiana), anziché il percorso inverso.

Ogni capitolo contiene:

* Inquadramento teorico dell'argomento trattato
* Implementazioni C++ commentate
* Esempi di esecuzione rappresentativi, con output atteso commentato
* Riferimenti ai concetti MPI rilevanti trattati nei capitoli precedenti, dove applicabile
