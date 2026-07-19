# Per Iniziare — Struttura di un Programma MPI in C++

> Questo documento è il punto di partenza pratico prima di affrontare qualunque capitolo di questo tutorial. Precede concettualmente il capitolo 00 (introduzione teorica al C++ per l'HPC): mentre quel capitolo motiva le scelte di linguaggio e le caratteristiche prestazionali rilevanti, questo documento copre gli aspetti puramente operativi — ambiente, header, compilazione, esecuzione — necessari a mandare in esecuzione anche il più semplice dei programmi MPI presentati nei capitoli successivi.

---

## Indice

1. [Header necessari](#1-header-necessari)
2. [Compilatori](#2-compilatori)
3. [Esecuzione con mpirun](#3-esecuzione-con-mpirun)
4. [Concetti fondamentali di MPI](#4-concetti-fondamentali-di-mpi)
5. [Differenze sintattiche: Fortran `call` vs chiamate dirette in C++](#5-differenze-sintattiche-fortran-call-vs-chiamate-dirette-in-c)
6. [Struttura minima di ogni programma MPI](#6-struttura-minima-di-ogni-programma-mpi)
7. [Errori comuni da evitare](#7-errori-comuni-da-evitare)
8. [Collegamento con il resto del tutorial](#8-collegamento-con-il-resto-del-tutorial)

---

## 1. Header necessari

### Per MPI

```cpp
#include <mpi.h>
```

Questo è l'unico header necessario per accedere a tutte le funzioni MPI (`MPI_Init`, `MPI_Send`, `MPI_Bcast`, ecc.). Va incluso **prima** di qualunque chiamata MPI, e tipicamente è il primo include del file. Da un punto di vista tecnico, `mpi.h` è un header C (le implementazioni MPI espongono un'interfaccia C standard, non C++ nativa).

### Header standard C++ più comuni in ambito HPC

```cpp
#include <iostream>   // std::cout, std::cerr
#include <vector>     // std::vector<T> — array dinamici contigui (capitolo 00, sezione 3.2)
#include <cmath>      // std::sqrt, std::abs, M_PI
#include <iomanip>    // std::setprecision, std::setw — formattazione dell'output
#include <algorithm>  // std::max, std::min, std::swap
#include <numeric>    // std::iota, std::accumulate
#include <cstring>    // std::memcpy, strlen — operazioni su buffer grezzi
```

Alcune note su header ricorrenti in questo tutorial: `<cmath>` è necessario ogniqualvolta si utilizzano funzioni matematiche standard (ad esempio `std::fabs` per il calcolo delle variazioni puntuali nel solutore di Jacobi, capitolo 03a); `<algorithm>` fornisce `std::swap`, usato sistematicamente nel solutore di Jacobi per scambiare i puntatori dei buffer `u`/`u_new` senza copia (capitolo 03a, sezione 2.7); `<cstring>` è utile quando si opera su buffer di memoria grezzi in scenari di ottimizzazione avanzata, meno frequente negli esercizi introduttivi ma presente in codice HPC di produzione.

### Ordine di include consigliato

Per chiarezza e portabilità, è buona norma seguire sempre questo ordine:

```cpp
// 1. Header MPI (sempre per primo, se si usa MPI)
#include <mpi.h>

// 2. Header della libreria standard C++
#include <iostream>
#include <vector>
#include <cmath>

// 3. Librerie di terze parti, se necessarie (Eigen, ecc. — vedi capitolo 00, sezione 4)
// #include <Eigen/Dense>

// 4. Header locali del progetto
// #include "utils.h"
```

Questo ordinamento non è puramente stilistico: includere `mpi.h` per primo riduce il rischio di conflitti di macro o di ridefinizione di simboli con altre librerie che potrebbero, a loro volta, includere internamente header di sistema in un ordine diverso da quello atteso dall'implementazione MPI in uso.

## 2. Compilatori

### Il compilatore diretto: `g++`

`g++` è il compilatore C++ del progetto GCC. Non ha alcuna conoscenza nativa di MPI: per compilare un programma che usa MPI con `g++` direttamente, sarebbe necessario specificare manualmente tutti i percorsi di include e di linking della libreria MPI installata sul sistema:

```bash
# NON fare così manualmente:
g++ -I/usr/include/mpi -L/usr/lib/x86_64-linux-gnu/openmpi/lib \
    -lmpi -o program program.cpp
```

Questo approccio è fragile: i percorsi esatti degli header e delle librerie MPI variano a seconda della distribuzione Linux, dell'implementazione MPI installata (OpenMPI, MPICH, Intel MPI, ecc.) e della versione specifica, rendendo il comando sopra non portabile tra sistemi diversi.

### Il wrapper consigliato: `mpicxx`

`mpicxx` (talvolta disponibile anche come `mpic++`, a seconda dell'implementazione) è un **wrapper** attorno a `g++` (o al compilatore C++ di sistema configurato dall'implementazione MPI, che può anche essere Clang o un compilatore proprietario) che aggiunge automaticamente tutti i flag di include e di linking necessari per MPI, interrogando la configurazione dell'implementazione MPI installata al momento della sua stessa compilazione/installazione. Questo è il modo corretto di compilare programmi MPI, indipendentemente dal sistema su cui si sta lavorando:

```bash
mpicxx -O2 -Wall -std=c++14 -o program program.cpp
```

| Flag | Significato |
| --- | --- |
| `-O2` | Livello di ottimizzazione 2 (buon compromesso tra velocità di esecuzione e facilità di debug; è il livello usato come riferimento nei template di compilazione di questo tutorial) |
| `-O3 -march=native` | Ottimizzazione massima, incluse istruzioni specifiche della CPU corrente (si veda capitolo 00, sezione 6, per la spiegazione dettagliata del significato e dei rischi di portabilità di `-march=native`) |
| `-Wall` | Abilita l'insieme standard di warning del compilatore — da usare sempre, poiché individua a tempo di compilazione una classe ampia di errori comuni (variabili non inizializzate, confronti sospetti tra tipi diversi, funzioni con valore di ritorno non gestito) prima ancora dell'esecuzione |
| `-std=c++14` | Seleziona lo standard C++14 (minimo raccomandato per questo tutorial; usare `-std=c++17` se disponibile sul sistema, per accedere a funzionalità del linguaggio più recenti come i structured bindings o `if constexpr`) |
| `-g` | Include i simboli di debug nel binario compilato, necessari per poter ispezionare il programma con `gdb` (o con `mpirun` in combinazione con `gdb`, tipicamente lanciando ciascun processo MPI sotto un'istanza separata del debugger, o usando strumenti di debug paralleli dedicati come TotalView o DDT su cluster di produzione) |

Da notare che `-Wall`, pur abilitando un ampio insieme di warning utili, non è esaustivo: per un controllo ancora più stringente in fase di sviluppo si aggiunge spesso anche `-Wextra`, non incluso di default in questo tutorial per mantenere l'output di compilazione più leggibile durante l'apprendimento iniziale.

### Verificare la propria installazione MPI

```bash
# Verifica quale implementazione MPI è installata
mpicxx --version          # mostra il compilatore sottostante utilizzato dal wrapper
mpirun --version          # mostra l'implementazione MPI (OpenMPI, MPICH, ecc.)

# Ubuntu/Debian: installazione di OpenMPI
sudo apt install libopenmpi-dev openmpi-bin

# Ubuntu/Debian: installazione di MPICH (alternativa)
sudo apt install mpich
```

> Le due implementazioni principali sono **OpenMPI** e **MPICH**. Per gli esercizi di questo tutorial sono intercambiabili, poiché entrambe implementano fedelmente lo standard MPI e la sintassi del codice C++ presentato non dipende da alcuna estensione specifica dell'una o dell'altra. Su cluster HPC, si utilizza tipicamente l'implementazione già disponibile e configurata dagli amministratori di sistema (spesso Intel MPI, o una build di OpenMPI/MPICH specificamente compilata per sfruttare l'interconnessione di rete del cluster, es. Infiniband).

## 3. Esecuzione con `mpirun`

```bash
# Lancia il programma con N processi paralleli
mpirun -np 4 ./program

# Equivalente (alcuni sistemi usano mpiexec)
mpiexec -n 4 ./program

# Su un cluster con SLURM (senza mpirun diretto)
srun --ntasks=4 ./program
```

> `-np` e `-n` sono equivalenti tra le implementazioni comuni. Su cluster gestiti da un sistema di scheduling dei job (SLURM, PBS/Torque, LSF), il launcher effettivo dei processi è determinato dal sistema di scheduling stesso, non invocato manualmente dall'utente come nei due comandi precedenti.

Vale la pena chiarire la distinzione tra `mpirun`/`mpiexec` e `srun`: nei primi due casi, è l'implementazione MPI stessa (tramite il proprio launcher) a occuparsi di avviare i processi sui nodi disponibili, tipicamente basandosi su un file di host o su variabili d'ambiente locali; con `srun` (SLURM), è invece il resource manager del cluster a farsi carico dell'allocazione delle risorse (nodi, core, memoria) e dell'avvio dei processi, delegando a MPI unicamente la fase di scoperta reciproca dei processi già lanciati e la costruzione dei canali di comunicazione tra loro (fase nota come *PMI bootstrap*, Process Management Interface). Su un cluster di produzione con SLURM, è quindi tipicamente `srun` (o l'invocazione di `mpirun` all'interno di uno script SLURM, a seconda della configurazione locale) il meccanismo effettivamente usato, non `mpirun` invocato in modo completamente autonomo come si farebbe su una singola workstation di sviluppo.

## 4. Concetti fondamentali di MPI

### `MPI_Init` — Inizializzazione

```cpp
MPI_Init(&argc, &argv);
```

**Deve essere la prima chiamata MPI** del programma. Inizializza l'ambiente di esecuzione MPI e rende disponibili tutti i meccanismi di comunicazione: prima di questa chiamata, nessuna funzione MPI è utilizzabile (con la sola eccezione di `MPI_Initialized`, una funzione di query pensata specificamente per verificare, in codice riutilizzabile o in librerie che potrebbero essere invocate sia in contesti MPI che non-MPI, se l'ambiente MPI è già stato inizializzato altrove).

`argc` e `argv` vengono passati da `main` per puntatore perché alcune implementazioni MPI utilizzano gli argomenti da riga di comando per configurazioni interne (ad esempio parametri specifici del launcher, filtrati e rimossi dagli argomenti prima che il programma applicativo li veda), e `MPI_Init` può quindi modificarne il contenuto. È buona pratica passare sempre `argc`/`argv` a `MPI_Init` anche quando il programma applicativo non necessita di argomenti da riga di comando propri, per garantire la piena compatibilità con questo comportamento specifico dell'implementazione. Esiste inoltre una variante `MPI_Init_thread`, non trattata in questo tutorial, che permette di richiedere esplicitamente un determinato livello di supporto al multithreading (rilevante in scenari ibridi MPI+thread, ad esempio MPI combinato con OpenMP), assente nella forma base di `MPI_Init` qui presentata.

### `MPI_Finalize` — Terminazione

```cpp
MPI_Finalize();
```

**Deve essere l'ultima chiamata MPI** del programma. Rilascia tutte le risorse allocate da MPI (buffer di comunicazione interni, canali di rete, strutture dati di gestione dei communicator) e sincronizza la chiusura ordinata di tutti i processi. Nessuna funzione MPI può essere invocata dopo questa chiamata.

> ⚠️ Se il programma termina senza invocare `MPI_Finalize` (ad esempio tramite una `exit()` diretta, oppure a seguito di un crash non gestito), MPI può lasciare processi "zombie" o risorse di comunicazione bloccate sul cluster, con effetti che vanno da un semplice spreco temporaneo di risorse fino, in casi più gravi su sistemi condivisi, alla necessità di intervento manuale dell'amministratore di sistema per liberare le risorse di rete/scheduling rimaste allocate.

### `MPI_COMM_WORLD` — Il communicator globale

```cpp
MPI_COMM_WORLD
```

Questo è il **communicator predefinito** che include tutti i processi lanciati da `mpirun`. Un communicator è un gruppo di processi che possono comunicare tra loro (concetto approfondito nel capitolo 01a, sezione 2.3). `MPI_COMM_WORLD` è sempre disponibile immediatamente dopo `MPI_Init` e contiene, per definizione, ogni processo del job MPI corrente: è la scelta di default per la quasi totalità delle comunicazioni presentate in questo tutorial, salvo nei casi in cui si costruiscono esplicitamente sotto-communicator (come nel caso di `MPI_Cart_sub`, capitolo 03b, sezione 6).

### `MPI_Comm_rank` — Chi sono io?

```cpp
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

Assegna a `rank` l'**identificatore univoco** del processo corrente all'interno del communicator specificato. I rank vanno da `0` a `size-1`. Il rank è il meccanismo con cui ogni processo determina la propria identità e decide il proprio comportamento (ad esempio `if (rank == 0)` per distinguere il processo master, un pattern ricorrente in ogni capitolo di questo tutorial).

### `MPI_Comm_size` — Quanti siamo?

```cpp
int size;
MPI_Comm_size(MPI_COMM_WORLD, &size);
```

Assegna a `size` il **numero totale di processi** presenti nel communicator specificato. Questo valore corrisponde esattamente a quello passato a `mpirun -np` (o a `-n`, `--ntasks`, a seconda del launcher usato, sezione 3), per il communicator `MPI_COMM_WORLD`; per un sotto-communicator ottenuto ad esempio tramite `MPI_Cart_sub`, `size` restituirebbe invece il numero di processi appartenenti a quello specifico sotto-gruppo, tipicamente inferiore al numero totale di processi del job.

## 5. Differenze sintattiche: Fortran `call` vs chiamate dirette in C++

Chi proviene da MPI in Fortran incontrerà immediatamente tre differenze sintattiche sistematiche in ogni programma MPI scritto in C++.

### Assenza della parola chiave `call`

In Fortran, le subroutine vengono invocate con la parola chiave `call`:

```fortran
call MPI_Init(ierr)
call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
```

In C++, le funzioni MPI sono funzioni C standard e vengono invocate direttamente per nome, senza alcuna parola chiave introduttiva:

```cpp
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

Non esiste alcun equivalente di `call`: ogni routine MPI è semplicemente un'espressione di chiamata a funzione, sintatticamente identica a qualunque altra chiamata di funzione C++.

### Codice di errore: ultimo argomento vs valore di ritorno

In Fortran, ogni routine MPI accetta `ierr` come **ultimo argomento**, un parametro di output tramite cui la routine comunica l'esito dell'operazione:

```fortran
call MPI_Send(buf, count, MPI_DOUBLE_PRECISION, dest, tag, MPI_COMM_WORLD, ierr)
```

In C++, il codice di errore è invece il **valore di ritorno** della funzione stessa:

```cpp
int err = MPI_Send(buf, count, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
```

In pratica, nel codice di questo tutorial il valore di ritorno viene spesso ignorato (nessuna delle due varianti, `int err = ...` o la chiamata "nuda" senza assegnazione, altera il comportamento della chiamata stessa), ma può e dovrebbe essere controllato esplicitamente in codice di produzione, tipicamente confrontandolo con la costante `MPI_SUCCESS`. Il comportamento di default di MPI, in assenza di un error handler personalizzato installato esplicitamente tramite `MPI_Comm_set_errhandler` (non trattato in questo tutorial), è comunque quello di terminare il programma con un messaggio diagnostico alla prima condizione di errore rilevata, rendendo il controllo esplicito del valore di ritorno una misura di robustezza aggiuntiva più che strettamente necessaria per la correttezza degli esercizi qui presentati.

### Puntatori per gli argomenti di output

Questa è la differenza più importante e la fonte più comune di errori di compilazione per chi proviene da Fortran. In Fortran, tutti gli argomenti sono passati **per riferimento di default** — è il compilatore a gestire automaticamente gli indirizzi di memoria, in modo trasparente al programmatore:

```fortran
integer :: rank
call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
! Fortran passa automaticamente l'indirizzo di 'rank' a MPI
```

In C++, quando MPI deve **scrivere un valore in una variabile** (un argomento di output), è necessario passarne esplicitamente l'indirizzo tramite l'operatore `&`:

```cpp
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//                             ^
//                             Obbligatorio: passa l'indirizzo di memoria di 'rank'
//                             Senza &, MPI non può modificare la variabile
//                             (verrebbe passato il VALORE corrente, non
//                             indefinito ma inutile ai fini della chiamata:
//                             il compilatore C++ segnalerebbe comunque un
//                             errore di tipo, dato che MPI_Comm_rank si
//                             aspetta un int*, non un int)
```

Per i **buffer di dati** (array), non è necessario l'operatore `&`, perché il nome di un array decade automaticamente a un puntatore al suo primo elemento, secondo la semantica standard del linguaggio C/C++:

```cpp
double buf[100];
MPI_Send(buf, 100, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
//        ^ nessun & necessario: 'buf' è già un puntatore

// Con std::vector, si usa .data() per ottenere il puntatore al buffer interno:
std::vector<double> v(100);
MPI_Send(v.data(), 100, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
```

**Riferimento rapido:**

| Tipo di argomento | Fortran | C++ |
| --- | --- | --- |
| Scalare di output (rank, size, ecc.) | `rank` | `&rank` |
| Array di dati | `buf` | `buf` oppure `v.data()` |
| Communicator (input) | `MPI_COMM_WORLD` | `MPI_COMM_WORLD` |
| Request (output) | `request` | `&request` |
| Status (output) | `status` | `&status` oppure `MPI_STATUS_IGNORE` |

## 6. Struttura minima di ogni programma MPI

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
    // Questo codice viene eseguito da TUTTI i processi simultaneamente.
    // La variabile 'rank' viene usata per differenziarne il comportamento.

    if (rank == 0) {
        // Solo il processo master (rank 0) esegue questo blocco
        std::cout << "Master: running with " << size << " processes." << std::endl;
    } else {
        // Tutti gli altri processi eseguono questo
        std::cout << "Worker " << rank << ": ready." << std::endl;
    }

    // ── 3. Terminazione ───────────────────────────────────────────────
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

```text
Master: running with 4 processes.
Worker 1: ready.
Worker 3: ready.
Worker 2: ready.
```

> L'ordine dell'output è **non deterministico**: ogni processo scrive su `cout` non appena raggiunge quella riga di codice, indipendentemente dagli altri processi, che procedono con la propria esecuzione a velocità non sincronizzata tra loro. Questo è normale e atteso in MPI. Va inoltre notato che, trattandosi di `size` processi indipendenti (non thread di uno stesso processo con uno stream `cout` fisicamente condiviso), le singole scritture su `cout` di ciascun processo non rischiano interleaving *a livello di singolo carattere* all'interno della stessa riga (ogni processo scrive sul proprio stream di standard output indipendente, tipicamente instradato dal launcher MPI verso lo stesso terminale), ma **l'ordine relativo tra righe emesse da processi diversi** non è in alcun modo garantito dallo standard MPI, e può variare da un'esecuzione all'altra dello stesso identico programma, anche a parità di hardware e di numero di processi.

## 7. Errori comuni da evitare

### Dimenticare `MPI_Init` o `MPI_Finalize`

```cpp
// SBAGLIATO: chiamata MPI prima di Init
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // comportamento indefinito
MPI_Init(&argc, &argv);
```

```cpp
// SBAGLIATO: uscita dal programma senza Finalize
MPI_Init(&argc, &argv);
// ...
return 0;  // MPI non è stato terminato correttamente
```

### Stampare senza controllare il rank

```cpp
// SPESSO NON INTENZIONALE: ogni processo stampa la stessa riga
// → output ripetuto una volta per ciascun processo
std::cout << "Final result: " << result << std::endl;

// CORRETTO: solo il processo 0 stampa il risultato globale
if (rank == 0)
    std::cout << "Final result: " << result << std::endl;
```

Questo errore è particolarmente insidioso quando `result` è, in realtà, un valore che **dovrebbe** essere identico su tutti i processi (ad esempio l'esito di una `MPI_Allreduce`, capitolo 02, sezione 6): il programma produce output apparentemente "corretto" nel valore numerico stampato, ma ripetuto `size` volte, un difetto facilmente trascurato durante lo sviluppo su un numero ridotto di processi e che diventa rapidamente fastidioso (o problematico, se l'output viene reindirizzato e successivamente processato da script automatici) al crescere del numero di processi impiegati.

### Usare buffer non inizializzati nelle chiamate MPI

```cpp
// SBAGLIATO: buf non è inizializzato; MPI_Recv scriverà in memoria indefinita
double buf;
MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

// CORRETTO: inizializzare sempre i buffer di ricezione
double buf = 0.0;
MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

Va chiarito il motivo tecnico di questa raccomandazione: `MPI_Recv` sovrascriverà comunque interamente `buf` con il contenuto del messaggio ricevuto (assumendo che la ricezione vada a buon fine), quindi il valore iniziale di `buf` non influenza mai il *risultato* dell'operazione di ricezione in sé. L'inizializzazione esplicita è però una misura di robustezza difensiva rilevante in scenari più complessi: in fase di debug (per distinguere a colpo d'occhio, in un dump di memoria o in un debugger, un buffer effettivamente popolato da MPI da uno mai toccato per un bug nel matching sender/receiver), e soprattutto in scenari in cui la ricezione potrebbe fallire silenziosamente o non completarsi per un errore logico altrove nel programma (ad esempio un mismatch di tag o rank, guida 01a, sezione 12): un buffer inizializzato a un valore noto e riconoscibile (es. `0.0`, o un valore sentinella chiaramente anomalo per il dominio applicativo) rende molto più agevole diagnosticare, a posteriori, se quella particolare ricezione sia effettivamente avvenuta come previsto.

## 8. Collegamento con il resto del tutorial

Una volta chiara questa struttura di base, ogni esercizio del tutorial segue lo stesso pattern generale: `MPI_Init` → logica differenziata per rank → `MPI_Finalize`. Ciò che cambia, capitolo dopo capitolo, è esclusivamente la **logica di comunicazione** inserita nel mezzo:

```text
getting_started.md        ←  sei qui
        ↓
00_Intro/                 ←  capitolo 00 — motivazioni, C++ per l'HPC, librerie numeriche
        ↓
01_Point_to_Point/         ←  capitolo 01a/01b — MPI_Send/MPI_Recv (bloccante e non bloccante) tra due processi
        ↓
02_Collective/              ←  capitolo 02 — Bcast, Scatter, Gather, Reduce tra tutti i processi
        ↓
03_Topologies/
   ├── Jacobi/                ←  capitolo 03a — solutore di Jacobi distribuito
   └── Virtual_Topologies/    ←  capitolo 03b — griglie cartesiane virtuali
```

Questo documento, insieme al capitolo 00, costituisce quindi il prerequisito comune a tutti i capitoli successivi: da qui in avanti, ogni nuovo capitolo introdurrà esclusivamente le funzioni MPI specifiche del pattern di comunicazione trattato, dando per acquisita la struttura generale (`Init`/`Finalize`, gestione del rank, compilazione con `mpicxx`, esecuzione con `mpirun`) presentata in questo documento.
