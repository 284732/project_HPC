# 04a — Topologie Virtuali (Griglia Cartesiana)

## Teoria

Una **topologia virtuale** MPI mappa i processi su una struttura geometrica logica (griglia, toro, ecc.). Non cambia le prestazioni fisiche, ma:
- Semplifica la gestione dei vicini (chi è a sinistra/destra/sopra/sotto?)
- Rende il codice più leggibile e manutenibile
- Permette a MPI di ottimizzare il mapping fisico su certi hardware

### MPI_Cart_create

```cpp
int MPI_Cart_create(
    MPI_Comm  comm_old,    // communicator di partenza
    int       ndims,       // numero di dimensioni (2 per griglia 2D)
    const int dims[],      // dimensioni: dims[0]=righe, dims[1]=colonne
    const int periods[],   // periodicità: 1=toroidale, 0=bordo fisso
    int       reorder,     // 1=MPI può riordinare i rank, 0=no
    MPI_Comm* comm_cart    // nuovo communicator cartesiano
);
```

### Funzioni principali

```cpp
// Ottieni le coordinate del processo corrente
MPI_Cart_coords(comm_cart, rank, ndims, coords);

// Ottieni il rank dato le coordinate
MPI_Cart_rank(comm_cart, coords, &rank);

// Trova i vicini lungo una dimensione
// direction=0 → verticale, direction=1 → orizzontale
// disp=+1 → avanti, disp=-1 → indietro
// MPI_PROC_NULL se fuori dai bordi (periodo=0)
MPI_Cart_shift(comm_cart, direction, disp, &rank_sorgente, &rank_destinazione);
```

### Schema di una griglia 4×3

```
         col 0    col 1    col 2
riga 0  [P0(0,0)][P1(0,1)][P2(0,2)]
riga 1  [P3(1,0)][P4(1,1)][P5(1,2)]
riga 2  [P6(2,0)][P7(2,1)][P8(2,2)]
riga 3  [P9(3,0)][P10(3,1)][P11(3,2)]
```

Per P4 (rank=4, coords=(1,1)):
- vicino NORD: P1 (rank=1)
- vicino SUD:  P7 (rank=7)
- vicino EST:  P5 (rank=5)
- vicino OVEST:P3 (rank=3)

### MPI_Cart_sub — Sottocommunicator

```cpp
// Estrae i communicator di riga o colonna dalla topologia cartesiana
int remain_dims_row[]  = {0, 1};  // mantieni dimensione colonne → comm per riga
int remain_dims_col[]  = {1, 0};  // mantieni dimensione righe   → comm per colonna

MPI_Comm comm_riga, comm_colonna;
MPI_Cart_sub(comm_cart, remain_dims_row, &comm_riga);
MPI_Cart_sub(comm_cart, remain_dims_col, &comm_colonna);
```

---

## Esercizi

### Esercizio 1 — Creazione griglia e stampa vicini (`ex1_cart_create.cpp`)
Crea una griglia 2D, ogni processo stampa le proprie coordinate e i rank dei vicini.

### Esercizio 2 — Halo exchange su griglia (`ex2_halo_exchange.cpp`)
Ogni processo scambia i valori di bordo con i 4 vicini usando `MPI_Sendrecv`.

### Esercizio 3 — MPI_Dims_create (`ex3_dims_create.cpp`)
Usa `MPI_Dims_create` per trovare automaticamente la decomposizione ottimale.

---

## Output Atteso

### ex1_cart_create (6 processi, griglia 2×3)
```
Processo 0 → coords(0,0) | N=MPI_PROC_NULL S=3 O=MPI_PROC_NULL E=1
Processo 1 → coords(0,1) | N=MPI_PROC_NULL S=4 O=0             E=2
Processo 2 → coords(0,2) | N=MPI_PROC_NULL S=5 O=1             E=MPI_PROC_NULL
Processo 3 → coords(1,0) | N=0             S=MPI_PROC_NULL O=MPI_PROC_NULL E=4
...
```
