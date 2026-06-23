# 00b — C++ vs Fortran per HPC: Benchmark e Analisi Comparativa

> Questo documento risponde alla domanda concreta: **vale la pena riscrivere codice Fortran in C++, e cosa ci guadagno/perdo in termini di performance?**

---

## 1. Il contesto: perché il confronto è complicato

C++ e Fortran sono entrambi linguaggi **compilati, staticamente tipizzati, con accesso diretto alla memoria**.
A parità di algoritmo e compilatore, la differenza di performance pura è spesso **inferiore al 5–10%** — e
dipende molto più dal **tipo di problema** che dal linguaggio in sé.

Tuttavia esistono differenze strutturali che in certi scenari diventano significative.

---

## 2. Il vantaggio strutturale di Fortran: il problema dell'aliasing

Questa è la ragione più citata in letteratura per cui Fortran può essere **più veloce di C/C++ per calcolo numerico**.

### Cosa è l'aliasing

In C/C++, due puntatori diversi possono puntare alla stessa area di memoria (alias).
Il compilatore **non può sapere** se questo accade, quindi deve essere conservativo:

```cpp
// C++: il compilatore NON può riordinare queste operazioni
// perché non sa se a e b puntano alla stessa memoria
void somma(double* a, double* b, double* c, int n) {
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];  // e se c == a? Il compilatore deve assumere che sì
}
```

In Fortran, il linguaggio **garantisce per standard** che gli argomenti di una subroutine
non si sovrappongono in memoria (no aliasing). Il compilatore può quindi:
- Riordinare le istruzioni liberamente
- Vettorizzare i loop automaticamente con SIMD
- Tenere valori nei registri più a lungo

```fortran
! Fortran: il compilatore SA che a, b, c non si sovrappongono
subroutine somma(a, b, c, n)
    real(8), intent(in)  :: a(n), b(n)
    real(8), intent(out) :: c(n)
    integer, intent(in)  :: n
    integer :: i
    do i = 1, n
        c(i) = a(i) + b(i)  ! il compilatore può vettorizzare liberamente
    end do
end subroutine
```

### La soluzione in C++: `__restrict__`

C++ offre la keyword `__restrict__` (non standard, ma supportata da GCC, Clang, Intel):

```cpp
void somma(double* __restrict__ a, double* __restrict__ b,
           double* __restrict__ c, int n) {
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];  // ora il compilatore può vettorizzare
}
```

Con `__restrict__` la performance di C++ raggiunge quella di Fortran per questi loop.
Ma in Fortran è il **default**, in C++ è opt-in (e richiede disciplina).

**Fonte:** <https://beza1e1.tuxen.de/articles/faster_than_C.html> — "Fortran semantics say that function arguments never alias [...] This is why Fortran is often faster than C. This is why numerical libraries are still written in Fortran."

---

## 3. Benchmark reali dalla letteratura (2019–2024)

### 3.1 — Array-heavy computation (fisica nucleare)

Un paper del 2024 (arXiv:2409.06837) confronta CMF (Fortran legacy) con CMF++ (C++
moderno con Eigen) sullo stesso algoritmo di equazioni di stato in 1D, 2D, 3D:

| Dimensione problema | Fortran runtime | C++ runtime | Speedup C++ |
|--------------------|-----------------|-------------|-------------|
| 1D (10k sistemi)   | ~10s            | ~0.3s       | **~33×**    |
| 2D (10M sistemi)   | ~10⁴s (est.)    | ~20s        | **~500×**   |
| 3D (100M sistemi)  | extrapolato     | ~200s       | **>1000×**  |

> ⚠️ Attenzione: in questo caso il vantaggio di C++ non viene dal linguaggio ma dalla **diversa complessità algoritmica** (O(n) vs O(n log n)) e dall'eliminazione di array temporanei grazie a Eigen. È un esempio di C++ moderno vs Fortran **legacy mal scritto**, non un confronto equo tra i linguaggi.

### 3.2 — Loop numerici semplici (Knapsack algorithm)

Un paper empirico (arXiv:1903.08936) confronta implementazioni identiche dello stesso algoritmo:

| Algoritmo | Fortran (media) | C++ (media) | Vincitore |
|-----------|-----------------|-------------|-----------|
| MTU1      | 59s             | 30s         | C++ ~2×   |
| MTU2      | ordini di grandezza diversi | — | dipende dal sorting |

### 3.3 — Parallel heat equation (LLM & HPC benchmark, 2024)

