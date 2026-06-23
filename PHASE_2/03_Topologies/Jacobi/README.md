# 04b — Solver di Jacobi 2D con MPI

## Il problema fisico

Risolviamo l'**equazione di Poisson** su un dominio quadrato [0,1]×[0,1]:

```
-∇²u = f(x,y)    nel dominio
 u   = g(x,y)    sui bordi (condizioni di Dirichlet)
```

Con `f=0` diventa l'**equazione di Laplace**: la soluzione è la distribuzione di temperatura in equilibrio data la temperatura fissata sui bordi.

## Il metodo di Jacobi

Schema alle differenze finite con stencil a 5 punti:

```
         u[i-1][j]
             |
u[i][j-1] - u[i][j] - u[i][j+1]
             |
         u[i+1][j]

Aggiornamento: u_new[i][j] = 0.25 * (u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1])
                                      (per f=0)
```

Il metodo itera finché la variazione massima scende sotto una soglia ε.

## Parallelizzazione con MPI

### Decomposizione del dominio

La griglia globale (N×N) viene divisa in strisce orizzontali:

```
Processo 0: righe  0 ..  N/P-1  + ghost row sotto
Processo 1: righe  N/P .. 2N/P-1  + ghost row sopra e sotto
...
Processo P-1: ultime N/P righe   + ghost row sopra
```

### Algoritmo per iterazione

```
1. Halo exchange: scambia righe di bordo con i vicini
2. Aggiorna tutte le celle interne: u_new[i][j] = media(vicini)
3. Calcola variazione locale: max|u_new - u_old|
4. MPI_Allreduce(MPI_MAX) → variazione globale
5. Se variazione < ε → converge → esci
```

### Schema temporale

```
Iterazione k:
  ├── MPI_Sendrecv (halo exchange Nord-Sud)
  ├── Aggiorna stencil locale
  ├── MPI_Allreduce (max variazione)
  └── Controllo convergenza
```

---

## Esercizi

### Esercizio 1 — Jacobi 1D (strisce) (`jacobi_1d_strips.cpp`)
Versione semplificata: decomposizione in strisce orizzontali, solo scambio Nord-Sud.

### Esercizio 2 — Jacobi 2D completo (`jacobi_2d_full.cpp`)
Decomposizione 2D con topologia cartesiana, halo exchange sui 4 lati, condizioni al bordo non banali.

---

## Output Atteso

### jacobi_1d_strips (4 processi, griglia 64×64)
```
[Iter    0] variazione max = 1.00000e+00
[Iter  100] variazione max = 1.23456e-02
[Iter  500] variazione max = 2.45e-04
[Iter 1847] CONVERGENZA raggiunta! variazione = 9.87e-07 < eps = 1e-06
Tempo totale: 0.342 secondi
```
