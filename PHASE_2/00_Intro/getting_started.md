# Getting Started — Struttura di un programma MPI in C++

Questo documento è il punto di partenza pratico prima di affrontare qualsiasi esercizio
del tutorial. Spiega come impostare l'ambiente, quali header includere, come compilare
ed eseguire, e qual è la struttura minima corretta di ogni programma MPI in C++.

---

## 1. Include necessari

### Per MPI

```cpp
#include <mpi.h>
```

È l'unico header necessario per accedere a tutte le funzioni MPI (`MPI_Init`,
`MPI_Send`, `MPI_Bcast`, ecc.). Va incluso **prima** di qualsiasi chiamata MPI
e tipicamente è il primo include del file.

### Header C++ standard più comuni in HPC

```cpp
#include <iostream>   // std::cout, std::cerr
#include <vector>     // std::vector<T> — array dinamici contigui
#include <cmath>      // std::sqrt, std::abs, M_PI
#include <iomanip>    // std::setprecision, std::setw — formattazione output
#include <algorithm>  // std::max, std::min, std::swap
#include <numeric>    // std::iota, std::accumulate
#include <cstring>    // std::memcpy, strlen — operazioni su buffer raw
```

### Struttura degli include consigliata

Segui sempre questo ordine per chiarezza e portabilità:

```cpp
// 1. Header MPI (sempre per primo se usi MPI)
#include <mpi.h>

// 2. Header C++ standard library
#include <iostream>
#include <vector>
#include <cmath>

// 3. Eventuali librerie di terze parti (Eigen, ecc.)
// #include <Eigen/Dense>

// 4. Eventuali header locali del tuo progetto
// #include "utils.h"
```

---

## 2. Compilatori

### Il compilatore diretto: `g++`

`g++` è il compilatore C++ di GCC. Non conosce MPI da solo — dovresti specificare
manualmente tutti i percorsi delle librerie MPI.

```bash
# NON fare questo manualmente:
g++ -I/usr/include/mpi -L/usr/lib/x86_64-linux-gnu/openmpi/lib \
    -lmpi -o programma programma.cpp
```

### Il wrapper consigliato: `mpicxx`

`mpicxx` (o `mpic++`) è un **wrapper** attorno a `g++` che aggiunge automaticamente
tutti i flag di include e linking per MPI. È il modo corretto per compilare.

```bash
mpicxx -O2 -Wall -std=c++14 -o programma programma.cpp
```

| Flag | Significato |
|------|-------------|
| `-O2` | Ottimizzazione di livello 2 (buon compromesso velocità/debug) |
| `-O3 -march=native` | Ottimizzazione massima + istruzioni specifiche per la CPU |
| `-Wall` | Attiva tutti i warning — usalo sempre |
| `-std=c++14` | Standard C++14 (minimo consigliato; usa `c++17` se disponibile) |
| `-g` | Simboli di debug (per usare `gdb`) |

### Quale MPI è installato?

```bash
# Verifica quale implementazione MPI hai
mpicxx --version          # mostra il compilatore sottostante
mpirun --version          # mostra l'implementazione MPI (OpenMPI, MPICH, ecc.)

# Su Ubuntu/Debian: installa OpenMPI
sudo apt install libopenmpi-dev openmpi-bin

# Su Ubuntu/Debian: installa MPICH (alternativa)
sudo apt install mpich
```

> Le due implementazioni principali sono **OpenMPI** e **MPICH**. Per gli esercizi
> di questo tutorial sono intercambiabili. Su cluster HPC usa quella disponibile
> (di solito Intel MPI o OpenMPI).

---

## 3. Esecuzione con `mpirun`

```bash
# Lancia il programma con N processi paralleli
mpirun -np 4 ./programma

# Equivalente (alcuni sistemi usano mpiexec)
mpiexec -n 4 ./programma

# Su un cluster con SLURM (no mpirun diretto)
srun --ntasks=4 ./programma
```

> `-np` e `-n` sono equivalenti. Su cluster il launcher è scelto dal sistema
> di job scheduling (SLURM, PBS, ecc.).

---

## 4. I concetti base di MPI

### `MPI_Init` — inizializzazione

```cpp
MPI_Init(&argc, &argv);
```

**Deve essere la prima chiamata MPI** nel programma. Inizializza l'ambiente MPI
e rende disponibili tutti i meccanismi di comunicazione. Prima di questa chiamata
nessuna funzione MPI è utilizzabile (eccetto `MPI_Initialized`).

Passa `argc` e `argv` dal `main` perché alcune implementazioni MPI usano gli
argomenti da riga di comando per configurarsi.

### `MPI_Finalize` — chiusura