Il paper arXiv:2504.03665 confronta C++ e Fortran su AMD EPYC 7763 e Intel Xeon con scaling MPI:

```
Scaling da 1 a 64 core (heat equation, 10M nodi):
  C++:    scala bene con il numero di core
  Fortran: speedup fino a ~5 core, poi stagnazione

Scaling matrix mult (10k×10k, Intel Xeon):
  C++:    comportamento irregolare  
  Fortran: comportamento irregolare (stesso problema)
  
DGEMM (Arm A64FX):
  Fortran: ~0.02 GFLOP/s (codice generato, non ottimizzato)
  C++:     performance crescenti con dimensione
```

> Nota: questi benchmark usano codice **generato da LLM** non ottimizzato a mano —
> i risultati riflettono quanto sia facile/difficile scrivere codice performante nei due linguaggi.

### 3.4 — Librerie QCD (fisica delle particelle)

Il progetto Grid (arXiv:1512.03487) mostra che C++ con SIMD intrinsics espliciti
raggiunge **65% del picco teorico** su Intel Core i7, **superando Fortran** per calcoli
su reticoli di matrici SU(3):

```
Peak performance su L2 cache (single core, Intel i7-3615QM):
  C++ (Grid con SIMD intrinsics): ~24 GFLOP/s  (~65% del picco)
  Fortran equivalente:            ~18-20 GFLOP/s
```

---

## 4. Il vero benchmark: cosa conta davvero in HPC

### 4.1 — Fortran vince quando:

```
Scenario                              Vantaggio Fortran
─────────────────────────────────     ─────────────────
Loop densi su array N-dim             +5% → +20% tipico
Codice scritto 20+ anni fa            Il compilatore conosce bene i pattern
Nessun uso di puntatori              No aliasing → ottimizzazione aggressiva
Column-major access (matrici)         Layout nativo, cache-friendly di default
```

**Citazione letterale** (arXiv:1910.06415, BACKUS paper, 2019):
> "Fortran is almost best in terms of performances (secondary only to machine code), and in the realm of C and C++ in terms of programmer productivity."

**Citazione letterale** (arXiv:2301.02432, "Myths and Legends in HPC"):
> "It seems hard to replace Fortran with C or other languages and outperform it or even achieve the same baseline. This may be due to [...] the limited language features (e.g., no pointer aliasing) that enable more powerful optimizations."

### 4.2 — C++ vince quando:

```
Scenario                              Vantaggio C++
─────────────────────────────────     ─────────────────
Strutture dati complesse (grafi)      Fortran non ha equivalenti
Algoritmi con sorting/hashing         STL (sort, unordered_map) ottimizzati
Template metaprogramming              Eigen, lazy evaluation, zero-copy
Interoperabilità con GPU              CUDA, SYCL, HIP nativi
Codice nuovo da zero                  Compilatori moderni (Clang, NVHPC)
```

**Dati concreti:** Il paper TBPLaS 2.0 (arXiv:2509.26309) mostra che C++ con
eliminazione di array temporanei (lazy evaluation via Eigen) può essere
**"several times or even an order of magnitude faster than Fortran"** per algoritmi
che Fortran gestisce con array temporanei impliciti nelle funzioni.

### 4.3 — Dipende dal compilatore, non solo dal linguaggio

Dalle osservazioni del paper arXiv:2504.03665 (benchmark su architetture reali):

```
Architettura    | Linguaggio | Scaling MPI
────────────────|────────────|─────────────────────────
AMD EPYC 7763   | C++        | scala fino a 64 core ✓
AMD EPYC 7763   | Fortran    | scala fino a ~5 core ✗
Intel Xeon 8358 | C++        | comportamento anomalo ✗
Intel Xeon 8358 | Fortran    | comportamento anomalo ✗
Arm A64FX       | C++        | DGEMM crescente ✓
Arm A64FX       | Fortran    | DGEMM ~0.02 GFLOP/s ✗ (codice LLM)
```

> Il risultato cambia **radicalmente** con l'architettura. Non esiste un vincitore universale.

---

## 5. Confronto pratico: MPI in C++ vs MPI in Fortran

Dal punto di vista MPI puro, le due API sono **equivalenti in performance** — MPI è
una libreria C, i binding Fortran e C++ sono wrapper con overhead trascurabile.

