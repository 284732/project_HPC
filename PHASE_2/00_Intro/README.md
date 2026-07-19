# 00 — Introduzione: C++ per il Calcolo ad Alte Prestazioni

> Capitolo introduttivo della repository.

---

## Indice

1. [Motivazione e contesto](#1-motivazione-e-contesto)
2. [Perché C++ per il calcolo scientifico](#2-perché-c-per-il-calcolo-scientifico)
3. [Organizzazione della memoria e array numerici](#3-organizzazione-della-memoria-e-array-numerici)
4. [Librerie numeriche](#4-librerie-numeriche)
5. [Fortran e C++ nell'HPC](#5-fortran-e-c-nellhpc)
6. [Ottimizzazione a livello di compilatore](#6-ottimizzazione-a-livello-di-compilatore)
7. [MPI e parallelismo a memoria distribuita](#7-mpi-e-parallelismo-a-memoria-distribuita)
8. [Roadmap della repository](#8-roadmap-della-repository)

---

## 1. Motivazione e contesto

Le applicazioni di **High Performance Computing (HPC)** richiedono l'uso efficiente delle risorse hardware disponibili sui sistemi di calcolo moderni: CPU multicore, unità di calcolo vettoriale (SIMD), acceleratori (GPU, e più in generale dispositivi con architetture massicciamente parallele), e cluster a memoria distribuita composti da centinaia o migliaia di nodi di calcolo interconnessi da reti ad alta velocità e bassa latenza (Infiniband, reti proprietarie a toro o dragonfly, ecc.).

Il termine "alte prestazioni" non va inteso in senso vago: nel contesto HPC si riferisce tipicamente alla capacità di un'applicazione di **scalare**, ossia di ridurre il proprio tempo di esecuzione (o di aumentare la dimensione del problema trattabile a parità di tempo) in modo proporzionale, o il più possibile vicino a proporzionale, all'aumentare delle risorse di calcolo impiegate. Questa proprietà — la scalabilità — è l'obiettivo di fondo che guida gran parte delle scelte progettuali discusse in questa repository, dalla scelta del linguaggio di programmazione fino ai pattern di comunicazione MPI trattati nei capitoli successivi.

Raggiungere buone prestazioni e buona scalabilità richiede attenzione simultanea a più livelli, tra loro interdipendenti:

* **Livello algoritmico**: la complessità computazionale e di comunicazione dell'algoritmo scelto (ad esempio, come discusso nel capitolo 03a, la scelta tra un pattern di comunicazione P2P mirato e uno collettivo dipende dalla struttura delle dipendenze dati dell'algoritmo).
* **Livello di implementazione**: come il codice sorgente sfrutta (o non sfrutta) le caratteristiche del linguaggio e delle librerie disponibili per produrre codice macchina efficiente.
* **Livello hardware**: come i dati vengono disposti in memoria e acceduti, in modo da sfruttare la gerarchia di cache, l'ampiezza di banda della memoria e le unità di calcolo vettoriale del processore.
* **Livello di sistema distribuito**: come i processi cooperano e si scambiano dati attraverso la rete di interconnessione, argomento centrale di tutti i capitoli successivi di questa repository.

Questo capitolo introduttivo si concentra sui primi tre livelli, fornendo il contesto necessario a comprendere **perché** certe scelte implementative (contiguità della memoria, scelta del linguaggio, uso di librerie ottimizzate) sono rilevanti prima ancora di introdurre la comunicazione distribuita.

## 2. Perché C++ per il calcolo scientifico

Le applicazioni scientifiche sono tipicamente caratterizzate da:

* Grandi insiemi di dati numerici, spesso array multidimensionali di dimensione dell'ordine di milioni o miliardi di elementi.
* Calcoli intensivi in virgola mobile (floating-point), spesso ripetuti un numero elevatissimo di volte all'interno di cicli iterativi (come, ad esempio, le iterazioni del metodo di Jacobi discusso nel capitolo 03a).
* Vincoli di memoria stringenti, dato che la dimensione dei problemi trattabili è spesso limitata dalla memoria RAM disponibile per processo/nodo, non solo dal tempo di calcolo.
* Tempi di esecuzione lunghi (ore o giorni, anche su cluster di grandi dimensioni), che rendono ogni inefficienza percentuale nel codice moltiplicata per un costo assoluto significativo in termini di tempo macchina e consumo energetico.

Per affrontare questi vincoli, in HPC ci si affida a linguaggi capaci di produrre codice fortemente ottimizzato, con overhead di runtime minimo o nullo. Il C++ moderno (a partire dallo standard C++11 e successivi) offre, in questo senso, un insieme di caratteristiche particolarmente rilevanti:

* **Ottimizzazione a tempo di compilazione.** Il compilatore C++ dispone di informazioni complete sui tipi e sulla struttura del programma già in fase di compilazione, permettendo ottimizzazioni aggressive (inlining di funzioni, eliminazione di codice morto, propagazione di costanti, srotolamento di cicli) senza alcun overhead a runtime, a differenza di linguaggi con risoluzione dinamica dei tipi o compilazione just-in-time.
* **Vettorizzazione automatica.** Il compilatore può trasformare automaticamente cicli che operano su dati contigui in istruzioni SIMD (Single Instruction, Multiple Data — vedi sezione 3), che eseguono la stessa operazione aritmetica simultaneamente su più elementi di dato in un singolo ciclo di clock, a patto che il codice sorgente non introduca ostacoli alla vettorizzazione (dipendenze tra iterazioni, accessi a memoria non contigui, aliasing di puntatori non dichiarato).
* **Controllo granulare della memoria.** A differenza di linguaggi con garbage collector, C++ consente al programmatore di controllare esplicitamente quando e come la memoria viene allocata, disposta e rilasciata (tramite RAII — Resource Acquisition Is Initialization — e contenitori come `std::vector`), evitando pause di garbage collection non deterministiche che sarebbero inaccettabili in codice numerico ad alte prestazioni con vincoli di tempo predicibili.
* **Accesso a librerie numeriche ottimizzate.** Come discusso in dettaglio nella sezione 4, l'ecosistema C++ include binding diretti o wrapper header-only per le principali librerie di algebra lineare e calcolo numerico usate in ambito HPC.

Grazie a questa combinazione di caratteristiche, il C++ è ampiamente utilizzato in fisica computazionale, fluidodinamica computazionale (CFD), analisi agli elementi finiti (FEM), infrastrutture di machine learning ad alte prestazioni, e framework di simulazione su larga scala.

## 3. Organizzazione della memoria e array numerici

L'efficienza di accesso alla memoria è uno dei fattori più determinanti per le prestazioni di un programma numerico, spesso più rilevante della sola complessità algoritmica asintotica quando si opera su hardware reale.

### 3.1 Gerarchia di memoria e cache

I processori moderni non accedono alla RAM principale direttamente per ogni operazione: interpongono una gerarchia di memorie cache (tipicamente L1, L2, L3), progressivamente più capienti ma più lente man mano che ci si allontana dal core di calcolo. Quando un dato viene acceduto, l'intera **cache line** che lo contiene (tipicamente 64 byte sui processori x86_64 attuali, corrispondenti a 8 valori `double` consecutivi) viene caricata in cache, non il singolo valore richiesto. Questo comportamento ha una conseguenza diretta e fondamentale per il codice numerico: **accedere a dati disposti in modo contiguo in memoria, nello stesso ordine in cui sono fisicamente disposti, massimizza il riutilizzo di ciò che è già stato caricato in cache**, riducendo drasticamente il numero di accessi alla RAM principale (che, in termini di cicli di clock, è uno o due ordini di grandezza più lenta della cache L1).

Un layout di memoria **contiguo** migliora quindi, simultaneamente:

* **L'utilizzo della cache**: più accessi successivi trovano il dato già disponibile in cache (cache hit) invece di dover attendere il caricamento da RAM (cache miss).
* **L'efficienza della banda di memoria**: il trasferimento di blocchi contigui satura più efficacemente la banda disponibile tra RAM e processore rispetto a trasferimenti sparsi (gather/scatter), che richiedono transazioni di memoria separate e non pienamente utilizzate.

### 3.2 Contenitori contigui in C++: `std::vector` e array grezzi

```cpp
#include <vector>

std::vector<double> x = {1.0, 2.0, 3.0};
```

`std::vector<T>` garantisce, per specifica dello standard C++, che i propri elementi siano memorizzati in un **blocco di memoria contiguo**, esattamente come un array C tradizionale: `&x[0]`, `&x[1]`, `&x[2]` sono indirizzi consecutivi (a meno del padding introdotto dall'allineamento del tipo `T`, non rilevante per `double`). Questo è il motivo per cui `std::vector` è la struttura dati di riferimento per kernel numerici e comunicazioni MPI: un puntatore al primo elemento (`x.data()`, oppure `&x[0]`) e la dimensione (`x.size()`) sono sufficienti per interfacciarsi sia con librerie numeriche C-style sia con le routine di comunicazione MPI, che operano su buffer contigui identificati da un puntatore di base e un conteggio di elementi (esattamente i parametri `buf`/`count` visti nei prototipi di `MPI_Send`, `MPI_Bcast`, ecc. nei capitoli successivi).

Gli array grezzi sono altrettanto frequentemente usati, con proprietà di contiguità identiche:

```cpp
double x[3] = {1.0, 2.0, 3.0};
```

Entrambe le rappresentazioni possono essere utilizzate direttamente nelle chiamate MPI perché MPI non conosce il tipo C++ del contenitore (ad esempio std::vector, array statici o memoria allocata dinamicamente). Le routine MPI ricevono semplicemente un indirizzo di memoria (void*) e interpretano i dati in base alle informazioni fornite dall'utente tramite i parametri datatype e count.
Per questo motivo, ciò che conta non è il contenitore utilizzato, ma il fatto che i dati siano memorizzati in modo contiguo in memoria. Inoltre, il tipo MPI specificato (ad esempio MPI_INT, MPI_DOUBLE, ecc.) e il numero di elementi dichiarato devono corrispondere esattamente alla reale disposizione dei dati nel buffer, affinché MPI possa leggere o scrivere correttamente la memoria.

## 4. Librerie numeriche

Le applicazioni scientifiche moderne raramente reimplementano da zero algoritmi numerici fondamentali: si affidano invece a librerie estremamente ottimizzate, spesso sviluppate e affinate nel corso di decenni, con implementazioni specifiche per l'architettura hardware target.

| Libreria | Scopo | Note tecniche |
| --- | --- | --- |
| **BLAS** (Basic Linear Algebra Subprograms) | Operazioni di algebra lineare di base | Organizzata in tre "livelli": Livello 1 (operazioni vettore-vettore, es. prodotto scalare), Livello 2 (operazioni matrice-vettore), Livello 3 (operazioni matrice-matrice, con la maggiore intensità aritmetica e quindi il maggior potenziale di ottimizzazione). Esistono implementazioni altamente ottimizzate per hardware specifico (OpenBLAS, Intel MKL, ATLAS) |
| **LAPACK** (Linear Algebra PACKage) | Algebra lineare densa (fattorizzazioni, autovalori, sistemi lineari) | Costruita sopra le primitive BLAS; fornisce routine per fattorizzazione LU, Cholesky, QR, decomposizione a autovalori/autovettori, SVD |
| **ScaLAPACK** (Scalable LAPACK) | Algebra lineare densa distribuita | Estensione di LAPACK a memoria distribuita tramite MPI; le matrici sono partizionate tra i processi secondo uno schema di distribuzione a blocchi ciclici (block-cyclic distribution), pensato per bilanciare il carico computazionale tra i processi anche per algoritmi con pattern di accesso non uniforme (es. eliminazione gaussiana con pivoting) |
| **FFTW** (Fastest Fourier Transform in the West) | Trasformata di Fourier veloce (FFT) | Include varianti parallele a memoria distribuita basate su MPI, che tipicamente richiedono una ridistribuzione dei dati tra processi tramite pattern di comunicazione all-to-all, concettualmente analogo a `MPI_Alltoall` (capitolo 02, sezione 7) |
| **PETSc** (Portable, Extensible Toolkit for Scientific Computation) | Framework per il calcolo scientifico distribuito | Fornisce strutture dati distribuite per matrici sparse e vettori, e solutori per sistemi lineari e non lineari (inclusi metodi iterativi di tipo Krylov, precondizionatori), con un'astrazione della comunicazione MPI sottostante |
| **Trilinos** | Framework per la simulazione numerica | Ecosistema modulare di pacchetti per algebra lineare distribuita, solutori, discretizzazioni e ottimizzazione, con obiettivi in parte sovrapponibili a PETSc, sviluppato principalmente presso i Sandia National Laboratories |
| **Eigen** | Libreria di algebra lineare C++ header-only | Non richiede compilazione/linking separato (l'intera libreria è distribuita come header C++ template); sfrutta espressioni template (expression templates) per fondere automaticamente operazioni concatenate (es. `A*B + C`) in un singolo ciclo di calcolo, evitando l'allocazione di risultati temporanei intermedi |
| **Armadillo** | Libreria di algebra lineare C++ ad alto livello | Sintassi ispirata a MATLAB/Octave, pensata per favorire la rapidità di prototipazione mantenendo prestazioni competitive tramite delega, ove disponibile, a BLAS/LAPACK sottostanti |

Esempio d'uso di Eigen, per il calcolo degli autovalori di una matrice 2×2:

```cpp
#include <Eigen/Dense>

Eigen::Matrix2d A;

A << 4, -2,
     1,  5;

Eigen::EigenSolver<Eigen::Matrix2d> solver(A);

auto eigenvalues = solver.eigenvalues();
```

Il vantaggio principale nell'affidarsi a queste librerie, invece di reimplementare gli algoritmi corrispondenti, è duplice: da un lato le implementazioni sono tipicamente ottimizzate a un livello (blocking per la cache, vettorizzazione manuale, tuning specifico per microarchitettura) difficilmente replicabile in tempi di sviluppo ragionevoli da un singolo progetto applicativo; dall'altro, la correttezza numerica di questi algoritmi (in particolare la loro stabilità numerica rispetto a errori di arrotondamento in virgola mobile) è stata validata estensivamente nel corso di decenni di uso in produzione, un livello di affidabilità difficile da raggiungere per un'implementazione scritta da zero.

## 5. Fortran e C++ nell'HPC

Fortran rimane, ancora oggi, uno dei linguaggi più rilevanti nel calcolo scientifico, per via della sua lunga storia (risale alla fine degli anni '50) e del vastissimo ecosistema di software numerico legacy scritto in questo linguaggio, ancora ampiamente utilizzato e mantenuto in molti codici di produzione HPC. Il C++ moderno raggiunge, in condizioni comparabili, prestazioni equivalenti a Fortran, offrendo in aggiunta meccanismi di astrazione (template, RAII, programmazione orientata agli oggetti) e strumenti di ingegneria del software (gestione della memoria automatizzata via RAII, sistema dei tipi più espressivo) generalmente più avanzati.

### 5.1 Ordinamento della memoria: row-major vs column-major

Una differenza tecnica rilevante, e fonte comune di bug prestazionali silenziosi quando si porta codice da un linguaggio all'altro, riguarda l'ordine di memorizzazione degli array multidimensionali:

| Linguaggio | Layout di memoria |
| --- | --- |
| Fortran | Column-major (per colonne) |
| C/C++ | Row-major (per righe) |

```fortran
real :: A(100,100)
A(i,j)
```

```cpp
double A[100][100];
A[i][j];
```

In un array bidimensionale **row-major** (C/C++), gli elementi di una stessa riga sono contigui in memoria: `A[i][j]` e `A[i][j+1]` differiscono di un solo elemento in indirizzo, mentre `A[i][j]` e `A[i+1][j]` differiscono di un'intera riga (100 elementi, nell'esempio sopra). In un array bidimensionale **column-major** (Fortran), vale l'esatto opposto: sono gli elementi di una stessa colonna a essere contigui, quindi `A(i,j)` e `A(i+1,j)` differiscono di un solo elemento, mentre `A(i,j)` e `A(i,j+1)` differiscono di un'intera colonna.

Questa differenza non è un dettaglio puramente accademico: determina direttamente quale **ordine di attraversamento** (nesting dei cicli) di un array multidimensionale produce accessi contigui alla memoria, e quindi buone prestazioni di cache, e quale ordine produce invece accessi con **stride** elevato (salti di memoria di dimensione pari a un'intera riga/colonna ad ogni iterazione del ciclo interno), penalizzando fortemente le prestazioni per via del conseguente aumento dei cache miss:

```cpp
// C/C++, row-major: il ciclo INTERNO deve scorrere sull'ULTIMO indice
// per ottenere accessi contigui in memoria
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        A[i][j] = ...;   // accesso contiguo: j varia più velocemente,
                          // coerente con il layout row-major
```

```fortran
! Fortran, column-major: il ciclo INTERNO deve scorrere sul PRIMO indice
do j = 1, N
  do i = 1, N
    A(i,j) = ...   ! accesso contiguo: i varia più velocemente,
                    ! coerente con il layout column-major
  end do
end do
```

Invertire l'ordine di nesting rispetto alla convenzione del linguaggio (ad esempio, in C++, iterare sull'indice di riga `i` nel ciclo più interno) produce codice **funzionalmente corretto** ma con un pattern di accesso a stride elevato, che in generale degrada sensibilmente le prestazioni, tanto più quanto la dimensione dell'array eccede la capacità delle cache più veloci: ogni accesso rischia di richiedere il caricamento di una nuova cache line, invece di riutilizzare quella già caricata dall'accesso precedente. Questo effetto è tipicamente osservabile e misurabile già per matrici di dimensione moderata (centinaia di elementi per lato), e diventa via via più marcato al crescere della dimensione del problema.

Questa considerazione non è puramente teorica nel contesto di questa repository: nel capitolo 03a (solutore di Jacobi), lo stencil a 5 punti applicato a una griglia bidimensionale accede sistematicamente ai quattro vicini cardinali di ogni nodo; la scelta di quale dimensione della griglia decomporre tra i processi (righe, come nella decomposizione a strisce presentata in quel capitolo, oppure colonne) e l'ordine con cui i cicli di aggiornamento locale vengono annidati nel codice C++ hanno un impatto diretto sulle prestazioni del kernel di calcolo locale, indipendentemente dal costo della comunicazione MPI associata.

### 5.2 Prestazioni: linguaggio vs implementazione

Per i kernel numerici, le prestazioni effettive sono determinate tipicamente più da:

* **Qualità del compilatore** e della sua infrastruttura di ottimizzazione (entrambi i principali compilatori usati in ambito HPC, GCC e LLVM/Clang, così come i compilatori Fortran ad essi associati o compilatori proprietari come Intel oneAPI, condividono in larga parte le stesse tecniche di ottimizzazione a basso livello).
* **Flag di ottimizzazione** scelti in fase di compilazione (sezione 6).
* **Pattern di accesso alla memoria** del codice sorgente (sezione 3 e 5.1).
* **Vettorizzazione**, automatica o assistita, del ciclo di calcolo.

che dal linguaggio di programmazione scelto in sé. In condizioni di ottimizzazione equivalenti (stessi flag, stesso pattern di accesso alla memoria, stesso livello di vettorizzazione ottenuto), implementazioni moderne in Fortran e in C++ tendono a mostrare prestazioni comparabili per la stessa classe di kernel numerici, poiché entrambi vengono compilati verso codice macchina dallo stesso tipo di infrastruttura di ottimizzazione del compilatore. Le differenze prestazionali osservate in pratica tra le due implementazioni di uno stesso algoritmo sono quindi, nella grande maggioranza dei casi, riconducibili a differenze nel pattern di accesso alla memoria effettivamente scritto dal programmatore (coerente o meno con il layout nativo del linguaggio, sezione 5.1) piuttosto che a limiti intrinseci dell'uno o dell'altro linguaggio.

## 6. Ottimizzazione a livello di compilatore

I flag di ottimizzazione comunemente usati includono:

```bash
-O3 -march=native
```

* **`-O3`** abilita il livello di ottimizzazione più aggressivo tra quelli standard offerti dal compilatore (superiore a `-O2`, usato invece come impostazione di riferimento più conservativa nei template di compilazione di questa repository, si vedano i capitoli successivi): include ottimizzazioni quali lo srotolamento automatico dei cicli (loop unrolling), l'inlining più aggressivo di funzioni, e — punto particolarmente rilevante per il codice numerico — un tentativo più esteso di vettorizzazione automatica dei cicli (auto-vectorization), trasformando cicli scalari in sequenze di istruzioni SIMD dove le dipendenze dati del codice lo consentono. `-O3`, in alcuni casi, può produrre codice più grande e, per pattern di codice non numerici, non garantisce sempre un miglioramento rispetto a `-O2`; per kernel numerici densi, tuttavia, è tipicamente la scelta preferita.
* **`-march=native`** istruisce il compilatore a generare codice ottimizzato specificamente per il set di istruzioni della CPU su cui avviene la compilazione stessa (rilevato automaticamente), permettendo l'uso di estensioni vettoriali eventualmente disponibili (AVX2, AVX-512, a seconda della microarchitettura) che non sarebbero incluse in un binario generico portabile. Il costo di questa scelta è la **perdita di portabilità**: un eseguibile compilato con `-march=native` su una determinata macchina può non funzionare correttamente (in genere fallendo con un'istruzione illegale a runtime) se eseguito su un processore diverso, privo delle estensioni per cui il codice è stato specificamente generato. Su un cluster HPC eterogeneo, o quando il binario deve essere distribuito su hardware non noto a priori, si preferisce tipicamente specificare esplicitamente una microarchitettura target compatibile con tutti i nodi di destinazione, piuttosto che affidarsi a `-march=native`.

In condizioni di ottimizzazione equivalenti, implementazioni moderne in Fortran e in C++ tendono quindi a mostrare prestazioni comparabili, come già osservato in sezione 5.2: la scelta dei flag di compilazione è, in pratica, un fattore di impatto prestazionale paragonabile alla scelta del linguaggio stesso, e va sempre documentata esplicitamente quando si riportano confronti di prestazioni tra implementazioni diverse di uno stesso algoritmo.

## 7. MPI e parallelismo a memoria distribuita

Mentre il parallelismo a memoria condivisa (ad esempio tramite thread, con modelli come OpenMP) si basa su unità di esecuzione che condividono lo stesso spazio di indirizzamento e possono quindi accedere direttamente alle stesse variabili in memoria, **MPI** abilita la comunicazione tra processi **indipendenti**, ciascuno con il proprio spazio di memoria privato, potenzialmente distribuiti su nodi fisicamente separati di un cluster e privi di qualunque meccanismo di accesso diretto alla memoria altrui.

Il modello di esecuzione MPI si fonda su:

* **Processi multipli**, ciascuno un'istanza a sé stante del programma, con la propria memoria, il proprio stack e le proprie variabili locali.
* **Scambio esplicito di messaggi** come unico meccanismo di comunicazione tra processi: nessun dato è condiviso implicitamente, ogni trasferimento di informazione deve essere codificato esplicitamente dal programmatore tramite una chiamata a una routine di comunicazione MPI (argomento centrale di tutti i capitoli successivi).
* **Memoria distribuita**: la somma della memoria disponibile a tutti i processi di un job MPI può eccedere quella di un singolo nodo fisico, permettendo di affrontare problemi la cui dimensione dei dati eccede la RAM disponibile su una singola macchina — motivazione pratica primaria per l'adozione di MPI in molte applicazioni HPC, oltre alla parallelizzazione del tempo di calcolo.
* **SPMD (Single Program, Multiple Data)**: un singolo eseguibile viene lanciato ripetutamente come processi distinti, ciascuno operante su una porzione diversa dei dati complessivi del problema, secondo il modello già descritto nel file indice della repository.

Ogni processo esegue lo stesso codice sorgente, ma opera su porzioni diverse dei dati, tipicamente distinguendo il proprio comportamento tramite il proprio **rank** (l'identificatore intero univoco assegnato a ciascun processo all'interno di un communicator, concetto approfondito a partire dal capitolo 01a):

```cpp
#include <mpi.h>
#include <iostream>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank;
    int size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::cout << "Process "
              << rank
              << " of "
              << size
              << std::endl;

    MPI_Finalize();

    return 0;
}
```

`MPI_Init` deve essere invocata prima di qualunque altra funzione MPI (con la sola eccezione di un numero ristretto di funzioni di query esplicitamente definite dallo standard come utilizzabili anche prima dell'inizializzazione), e `MPI_Finalize` deve essere l'ultima chiamata MPI eseguita da ciascun processo prima della terminazione: questa struttura "a sandwich" (`MPI_Init` all'inizio, `MPI_Finalize` alla fine, tutta la logica applicativa MPI nel mezzo) è comune a **tutti** i programmi MPI presentati in questa repository, e non verrà ripetuta esplicitamente in ogni singolo capitolo successivo.

MPI costituisce il fondamento della maggior parte delle applicazioni HPC su larga scala e rimane, a tutt'oggi, lo standard dominante per la programmazione parallela a memoria distribuita, adottato pressoché universalmente sui sistemi di calcolo ad alte prestazioni di livello mondiale (dai cluster dipartimentali ai sistemi exascale), spesso in combinazione con modelli di parallelismo a memoria condivisa a livello di singolo nodo (il cosiddetto modello ibrido MPI+OpenMP, o MPI+CUDA/HIP quando sono coinvolti acceleratori), non trattato in questa repository ma utile da conoscere come naturale estensione di quanto qui presentato.

## 8. Roadmap della repository

Il materiale è organizzato secondo livelli di complessità crescente:

```text
Introduzione (00)
      ↓
Comunicazione Point-to-Point Bloccante (01a)
      ↓
Comunicazione Point-to-Point Non Bloccante (01b)
      ↓
Comunicazione Collettiva (02)
      ↓
Solutore di Jacobi Distribuito (03a)
      ↓
Topologie Virtuali (03b)
```

Non è presente alcun capitolo 4: la progressione didattica di questa repository si conclude con il capitolo 03b, e non comprende (in questa versione della repository) un modulo dedicato al benchmarking sistematico o all'analisi comparativa delle prestazioni tra linguaggi, argomento comunque introdotto concettualmente in questo capitolo (sezione 5) come base per un eventuale approfondimento autonomo.

I capitoli finali (03a, 03b) combinano le primitive di comunicazione presentate nei capitoli precedenti (01a, 01b, 02) con un algoritmo numerico completo — il metodo iterativo di Jacobi — per mostrare un flusso di lavoro HPC realistico, basato su decomposizione del dominio e halo exchange, che costituisce il pattern di parallelizzazione più diffuso per la classe di algoritmi a stencil su griglia strutturata, ampiamente rappresentativa di una vasta gamma di solutori numerici per equazioni alle derivate parziali oltre al caso specifico del laplaciano trattato in questa repository.
