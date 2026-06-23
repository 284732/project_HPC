# 01b — Comunicazione Point-to-Point Non Bloccante

## Teoria

Le operazioni **non bloccanti** restituiscono il controllo immediatamente, permettendo al processo di continuare a lavorare mentre la comunicazione avviene in background. La completezza dell'operazione viene verificata in un secondo momento.

### Il pattern generale

```
MPI_Isend / MPI_Irecv   →  avvia l'operazione (torna subito)
         [lavoro utile]  →  computa qualcosa mentre i dati viaggiano
MPI_Wait / MPI_Waitall   →  aspetta che l'operazione sia terminata
```

### MPI_Isend e MPI_Irecv

```cpp
MPI_Request request;  // handle che identifica l'operazione in corso

// Avvia invio non bloccante
MPI_Isend(buf, count, datatype, dest, tag, comm, &request);

// Avvia ricezione non bloccante
MPI_Irecv(buf, count, datatype, source, tag, comm, &request);

// ... computa qualcosa ...

// Aspetta il completamento
MPI_Wait(&request, MPI_STATUS_IGNORE);
```

> ⚠️ **Importante**: non accedere al buffer tra `MPI_Isend`/`MPI_Irecv` e `MPI_Wait`! Il contenuto è "in uso" da MPI.

### Funzioni di completamento

| Funzione | Descrizione |
|----------|-------------|
| `MPI_Wait(&req, &status)` | Blocca finché `req` è completa |
| `MPI_Waitall(n, reqs[], statuses[])` | Aspetta che **tutte** le `n` richieste siano complete |
| `MPI_Waitany(n, reqs[], &idx, &status)` | Aspetta che **almeno una** sia completa |
| `MPI_Test(&req, &flag, &status)` | Non bloccante: controlla se è completa (flag = 0/1) |

### Vantaggio: Overlap Computazione-Comunicazione

```
Bloccante:  [Send████████][Recv████████][Computa████████]
                                              totale: 3T

Non-bloccante: [Isend][Computa████████][Wait][Irecv][Wait]
                           totale: T + overhead
```

Nei problemi reali (Jacobi, FFT distribuita, ecc.) il vantaggio può essere significativo.

---

## Esercizi

### Esercizio 1 — Isend/Irecv base (`ex1_nonblocking_basic.cpp`)
Riscrittura del ping-pong con operazioni non bloccanti.

### Esercizio 2 — Overlap compute-communicate (`ex2_overlap.cpp`)
Calcola una somma locale mentre invia dati al vicino. Confronta i tempi.

### Esercizio 3 — Waitall con comunicazioni multiple (`ex3_waitall.cpp`)
Il master lancia N invii non bloccanti contemporaneamente, poi aspetta con `MPI_Waitall`.

---

## Output Atteso

### ex3_waitall (con 4 processi)
```
[Master] Avvio 3 Isend non bloccanti...
[Master] Faccio altro lavoro mentre i dati viaggiano...
[Master] MPI_Waitall: tutte le comunicazioni completate.
[Worker 1] Ricevuto: 100
[Worker 2] Ricevuto: 200
[Worker 3] Ricevuto: 300
```
