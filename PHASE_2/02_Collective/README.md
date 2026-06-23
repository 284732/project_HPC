# 03 — Comunicazione Collettiva

## Teoria

Le operazioni collettive coinvolgono **tutti** i processi di un communicator simultaneamente. Sono più efficienti delle equivalenti P2P (usano algoritmi ad albero, pipeline, ecc.) e semplificano il codice.

> ⚠️ **Regola fondamentale**: ogni processo nel communicator **deve** chiamare la stessa funzione collettiva. Non c'è un mittente e un destinatario: tutti partecipano.

---

### Panoramica delle operazioni

```
MPI_Bcast      → 1 processo invia a tutti
MPI_Scatter    → 1 processo distribuisce parti a ciascuno
MPI_Gather     → tutti inviano al processo root che li raccoglie
MPI_Allgather  → tutti raccolgono da tutti
MPI_Reduce     → tutti contribuiscono, root ottiene il risultato
MPI_Allreduce  → tutti contribuiscono, TUTTI ottengono il risultato
MPI_Alltoall   → ogni processo invia dati diversi a ogni altro
MPI_Barrier    → sincronizzazione: tutti aspettano che tutti arrivino
```

---

### MPI_Bcast

```
Prima:  Proc 0 [X]  Proc 1 [?]  Proc 2 [?]  Proc 3 [?]
                  ↓
Dopo:   Proc 0 [X]  Proc 1 [X]  Proc 2 [X]  Proc 3 [X]
```
```cpp
MPI_Bcast(buf, count, datatype, root, comm);
// root: chi ha il dato iniziale (il "mittente" della broadcast)
// tutti gli altri ricevono in buf
```

### MPI_Scatter / MPI_Gather

```
Scatter:
  Root [A B C D]  →  Proc 0:[A]  Proc 1:[B]  Proc 2:[C]  Proc 3:[D]

Gather:
  Proc 0:[A]  Proc 1:[B]  Proc 2:[C]  Proc 3:[D]  →  Root [A B C D]
```
```cpp
MPI_Scatter(sendbuf, sendcount, sendtype,
            recvbuf, recvcount, recvtype, root, comm);

MPI_Gather(sendbuf, sendcount, sendtype,
           recvbuf, recvcount, recvtype, root, comm);
```

### MPI_Reduce

```
Proc 0:[2]  Proc 1:[5]  Proc 2:[3]  Proc 3:[1]
              ↓  MPI_SUM
            Root:[11]
```

**Operazioni di riduzione disponibili:**

| Costante MPI   | Operazione      |
|----------------|-----------------|
| `MPI_SUM`      | somma           |
| `MPI_PROD`     | prodotto        |
| `MPI_MAX`      | massimo         |
| `MPI_MIN`      | minimo          |
| `MPI_LAND`     | AND logico      |
| `MPI_LOR`      | OR logico       |
| `MPI_MAXLOC`   | max + indice    |
| `MPI_MINLOC`   | min + indice    |

```cpp
MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
// Allreduce: tutti ottengono il risultato, non solo root
```

---

## Esercizi

### Esercizio 1 — Bcast e calcolo del PI (`ex1_bcast_pi.cpp`)
Il root brodacast i parametri, ogni worker calcola una parte del PI con la serie di Leibniz, poi si fa una Reduce finale.

### Esercizio 2 — Scatter/Gather: somma distribuita (`ex2_scatter_gather.cpp`)
Il root ha un vettore grande. Lo scattera, ogni worker elabora la propria parte, si fa il gather del risultato.

### Esercizio 3 — Allreduce: norma distribuita (`ex3_allreduce.cpp`)
Ogni processo ha un vettore locale. Calcola la norma L2 globale con Allreduce (tutti ottengono il risultato).

### Esercizio 4 — Alltoall: trasposizione distribuita (`ex4_alltoall.cpp`)
Ogni processo invia dati diversi a ogni altro processo: base della FFT distribuita.

---

## Output Atteso

### ex1_bcast_pi (4 processi)
```
[Root] N_termini = 1000000, broadcaster a tutti.
[Processo 0] Calcolo termini 0..249999
[Processo 1] Calcolo termini 250000..499999
...
PI approssimato = 3.14159265...  (errore: 9.3e-7)
```