```cpp
MPI_Finalize();
```

**Deve essere l'ultima chiamata MPI** nel programma. Libera tutte le risorse MPI
e sincronizza la chiusura dei processi. Dopo questa chiamata nessuna funzione MPI
è più utilizzabile.

> ⚠️ Se esci dal programma senza chiamare `MPI_Finalize` (es. con `exit()` o un
> crash), MPI potrebbe lasciare processi zombie o risorse bloccate sul cluster.

### `MPI_COMM_WORLD` — il communicator globale

```cpp
MPI_COMM_WORLD
```

È il **communicator predefinito** che include tutti i processi lanciati da `mpirun`.
Un communicator è un gruppo di processi che possono comunicare tra loro. 
`MPI_COMM_WORLD` è sempre disponibile dopo `MPI_Init` e contiene tutti i processi.

### `MPI_Comm_rank` — chi sono io?

```cpp
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

Assegna a `rank` l'**identificatore univoco** del processo corrente all'interno del
communicator. I rank vanno da `0` a `size-1`. È tramite il rank che ogni processo
sa "chi è" e cosa deve fare (es. `if (rank == 0)` per il processo master).

### `MPI_Comm_size` — quanti siamo?

```cpp
int size;
MPI_Comm_size(MPI_COMM_WORLD, &size);
```

Assegna a `size` il **numero totale di processi** nel communicator. Corrisponde al
valore passato a `mpirun -np`.

---

## 5. La struttura minima di ogni programma MPI

Questo è il template di partenza da cui derivano tutti gli esercizi del tutorial:

```cpp
#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[]) {

    // ── 1. Inizializzazione ──────────────────────────────────────────
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ── 2. Logica del programma ──────────────────────────────────────
    // Il codice qui è eseguito da TUTTI i processi simultaneamente.
    // Si usa 'rank' per differenziare il comportamento.

    if (rank == 0) {
        // Solo il processo master (rank 0) esegue questo blocco
        std::cout << "Master: siamo in " << size << " processi." << std::endl;
    } else {
        // Tutti gli altri processi eseguono questo
        std::cout << "Worker " << rank << ": pronto." << std::endl;
    }

    // ── 3. Chiusura ──────────────────────────────────────────────────
    MPI_Finalize();
    return 0;
}
```

### Compilazione ed esecuzione

```bash
mpicxx -O2 -Wall -o hello hello.cpp
mpirun -np 4 ./hello
```

### Output atteso (l'ordine può variare)

```
Master: siamo in 4 processi.
Worker 1: pronto.
Worker 3: pronto.
Worker 2: pronto.
```

> L'ordine dell'output **non è deterministico**: ogni processo scrive quando arriva
> al `cout`, indipendentemente dagli altri. Questo è normale in MPI.

---

## 6. Errori comuni da evitare subito

### Dimenticare `MPI_Init` o `MPI_Finalize`

```cpp
// ✗ SBAGLIATO: chiamata MPI prima di Init
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // undefined behavior
MPI_Init(&argc, &argv);
```

```cpp
// ✗ SBAGLIATO: uscita senza Finalize
MPI_Init(&argc, &argv);
// ...
return 0;  // MPI non è stato chiuso correttamente
```

### Stampare senza considerare il rank

```cpp
// ✗ SPESSO INDESIDERATO: tutti i processi stampano la stessa riga
// → output duplicato per ogni processo
std::cout << "Risultato finale: " << risultato << std::endl;

// ✓ CORRETTO: solo il processo 0 stampa il risultato globale
if (rank == 0)
    std::cout << "Risultato finale: " << risultato << std::endl;
```

### Usare variabili non inizializzate nei buffer MPI

```cpp
// ✗ SBAGLIATO: buf non inizializzato, MPI_Recv scrive qui
double buf;  // valore indefinito
MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

// ✓ CORRETTO: inizializza sempre i buffer di ricezione
double buf = 0.0;
MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

---

## 7. Collegamento con il resto del tutorial

Una volta chiara questa struttura base, tutti gli esercizi del tutorial seguono
sempre lo stesso schema: `MPI_Init` → logica differenziata per rank → `MPI_Finalize`.
Ciò che cambia è la **logica di comunicazione** nel mezzo:

```
getting_started.md   ←  sei qui
        ↓
01_point_to_point/   ←  MPI_Send / MPI_Recv tra due processi
        ↓
02_communicators/    ←  sottogruppi e communicator personalizzati
        ↓
03_collective/       ←  Bcast, Scatter, Gather, Reduce su tutti i processi
        ↓
04_topologies/       ←  griglie virtuali e solver Jacobi 2D
```