```
Operazione MPI         | Overhead C++ vs Fortran
───────────────────────|─────────────────────────
MPI_Send/Recv          | identico (stessa libreria)
MPI_Bcast              | identico
MPI_Allreduce          | identico
MPI_Cart_create        | identico
Halo exchange          | identico
```

La differenza di performance in codice MPI viene dall'algoritmo circostante,
non dalle chiamate MPI. Per esempio, il layout della memoria del sotto-dominio
(row-major C++ vs column-major Fortran) può influenzare i cache miss nell'halo exchange.

### Il problema del layout in Jacobi 2D

```cpp
// C++: array row-major. u[i][j] → riga i, colonna j
// Scorrere per colonne è cache-UNFRIENDLY:
for (int j = 0; j < N; j++)
    for (int i = 0; i < N; i++)   // ← MALE: salta in memoria
        u[i*N + j] = ...;

// Scorrere per righe è cache-FRIENDLY:
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)   // ← BENE: accesso sequenziale
        u[i*N + j] = ...;
```

```fortran
! Fortran: array column-major. A(i,j) → colonna j, riga i
! Scorrere per righe è cache-UNFRIENDLY in Fortran:
do j = 1, N
    do i = 1, N   ! ← BENE in Fortran: accesso colonna per colonna
        u(i,j) = ...
    end do
end do
```

> Se porti codice Fortran in C++ senza invertire i loop, perdi performance di cache.
> Questo è **il bug più comune** nelle traduzioni Fortran→C++.

---

## 6. Tabella riassuntiva finale

| Criterio | Fortran | C++ |
|----------|---------|-----|
| **Performance loop numerici** | ★★★★★ (no aliasing) | ★★★★☆ (con `__restrict__`) |
| **Performance strutture dati** | ★★☆☆☆ | ★★★★★ (STL, template) |
| **GPU/acceleratori** | ★★★☆☆ (OpenACC, coarray) | ★★★★★ (CUDA, SYCL nativi) |
| **Interop con librerie moderne** | ★★☆☆☆ (wrapper C) | ★★★★★ (nativo) |
| **Leggibilità codice scientifico** | ★★★★★ (array nativi) | ★★★☆☆ (verboso) |
| **Tooling moderno** (debugger, sanitizer) | ★★★☆☆ | ★★★★★ |
| **Ecosistema librerie** | ★★☆☆☆ (BLAS/LAPACK) | ★★★★★ (Eigen, Boost, ...) |
| **Diffusione in codici legacy HPC** | ★★★★★ (80% su ARCHER2) | ★★★☆☆ |
| **Tendenza futura** | ↔ stabile nel calcolo | ↑ crescente in HPC |

### In una frase

> **Fortran è imbattibile per loop densi su array regolari scritti da esperti.**
> **C++ vince su tutto il resto, e con `__restrict__` + compilatore moderno pareggia anche i loop.**

---

## 7. Cosa significa per questo tutorial

Il codice che hai scritto in C++ in questo repository è **equivalente in performance**
al codice Fortran che hai già studiato, con queste avvertenze:

1. **Loop interni del Jacobi**: assicurati che il loop esterno sia sulle righe (`i`) e
   quello interno sulle colonne (`j`) — esattamente come nel codice di `jacobi_1d_strips.cpp`.
   
2. **Halo exchange**: `MPI_Sendrecv` in C++ e Fortran chiamano **la stessa funzione C** sottostante — nessuna differenza.

3. **Se vuoi Fortran-level performance garantita** nei tuoi loop Jacobi, aggiungi
   `-fno-strict-aliasing` a GCC oppure usa `__restrict__` sui puntatori.

---

## Riferimenti

- arXiv:2409.06837 — Phase Stability in Chiral Mean-Field Model (C++ vs Fortran benchmark)
- arXiv:1903.08936 — Knapsack algorithm, C++ vs Fortran empirical comparison
- arXiv:2504.03665 — LLM & HPC: Benchmarking on AMD/Intel/ARM (2024)
- arXiv:1512.03487 — Grid: C++ QCD library, SIMD performance vs Fortran
- arXiv:1910.06415 — BACKUS: Modern Fortran performance positioning
- arXiv:2301.02432 — Myths and Legends in HPC (aliasing analysis)
- arXiv:2509.26309 — TBPLaS 2.0: C++ lazy evaluation vs Fortran temporary arrays
- arXiv:2308.13274 — Fortran HLS on FPGAs (C++ vs Fortran HLS comparison)
- <https://beza1e1.tuxen.de/articles/faster_than_C.html> — Aliasing semantics explained