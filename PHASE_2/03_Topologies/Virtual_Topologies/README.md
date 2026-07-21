# 03b — Topologie Virtuali in MPI (Griglia Cartesiana)

> Capitolo di riferimento sulle topologie virtuali cartesiane in MPI. Presuppone la lettura dei capitoli precedenti (01a/01b — comunicazione point-to-point, 02 — comunicazione collettiva, 03a — solutore di Jacobi, dove `MPI_Sendrecv` e il concetto di halo exchange sono già stati introdotti in dettaglio). Qui `MPI_Sendrecv` viene riutilizzata senza essere ridefinita.

---

## Indice

1. [Cos'è una topologia virtuale e perché esiste](#1-cosè-una-topologia-virtuale-e-perché-esiste)
2. [MPI_Cart_create: creazione della griglia cartesiana](#2-mpi_cart_create-creazione-della-griglia-cartesiana)
3. [Corrispondenza tra rank e coordinate: ordinamento row-major](#3-corrispondenza-tra-rank-e-coordinate-ordinamento-row-major)
4. [MPI_Cart_coords e MPI_Cart_rank](#4-mpi_cart_coords-e-mpi_cart_rank)
5. [MPI_Cart_shift: individuazione dei vicini](#5-mpi_cart_shift-individuazione-dei-vicini)
6. [MPI_Dims_create: bilanciamento automatico delle dimensioni](#6-mpi_dims_create-bilanciamento-automatico-delle-dimensioni)
7. [Esercizi guidati](#7-esercizi-guidati)
8. [Output atteso e come interpretarlo](#8-output-atteso-e-come-interpretarlo)
9. [Errori comuni e come evitarli](#9-errori-comuni-e-come-evitarli)

---

## 1. Cos'è una topologia virtuale e perché esiste

Una **topologia virtuale** in MPI è una struttura logica (griglia, toro, grafo generico) che viene sovrapposta all'insieme di processi di un communicator, associando a ciascun rank una posizione all'interno di questa struttura, oltre al semplice intero sequenziale `0..P-1` già disponibile in un communicator ordinario.

È fondamentale chiarire subito cosa una topologia virtuale **non è**: non modifica in alcun modo l'hardware fisico sottostante, non sposta processi tra nodi di calcolo, e non altera di per sé le prestazioni di comunicazione. Il termine "virtuale" è scelto proprio in contrapposizione a "fisica": la topologia è un'astrazione puramente logica, costruita sopra un communicator esistente, che semplifica il modo in cui il programmatore esprime e ragiona sulle relazioni di adiacenza tra processi.

I vantaggi concreti offerti da una topologia virtuale cartesiana sono tre, distinti tra loro:

* **Semplificazione della gestione dei vicini.** In un problema di decomposizione del dominio su griglia 2D (ad esempio una generalizzazione bidimensionale del problema di Jacobi trattato nel capitolo 04a, dove là la decomposizione era 1D a strisce), ogni processo deve individuare i rank dei propri vicini nord/sud/est/ovest. Senza topologia virtuale, questo richiede calcoli aritmetici espliciti sul rank lineare (tipicamente basati su divisioni intere e moduli) ripetuti e soggetti a errore in ogni punto del codice in cui servono; con `MPI_Cart_shift` (sezione 5), questo calcolo è delegato a una singola chiamata di libreria, con gestione automatica anche dei casi limite (bordi della griglia, periodicità).
* **Leggibilità e manutenibilità del codice.** Esprimere la posizione di un processo come coordinate `(riga, colonna)` anziché come singolo intero rank rende il codice direttamente tracciabile rispetto alla struttura logica del problema che si sta parallelizzando, riducendo la probabilità di errori di indicizzazione nei calcoli manuali di rank dei vicini.
* **Possibilità di ottimizzazione del mapping fisico.** Il parametro `reorder` di `MPI_Cart_create` (sezione 2) consente all'implementazione MPI di **riassegnare** i rank all'interno del nuovo communicator cartesiano, potenzialmente in modo da far coincidere la topologia logica richiesta con la topologia fisica reale dell'interconnessione di rete (ad esempio una rete a toro fisica, comune in molti sistemi HPC). Questa ottimizzazione è **facoltativa e dipendente dall'implementazione**.

## 2. MPI_Cart_create: creazione della griglia cartesiana

```cpp
int MPI_Cart_create(
    MPI_Comm  comm_old,    // communicator di partenza (tipicamente MPI_COMM_WORLD)
                            // da cui derivare la topologia cartesiana
    int       ndims,       // numero di dimensioni della griglia (2 per una
                            // griglia 2D, 3 per una griglia 3D, ecc.)
    const int dims[],      // dimensione di ciascun asse: dims[0] = numero di
                            // righe, dims[1] = numero di colonne (per ndims=2).
                            // Il prodotto di tutti i dims[i] NON deve superare
                            // il numero di processi in comm_old
    const int periods[],   // periodicità per ciascuna dimensione: periods[i]=1
                            // rende l'asse i-esimo periodico (topologia toroidale,
                            // il vicino "oltre" l'ultimo elemento è il primo),
                            // periods[i]=0 rende l'asse a confine fisso (i
                            // processi ai bordi non hanno vicino su quel lato)
    int       reorder,     // reorder=1: l'implementazione MPI PUÒ riassegnare
                            // i rank nel nuovo communicator per ottimizzare il
                            // mapping fisico (sezione 1); reorder=0: i rank nel
                            // nuovo communicator restano identici a quelli in
                            // comm_old, nello stesso ordine
    MPI_Comm* comm_cart    // OUTPUT: nuovo communicator con topologia cartesiana
                            // associata, distinto da comm_old
);
```

Alcune precisazioni tecniche rilevanti, spesso fonte di errore per chi usa questa funzione per la prima volta:

* Se il prodotto `dims[0] × dims[1] × ... × dims[ndims-1]` è **strettamente minore** del numero di processi disponibili in `comm_old`, i processi "in eccesso" (quelli il cui rank originale non trova posto nella griglia) ricevono `MPI_COMM_NULL` come valore di `comm_cart` e non partecipano alla topologia risultante: è responsabilità del programmatore verificare che il numero di processi lanciati corrisponda esattamente (o sia gestito esplicitamente per il caso di eccesso) al prodotto delle dimensioni richieste.
* Se il prodotto delle dimensioni **eccede** il numero di processi disponibili, la chiamata è un errore MPI (tipicamente terminazione del programma con codice di errore, a seconda della gestione degli errori configurata).
* `MPI_Cart_create` è essa stessa una **operazione collettiva** (implicitamente, per la natura di come i communicator vengono costruiti in MPI): deve essere invocata da tutti i processi di `comm_old`, con argomenti `ndims`, `dims[]` e `periods[]` consistenti su tutti i chiamanti, per lo stesso motivo di consistenza discusso per le collettive generiche nel capitolo 02(sezione 1).
* Il communicator `comm_cart` restituito è un nuovo communicator, **distinto** da `comm_old`: le operazioni di comunicazione (P2P o collettive) successive vanno indirizzate a `comm_cart` se si desidera operare all'interno del gruppo di processi della griglia (che coincide con quello di `comm_old` a meno del caso di eccesso di processi discusso sopra), mentre `comm_old` resta comunque valido e utilizzabile indipendentemente.

## 3. Corrispondenza tra rank e coordinate: ordinamento row-major

Quando `MPI_Cart_create` costruisce la griglia (con `reorder=0`, il caso più semplice da ragionare esplicitamente), assegna alle coordinate cartesiane un ordinamento **row-major** rispetto al rank lineare originale: il rank cresce scorrendo prima lungo l'ultima dimensione (le colonne, per una griglia 2D), e solo al termine di ogni riga passa alla riga successiva — esattamente la stessa convenzione di memorizzazione row-major usata per gli array multidimensionali in C/C++.

Per una griglia 4×3 (`dims = {4, 3}`, quindi 4 righe e 3 colonne, per un totale di 12 processi), la corrispondenza tra rank lineare e coordinate `(riga, colonna)` è:

```
             col 0      col 1      col 2
riga 0   [P0 (0,0)] [P1 (0,1)] [P2 (0,2)]
riga 1   [P3 (1,0)] [P4 (1,1)] [P5 (1,2)]
riga 2   [P6 (2,0)] [P7 (2,1)] [P8 (2,2)]
riga 3   [P9 (3,0)] [P10(3,1)] [P11(3,2)]
```

La relazione generale, per una griglia con `dims[1]` colonne, è:

```
rank = coords[0] * dims[1] + coords[1]
```

ossia esattamente la formula di linearizzazione di un array 2D memorizzato in ordine row-major, dove `coords[0]` è l'indice di riga e `coords[1]` l'indice di colonna. Questa relazione è quella implementata internamente da `MPI_Cart_rank` (conversione coordinate → rank) e dalla sua inversa da `MPI_Cart_coords` (rank → coordinate), descritte nella sezione seguente. Va sottolineato che questa corrispondenza esplicita vale quando `reorder=0`: con `reorder=1`, l'implementazione MPI può assegnare i rank nella griglia secondo un ordine diverso (dipendente dall'implementazione, orientato all'ottimizzazione del mapping fisico discussa in sezione 1), e il programmatore non deve fare affidamento su un ordinamento row-major esplicito basato sul rank originale in quel caso — deve invece sempre interrogare `MPI_Cart_coords`/`MPI_Cart_rank` per ottenere la corrispondenza effettiva, piuttosto che calcolarla manualmente.

## 4. MPI_Cart_coords e MPI_Cart_rank

Queste due funzioni sono l'una l'inversa dell'altra, e permettono di convertire tra la rappresentazione "rank lineare" e la rappresentazione "coordinate cartesiane" di un processo all'interno della topologia.

```cpp
int MPI_Cart_coords(
    MPI_Comm comm_cart,  // il communicator con topologia cartesiana
    int      rank,       // il rank (in comm_cart) di cui si vogliono le coordinate
    int      ndims,      // numero di dimensioni della griglia (deve corrispondere
                          // a quello usato in MPI_Cart_create)
    int      coords[]    // OUTPUT: array di ndims interi, riempito con le
                          // coordinate del processo di rank specificato
);

int MPI_Cart_rank(
    MPI_Comm comm_cart,   // il communicator con topologia cartesiana
    const int coords[],   // le coordinate di cui si vuole il rank corrispondente
    int*     rank         // OUTPUT: il rank corrispondente a quelle coordinate
);
```

Un uso tipico è ottenere le proprie coordinate a inizio programma, immediatamente dopo la creazione della topologia:

```cpp
int rank, coords[2];
MPI_Comm_rank(comm_cart, &rank);           // rank di QUESTO processo in comm_cart
MPI_Cart_coords(comm_cart, rank, 2, coords); // le proprie coordinate (riga, colonna)
```

Da notare che `MPI_Cart_coords` accetta come parametro un rank **arbitrario**, non necessariamente quello del processo chiamante: è quindi possibile, da un qualunque processo, interrogare le coordinate di un altro processo di cui si conosce il rank, utile ad esempio per scopi diagnostici o di logging centralizzato. Analogamente, `MPI_Cart_rank` può essere invocata per determinare il rank corrispondente a coordinate arbitrarie, anche non corrispondenti al processo chiamante — operazione particolarmente utile quando si deve indirizzare esplicitamente una comunicazione verso un processo identificato per posizione logica nella griglia piuttosto che per rank, ad esempio "il processo nell'angolo opposto della griglia" o "il processo nella stessa colonna ma riga 0".

## 5. MPI_Cart_shift: individuazione dei vicini

`MPI_Cart_shift` è la funzione centrale per la gestione dei vicini in una topologia cartesiana: calcola, per il processo chiamante, i rank dei vicini lungo una specifica dimensione e in entrambe le direzioni (avanti e indietro) rispetto a quella dimensione, in un'unica chiamata.

```cpp
int MPI_Cart_shift(
    MPI_Comm comm_cart,        // il communicator con topologia cartesiana
    int      direction,        // l'asse lungo cui cercare i vicini:
                                // 0 → verticale (lungo le righe, quindi
                                //     variando coords[0]),
                                // 1 → orizzontale (lungo le colonne, quindi
                                //     variando coords[1])
    int      disp,             // spostamento (displacement) da applicare:
                                // disp=+1 → vicino "in avanti" lungo l'asse
                                //     (coordinata incrementata di 1),
                                // disp=-1 → vicino "all'indietro" lungo l'asse
                                //     (coordinata decrementata di 1).
                                // Valori |disp|>1 sono ammessi dallo standard
                                // e restituiscono il vicino a distanza disp,
                                // ma l'uso più comune è ±1 per i vicini immediati
    int*     rank_source,      // OUTPUT: il rank del processo da cui QUESTO
                                // processo riceverebbe, se effettuasse uno
                                // spostamento nella direzione OPPOSTA a disp
                                // (utile passandolo direttamente come 'source'
                                // di una successiva MPI_Sendrecv)
    int*     rank_destination  // OUTPUT: il rank del processo verso cui QUESTO
                                // processo invierebbe, spostandosi nella
                                // direzione indicata da disp (utile come
                                // 'dest' di una MPI_Sendrecv)
);
```

Il punto concettualmente più delicato di questa funzione, spesso frainteso, è la relazione tra `disp` e i due output `rank_source`/`rank_destination`: **non** restituiscono semplicemente "il vicino avanti" e "il vicino indietro" in modo simmetrico e indipendente, ma sono pensati esplicitamente per essere passati **direttamente** come parametri `source` e `dest` di una singola chiamata `MPI_Sendrecv` (capitolo 03a, sezione 2.5), realizzando in un solo passo uno scambio di dati bidirezionale lungo quella direzione:

```cpp
int rank_source, rank_dest;
MPI_Cart_shift(comm_cart, /*direction=*/0, /*disp=*/+1, &rank_source, &rank_dest);

// rank_dest   = il vicino a cui inviare i miei dati (un passo avanti, disp=+1)
// rank_source = il vicino da cui ricevere dati (un passo indietro rispetto a me,
//               ovvero il processo per cui IO sono il vicino "avanti")

MPI_Sendrecv(
    my_data, count, MPI_DOUBLE, rank_dest,   TAG,
    recv_buf, count, MPI_DOUBLE, rank_source, TAG,
    comm_cart, MPI_STATUS_IGNORE
);
```

Per ottenere sia il vicino nord che il vicino sud (o est/ovest) di un processo, sono quindi necessarie **due chiamate distinte** a `MPI_Cart_shift`, una con `disp=+1` e una con `disp=-1` sulla stessa `direction`, esattamente come nella tabella di esempio della sezione successiva.

**Comportamento ai bordi della griglia.** Se `periods[direction]=0` (asse non periodico) e lo spostamento richiesto porterebbe fuori dai limiti della griglia (ad esempio, chiedere il vicino "a nord" per un processo già sulla prima riga), la funzione non genera un errore: restituisce la costante speciale `MPI_PROC_NULL` per l'output corrispondente. Come già accennato nel capitolo 03a (sezione 2.5), `MPI_PROC_NULL` è un rank "fittizio" verso cui/da cui ogni operazione di comunicazione MPI (incluso `MPI_Sendrecv`) è garantita dallo standard essere un no-op immediato, senza necessità di logica condizionale esplicita nel codice applicativo per gestire il caso di bordo. Se invece `periods[direction]=1` (asse periodico), lo spostamento "avvolge" automaticamente attorno alla griglia (topologia toroidale): il vicino "a nord" del processo sulla prima riga è il processo sull'ultima riga della stessa colonna, e la funzione restituisce quel rank invece di `MPI_PROC_NULL`.

### Esempio: i quattro vicini del processo P4 in una griglia 4×3

Per il processo di rank 4, coordinate `(1,1)`, nella griglia 4×3 di sezione 3:

```
NORTH (direction=0, disp=-1): P1  (rank=1, coords=(0,1))
SOUTH (direction=0, disp=+1): P7  (rank=7, coords=(2,1))
WEST  (direction=1, disp=-1): P3  (rank=3, coords=(1,0))
EAST  (direction=1, disp=+1): P5  (rank=5, coords=(1,2))
```

Da notare la corrispondenza: `direction=0` agisce sulla prima coordinata (`coords[0]`, la riga), quindi produce vicini in direzione verticale (nord/sud); `direction=1` agisce sulla seconda coordinata (`coords[1]`, la colonna), producendo vicini in direzione orizzontale (est/ovest). Questa corrispondenza `direction ↔ asse geometrico` è una convenzione applicativa (nord/sud/est/ovest sono etichette scelte dal programmatore in base a come interpreta le due dimensioni della griglia), non un vincolo imposto dallo standard MPI, che tratta `direction` semplicemente come indice di una delle `ndims` dimensioni astratte della griglia.

## 6. MPI_Dims_create: bilanciamento automatico delle dimensioni

Scegliere manualmente `dims[]` per `MPI_Cart_create` richiede di conoscere a priori una fattorizzazione del numero di processi P che produca una griglia ragionevolmente bilanciata. `MPI_Dims_create` automatizza questa scelta:

```cpp
int MPI_Dims_create(
    int nnodes,   // numero totale di processi da disporre nella griglia
                   // (tipicamente il size del communicator di partenza)
    int ndims,    // numero di dimensioni della griglia desiderata
    int dims[]    // INPUT/OUTPUT: le dimensioni già FISSATE dal chiamante
                   // (valore diverso da 0) vengono rispettate e non alterate;
                   // le dimensioni lasciate a 0 dal chiamante vengono calcolate
                   // automaticamente da MPI, in modo che il prodotto finale di
                   // tutti i dims[i] sia esattamente pari a nnodes
);
```

La funzione tenta di produrre dimensioni il più possibile **equilibrate tra loro**, un obiettivo motivato dal fatto che, a parità di numero totale di processi, una griglia "quadrata" (o quasi) minimizza tipicamente il rapporto superficie/volume dei sottodomini in una decomposizione spaziale 2D, riducendo il volume di comunicazione di halo exchange per unità di lavoro computazionale rispetto a una griglia fortemente "allungata" (ad esempio 1×P, che degenera di fatto in una decomposizione 1D a strisce come quella della guida 03a).

Esempio d'uso, con entrambe le dimensioni lasciate libere (`dims[] = {0, 0}` in ingresso):

```cpp
int size;
MPI_Comm_size(MPI_COMM_WORLD, &size);

int dims[2] = {0, 0};        // entrambe le dimensioni da determinare automaticamente
MPI_Dims_create(size, 2, dims);
// Per size=12: dims diventa {4,3} oppure {3,4} (a seconda della strategia di
// fattorizzazione interna dell'implementazione), non {12,1} né {1,12},
// poiché 4×3 è la fattorizzazione più bilanciata di 12 in due fattori

MPI_Comm comm_cart;
int periods[2] = {0, 0};
MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &comm_cart);
```

È anche possibile **vincolare** esplicitamente una sola dimensione e lasciare che `MPI_Dims_create` determini l'altra, impostando a un valore diverso da zero solo la componente che si vuole fissare:

```cpp
int dims[2] = {2, 0};  // vincola esplicitamente 2 righe, lascia libere le colonne
MPI_Dims_create(size, 2, dims);
// Per size=12: dims diventa {2, 6}, rispettando il vincolo dims[0]=2
```

Se il numero di processi `nnodes` non ammette una fattorizzazione compatibile con i vincoli già fissati dal chiamante (ad esempio richiedere `dims[0]=5` con `size=12`, che non è divisibile per 5), la chiamata è un errore MPI. È inoltre importante notare che, se **tutte** le dimensioni sono già state fissate esplicitamente dal chiamante (nessun valore a 0 in ingresso), `MPI_Dims_create` si limita a verificarne la consistenza rispetto a `nnodes` (il prodotto deve corrispondere esattamente), senza modificare alcun valore.

## 7. Esercizi guidati

### Esercizio 1 — Creazione della griglia e stampa dei vicini (`ex1_cart_create.cpp`)

Crea una griglia cartesiana 2D con `MPI_Cart_create`. Ogni processo determina le proprie coordinate con `MPI_Cart_coords` e i rank dei propri quattro vicini (nord, sud, ovest, est) con quattro chiamate a `MPI_Cart_shift` (due per l'asse verticale con `disp=±1`, due per l'asse orizzontale con `disp=±1`), stampando il risultato.

**Obiettivo:** familiarizzare con la sequenza minima di chiamate necessaria per istanziare una topologia cartesiana e interrogarla (`Cart_create` → `Cart_coords` → `Cart_shift`), e osservare direttamente in output la comparsa di `MPI_PROC_NULL` per i processi posizionati sui bordi della griglia, verificando la correttezza della gestione dei casi limite descritta in sezione 5.

### Esercizio 2 — Halo exchange su griglia (`ex2_halo_exchange.cpp`)

Ogni processo scambia i valori di confine con i propri quattro vicini usando `MPI_Sendrecv`, applicando lo stesso pattern di comunicazione già visto nella guida 04b (sezione 2.5) per l'halo exchange 1D, ma generalizzato a due dimensioni: qui ogni processo ha fino a quattro vicini (anziché due), corrispondenti ai quattro lati di una porzione bidimensionale di dominio, anziché ai soli lati nord/sud di una striscia 1D.

**Obiettivo:** generalizzare il pattern di decomposizione del dominio e halo exchange dalla griglia 1D a strisce (guida 04b) a una decomposizione 2D a blocchi, osservando come l'uso di `MPI_Cart_shift` elimini la necessità di calcolare manualmente i rank dei quattro vicini (calcolo che, in una decomposizione 2D a blocchi indicizzata manualmente, sarebbe sensibilmente più soggetto a errore rispetto al caso 1D per via della doppia indicizzazione riga/colonna).

### Esercizio 3 — MPI_Dims_create (`ex3_dims_create.cpp`)

Utilizza `MPI_Dims_create` per determinare automaticamente la decomposizione più bilanciata possibile per un dato numero di processi, senza fissare alcuna dimensione a priori (`dims[] = {0, 0}` in ingresso).

**Obiettivo:** osservare il comportamento di `MPI_Dims_create` su valori di `size` con fattorizzazioni diverse (numeri con molti fattori, come 12 o 16, rispetto a numeri primi, come 7 o 13, per i quali l'unica fattorizzazione bilanciata possibile in due fattori interi è banale, tipicamente `1×P`), comprendendo empiricamente il legame tra la fattorizzazione aritmetica di `size` e la "qualità" geometrica (rapporto tra i lati) della griglia risultante.

## 8. Output atteso e come interpretarlo

### ex1_cart_create (eseguito con `-np 6`, griglia 2×3)

```text
Process 0 → coords(0,0) | N=MPI_PROC_NULL  S=3  W=MPI_PROC_NULL  E=1
Process 1 → coords(0,1) | N=MPI_PROC_NULL  S=4  W=0              E=2
Process 2 → coords(0,2) | N=MPI_PROC_NULL  S=5  W=1              E=MPI_PROC_NULL
Process 3 → coords(1,0) | N=0              S=MPI_PROC_NULL  W=MPI_PROC_NULL  E=4
...
```

Con `size=6` e una griglia `dims={2,3}` (2 righe, 3 colonne, coerente con la convenzione row-major di sezione 3), i rank 0, 1, 2 occupano la riga 0 e i rank 3, 4, 5 la riga 1. Ogni processo della riga 0 ha correttamente `N=MPI_PROC_NULL` (nessun vicino a nord, essendo sulla prima riga e la griglia non periodica in questo esempio, `periods[0]=0`), mentre il rispettivo vicino a sud è il processo nella stessa colonna sulla riga 1 (ad esempio, per il processo 0 in `(0,0)`, il vicino sud è il processo 3 in `(1,0)`, coerente con la relazione `rank = coords[0]*dims[1] + coords[1]` di sezione 3: `1*3+0=3`). Analogamente, i processi in colonna 0 (P0, P3) hanno `W=MPI_PROC_NULL` e i processi in colonna 2 (P2, P5, non mostrato ma deducibile per simmetria) hanno `E=MPI_PROC_NULL`, coerentemente con l'assenza di periodicità anche sull'asse orizzontale. Si noti che ogni valore `S`/`N`/`E`/`W` diverso da `MPI_PROC_NULL` è simmetrico e reciproco tra coppie di processi adiacenti: il vicino sud di P0 è P3, e — come atteso — il vicino nord di P3 è P0, verificabile nella riga corrispondente dell'output.

## 9. Errori comuni e come evitarli

| Errore | Causa tipica | Come evitarlo |
|---|---|---|
| `MPI_Cart_create` termina il programma con errore | Il prodotto delle `dims[]` fornite eccede il numero di processi disponibili nel communicator di partenza | Verificare `dims[0] * dims[1] * ... == size` (o `≤ size`, gestendo esplicitamente i processi in eccesso) prima della chiamata, tipicamente derivando `dims[]` da `MPI_Dims_create` (sezione 6) invece di valori scelti arbitrariamente |
| I rank dei vicini calcolati "a mano" (senza `MPI_Cart_shift`) risultano errati quando `reorder=1` | Assunzione implicita che il rank nel communicator cartesiano coincida con il rank originale in `comm_old`, violata quando l'implementazione MPI riassegna i rank per ottimizzazione del mapping fisico (sezione 1) | Con `reorder=1`, non calcolare mai manualmente i rank dei vicini a partire dal rank originale: usare sempre `MPI_Cart_shift`/`MPI_Cart_coords`/`MPI_Cart_rank`, che riflettono correttamente il mapping effettivo qualunque esso sia |
| Confusione tra `direction` e l'etichetta geometrica nord/sud/est/ovest usata nel proprio codice | `direction=0` e `direction=1` sono indici astratti delle dimensioni della griglia (sezione 5); l'associazione a "verticale" o "orizzontale", e conseguentemente a nord/sud o est/ovest, è una convenzione scelta dal programmatore in fase di progettazione, non imposta da MPI | Documentare esplicitamente, a inizio codice, la convenzione scelta (es. "dims[0]=righe → direction=0 è l'asse verticale") e mantenerla coerente in tutte le chiamate a `MPI_Cart_shift` del programma |
| Comunicazione bloccata indefinitamente (deadlock) nell'halo exchange 2D dell'esercizio 2 | Gestione non simmetrica dei quattro vicini, ad esempio usando `MPI_Send`/`MPI_Recv` separati invece di `MPI_Sendrecv` per ciascuna coppia di direzioni opposte (lo stesso scenario generale già discusso in guida 01a sezione 6 e in guida 03a sezione 2.5, qui replicato su quattro direzioni anziché due) | Usare sempre `MPI_Sendrecv` per ciascuna coppia direzione/verso opposto (una chiamata per l'asse verticale, una per l'asse orizzontale), passando direttamente gli output di `MPI_Cart_shift` come parametri `dest`/`source`, così come mostrato in sezione 5 |
