# 03b — Solutore di Jacobi in MPI

> Guida di riferimento sul metodo iterativo di Jacobi applicato in ambiente a memoria distribuita. Presuppone la lettura delle guide precedenti (01a/01b — comunicazione point-to-point, 03 — comunicazione collettiva): qui `MPI_Allreduce` e `MPI_Allgather` vengono riutilizzate senza essere ridefinite da zero, mentre `MPI_Sendrecv` viene introdotta ex novo, essendo la prima occorrenza in questa serie di guide.
>
> Questo modulo presenta due applicazioni del metodo di Jacobi strutturalmente distinte: la prima applica il metodo alla soluzione numerica di un'equazione differenziale alle derivate parziali (PDE) tramite discretizzazione su griglia, con parallelizzazione **spaziale** del dominio; la seconda applica lo stesso metodo a un sistema lineare denso di piccola dimensione, con parallelizzazione **algebrica** a livello delle singole incognite. Il confronto tra le due strategie di parallelizzazione è l'obiettivo didattico-ingegneristico centrale di questa guida.

---

## Indice

1. [Il metodo di Jacobi: fondamenti matematici generali](#1-il-metodo-di-jacobi-fondamenti-matematici-generali)
2. [Esercizio 1 — Diffusione del calore su griglia (Jacobi 1D a strisce)](#2-esercizio-1--diffusione-del-calore-su-griglia-jacobi-1d-a-strisce)
   - 2.1 [Il problema fisico e l'equazione di Laplace](#21-il-problema-fisico-e-lequazione-di-laplace)
   - 2.2 [Discretizzazione alle differenze finite: derivazione dello stencil](#22-discretizzazione-alle-differenze-finite-derivazione-dello-stencil)
   - 2.3 [Condizioni al contorno](#23-condizioni-al-contorno)
   - 2.4 [Strategia di parallelizzazione: decomposizione a strisce](#24-strategia-di-parallelizzazione-decomposizione-a-strisce)
   - 2.5 [Ghost rows e halo exchange con MPI_Sendrecv](#25-ghost-rows-e-halo-exchange-con-mpi_sendrecv)
   - 2.6 [Criterio di convergenza globale](#26-criterio-di-convergenza-globale)
   - 2.7 [Algoritmo completo per iterazione](#27-algoritmo-completo-per-iterazione)
   - 2.8 [Compilazione, esecuzione e output atteso](#28-compilazione-esecuzione-e-output-atteso)
3. [Esercizio 2 — Jacobi su sistema lineare 4×4](#3-esercizio-2--jacobi-su-sistema-lineare-4×4)
   - 3.1 [Formulazione algebrica e condizione di convergenza](#31-formulazione-algebrica-e-condizione-di-convergenza)
   - 3.2 [Strategia di parallelizzazione: un'incognita per processo](#32-strategia-di-parallelizzazione-unincognita-per-processo)
   - 3.3 [Pattern di comunicazione: perché Allgather e non halo exchange](#33-pattern-di-comunicazione-perché-allgather-e-non-halo-exchange)
   - 3.4 [Algoritmo completo per iterazione](#34-algoritmo-completo-per-iterazione)
   - 3.5 [Compilazione, esecuzione e output atteso](#35-compilazione-esecuzione-e-output-atteso)
4. [Confronto tra le due strategie di parallelizzazione](#4-confronto-tra-le-due-strategie-di-parallelizzazione)
5. [Errori comuni e come evitarli](#5-errori-comuni-e-come-evitarli)

---

## 1. Il metodo di Jacobi: fondamenti matematici generali

Il metodo di Jacobi è un metodo iterativo per la soluzione di sistemi lineari `Ax = b`, appartenente alla famiglia dei metodi basati sullo **splitting** della matrice dei coefficienti. Si scompone `A` come:

```
A = D - L - U
```

dove `D` è la matrice diagonale contenente gli elementi diagonali di `A`, `-L` è la parte strettamente triangolare inferiore e `-U` la parte strettamente triangolare superiore (con segno tale che `L` e `U` abbiano diagonale nulla e contengano gli opposti degli elementi fuori diagonale). Il sistema `Ax = b` si riscrive quindi come:

```
(D - L - U)x = b   ⟹   Dx = (L + U)x + b   ⟹   x = D⁻¹(L + U)x + D⁻¹b
```

Questa riscrittura suggerisce naturalmente uno schema iterativo a punto fisso:

```
x^(k+1) = D⁻¹(L + U) x^(k) + D⁻¹b
```

dove `x^(k)` indica il vettore delle incognite alla `k`-esima iterazione. La matrice `T = D⁻¹(L + U)` è detta **matrice di iterazione di Jacobi**. Componente per componente, l'aggiornamento si esplicita come:

```
x_i^(k+1) = ( b_i - Σ_{j≠i} a_ij · x_j^(k) ) / a_ii
```

ossia: l'incognita `i`-esima viene isolata usando l'equazione `i`-esima del sistema originale, sostituendo a tutte le altre incognite i valori dell'iterazione **precedente** (non quelli già aggiornati nell'iterazione corrente — questa è la distinzione fondamentale rispetto al metodo di Gauss-Seidel, che invece riutilizza immediatamente i valori più recenti disponibili, introducendo però una dipendenza sequenziale tra le componenti che rende il metodo intrinsecamente meno parallelizzabile).

**Condizione di convergenza.** Il metodo converge, per qualunque scelta del vettore iniziale `x^(0)`, se e solo se il raggio spettrale della matrice di iterazione `T = D⁻¹(L + U)` è strettamente minore di 1:

```
ρ(T) = max_i |λ_i(T)| < 1
```

dove `λ_i(T)` sono gli autovalori di `T`. Verificare direttamente questa condizione richiede il calcolo degli autovalori, operazione costosa; esiste tuttavia una condizione **sufficiente** (ma non necessaria) più semplice da verificare, quella di **dominanza diagonale stretta per righe**:

```
|a_ii| > Σ_{j≠i} |a_ij|    per ogni riga i
```

Se questa condizione è soddisfatta per ogni riga della matrice, si dimostra che `ρ(T) < 1` (in norma infinito, `‖T‖_∞ < 1`, il che implica `ρ(T) ≤ ‖T‖_∞ < 1` per la relazione generale tra raggio spettrale e una qualsiasi norma matriciale indotta), e quindi il metodo converge indipendentemente dal punto di partenza. Entrambi gli esercizi di questo modulo sfruttano, in forme diverse, questa proprietà: il primo la eredita implicitamente dalla struttura del laplaciano discreto (stencil a 5 punti, coefficiente diagonale pari alla somma dei pesi dei vicini in valore assoluto, con uguaglianza — non stretta dominanza — compensata dalle condizioni al contorno di Dirichlet che fissano valori noti sul bordo), il secondo la verifica esplicitamente sulla matrice 4×4 del sistema (sezione 3.1).

**Criterio d'arresto pratico.** Poiché la convergenza teorica è asintotica (il metodo raggiunge la soluzione esatta solo per `k → ∞`), in pratica l'iterazione si interrompe quando una misura dello scarto tra iterazioni successive scende sotto una soglia di tolleranza `ε` fissata a priori:

```
‖x^(k+1) - x^(k)‖ < ε
```

Entrambi gli esercizi di questo modulo usano la norma infinito (massimo scarto puntuale in valore assoluto) come criterio, per ragioni sia di semplicità implementativa sia di aderenza diretta al significato fisico/numerico della grandezza monitorata.

## 2. Esercizio 1 — Diffusione del calore su griglia (Jacobi 1D a strisce)

### 2.1 Il problema fisico e l'equazione di Laplace

Si risolve l'**equazione di Laplace** su un dominio quadrato `[0,1]×[0,1]`:

```
-∇²u = 0    all'interno del dominio
 u   = g    sul bordo (condizioni di Dirichlet)
```

dove `∇²u = ∂²u/∂x² + ∂²u/∂y²` è l'operatore di Laplace (laplaciano) applicato al campo scalare `u(x,y)`. Questa equazione descrive la distribuzione di temperatura in **regime stazionario** (nessuna dipendenza dal tempo: `∂u/∂t = 0`, condizione che si ottiene come caso limite dell'equazione del calore parabolica `∂u/∂t = α∇²u` quando il transitorio si è esaurito) su una lamina piana, dati valori di temperatura fissati sul bordo del dominio (condizioni al contorno di **tipo Dirichlet**, in cui è il valore della funzione, non la sua derivata normale, ad essere specificato sul bordo).

Le condizioni al contorno usate in questo esercizio sono:

```
Bordo superiore (riga 0):     u = 0.0  (freddo)
Bordo sinistro  (colonna 0):  u = 0.0
Bordo destro    (colonna N-1): u = 0.0
Bordo inferiore (riga N-1):   u = 1.0  (caldo)
```

Fisicamente, questo corrisponde a una lamina quadrata con tre lati mantenuti a temperatura 0 e un lato mantenuto a temperatura 1: la soluzione stazionaria `u(x,y)` rappresenta la temperatura di equilibrio in ogni punto interno del dominio, risultato della diffusione del calore dal bordo caldo verso il resto della lamina.

### 2.2 Discretizzazione alle differenze finite: derivazione dello stencil

Per risolvere numericamente l'equazione, il dominio continuo viene discretizzato su una griglia regolare N×N di punti, con passo di griglia `h = 1/(N-1)` in entrambe le direzioni. Il valore `u(x,y)` viene approssimato dai valori discreti `u[i][j] ≈ u(i·h, j·h)` nei nodi della griglia.

Le derivate parziali seconde vengono approssimate tramite lo schema alle **differenze finite centrate** del secondo ordine:

```
∂²u/∂x² ≈ ( u[i-1][j] - 2·u[i][j] + u[i+1][j] ) / h²
∂²u/∂y² ≈ ( u[i][j-1] - 2·u[i][j] + u[i][j+1] ) / h²
```

Questa approssimazione deriva dallo sviluppo in serie di Taylor di `u` attorno al punto `(i,j)` nelle due direzioni, troncato al secondo ordine: sommando gli sviluppi di `u[i+1][j]` e `u[i-1][j]` (o analogamente per la direzione `j`), i termini di ordine dispari si cancellano per simmetria, lasciando un errore di troncamento locale `O(h²)`.

Sostituendo entrambe le approssimazioni nell'equazione `-∇²u = 0`, ovvero `∂²u/∂x² + ∂²u/∂y² = 0`, si ottiene:

```
( u[i-1][j] - 2u[i][j] + u[i+1][j] ) / h² + ( u[i][j-1] - 2u[i][j] + u[i][j+1] ) / h² = 0
```

Moltiplicando per `h²` e raccogliendo i termini in `u[i][j]` (che compaiono con coefficiente `-4`):

```
u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1] - 4·u[i][j] = 0
```

da cui, isolando `u[i][j]`:

```
u[i][j] = 0.25 · ( u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1] )
```

Questa è esattamente la relazione di punto fisso che il metodo di Jacobi itera, applicata **localmente** ad ogni nodo interno della griglia: il valore in ogni punto, a convergenza, è la media aritmetica dei quattro vicini cardinali (nord, sud, est, ovest). Il pattern di accesso a 5 punti (il nodo stesso più i 4 vicini) è noto come **stencil a 5 punti** (5-point stencil):

```
             u[i-1][j]
                 |
u[i][j-1] --- u[i][j] --- u[i][j+1]
                 |
             u[i+1][j]

Aggiornamento: u_new[i][j] = 0.25 · (u[i-1][j] + u[i+1][j] + u[i][j-1] + u[i][j+1])
```

Da notare la corrispondenza diretta con il caso generale della sezione 1: la matrice `A` implicita in questa formulazione (se si "srotolasse" la griglia bidimensionale in un vettore monodimensionale di incognite, come si farebbe per un solutore diretto) avrebbe `-4` sulla diagonale e `+1` nelle quattro posizioni corrispondenti ai vicini dello stencil — una matrice sparsa, pentadiagonale a blocchi, con dominanza diagonale non stretta ma "quasi ovunque" resa effettivamente stretta dai nodi adiacenti al bordo, dove alcuni dei quattro vicini sono in realtà valori di bordo noti (costanti), non incognite.

Il metodo itera l'aggiornamento su tutti i nodi **interni** della griglia (i nodi di bordo restano fissati alle condizioni di Dirichlet e non vengono mai aggiornati) fino a quando la variazione puntuale massima tra due iterazioni successive scende sotto una soglia `ε`:

```
max_{i,j} |u_new[i][j] - u[i][j]| < ε
```

### 2.3 Condizioni al contorno

Le condizioni al contorno sono imposte fissando i valori di `u` sulle righe/colonne di bordo del dominio globale e **non aggiornandoli mai** durante l'iterazione: riga 0 (bordo superiore) e colonna 0/colonna N-1 (bordi laterali) restano a `0.0`; l'ultima riga (bordo inferiore) resta a `1.0`. Solo i nodi strettamente interni alla griglia (`1 ≤ i ≤ N-2`, `1 ≤ j ≤ N-2` nella numerazione globale) sono soggetti all'aggiornamento dello stencil descritto sopra.

### 2.4 Strategia di parallelizzazione: decomposizione a strisce

La griglia globale N×N viene partizionata in **strisce orizzontali contigue di righe**, una per processo, secondo uno schema di **decomposizione del dominio** (domain decomposition) 1D:

```
Processo 0:    righe  0      ..  N/P-1     + ghost row sud
Processo 1:    righe  N/P    ..  2N/P-1    + ghost row nord e sud
...
Processo P-1:  righe (P-1)N/P .. N-1       + ghost row nord
```

Ogni processo possiede in memoria locale le proprie righe assegnate **più** una o due righe aggiuntive dette **ghost row** (righe fantasma), che replicano localmente i valori delle righe di confine dei processi vicini. Questa replica è necessaria perché lo stencil a 5 punti, applicato a un nodo sul bordo superiore o inferiore della striscia locale di un processo, richiede il valore del vicino nord o sud, che fisicamente risiede nella memoria di un **altro** processo. Senza le ghost row, ogni processo dovrebbe effettuare una comunicazione point-to-point per ciascun accesso al confine ad ogni singolo aggiornamento di nodo, con un overhead di comunicazione proibitivo; con le ghost row, la comunicazione viene invece **batchata**: una sola coppia di scambi di riga per vicino, per ogni iterazione completa dello stencil su tutti i nodi interni della striscia locale.

Il processo di rank `r` (con `0 < r < P-1`, quindi non ai bordi della decomposizione) ha come vicino nord il processo `r-1` e come vicino sud il processo `r+1`. I processi di rank `0` e `P-1` hanno un solo vicino (rispettivamente solo sud e solo nord), dato che le loro righe di confine opposte coincidono con il bordo fisico del dominio globale (condizione di Dirichlet nota, non ghost row da altri processi).

### 2.5 Ghost rows e halo exchange con MPI_Sendrecv

Ad ogni iterazione, prima di poter applicare lo stencil sui nodi di confine della propria striscia locale, ogni processo deve aggiornare le proprie ghost row con i valori correnti delle righe di confine dei vicini. Questa operazione è nota come **halo exchange** (scambio dell'alone/margine).

L'implementazione tipica usa `MPI_Sendrecv`, una primitiva **bloccante** che combina in un'unica chiamata un invio e una ricezione simultanei, evitando esplicitamente il rischio di deadlock descritto nella guida 01a (sezione 6) per pattern di invio/ricezione simmetrici tra processi vicini:

```cpp
int MPI_Sendrecv(
    const void*  sendbuf,     // buffer da inviare al vicino
    int          sendcount,   // numero di elementi da inviare
    MPI_Datatype sendtype,
    int          dest,        // rank del processo a cui inviare
    int          sendtag,     // tag del messaggio in uscita
    void*        recvbuf,     // buffer in cui ricevere dal vicino (DISTINTO
                               // da sendbuf: MPI_Sendrecv non è in-place)
    int          recvcount,   // capacità massima del buffer di ricezione
    MPI_Datatype recvtype,
    int          source,      // rank del processo da cui ricevere
    int          recvtag,     // tag atteso del messaggio in ingresso
    MPI_Comm     comm,
    MPI_Status*  status
);
```

Il punto cruciale di `MPI_Sendrecv` è che l'implementazione MPI **gestisce internamente** l'ordinamento delle operazioni di invio e ricezione in modo da evitare il deadlock che si presenterebbe scrivendo manualmente due `MPI_Send` bloccanti consecutivi tra processi che si scambiano dati simmetricamente (esattamente lo scenario discusso nella guida 01a, sezione 6.1). Semanticamente, `MPI_Sendrecv` è equivalente a eseguire una `MPI_Isend` e una `MPI_Irecv` seguite da una `MPI_Waitall` su entrambe (guida 01b), ma senza dover gestire esplicitamente gli handle `MPI_Request`: la gestione dell'asincronia interna è delegata completamente all'implementazione.

Nel contesto dell'halo exchange, ogni processo esegue **due** chiamate `MPI_Sendrecv` per iterazione (una per il vicino nord, una per il vicino sud), ciascuna delle quali invia la propria riga di confine e riceve contestualmente la ghost row corrispondente:

```cpp
// Scambio con il vicino sud: invio la mia ultima riga locale,
// ricevo la sua prima riga locale nella mia ghost row sud
MPI_Sendrecv(
    &u[last_local_row][0], N, MPI_DOUBLE, south_neighbor, TAG_S,
    &u[ghost_row_south][0], N, MPI_DOUBLE, south_neighbor, TAG_N,
    MPI_COMM_WORLD, MPI_STATUS_IGNORE
);

// Scambio con il vicino nord: invio la mia prima riga locale,
// ricevo la sua ultima riga locale nella mia ghost row nord
MPI_Sendrecv(
    &u[first_local_row][0], N, MPI_DOUBLE, north_neighbor, TAG_N,
    &u[ghost_row_north][0], N, MPI_DOUBLE, north_neighbor, TAG_S,
    MPI_COMM_WORLD, MPI_STATUS_IGNORE
);
```

I processi ai bordi della decomposizione (rank 0 e rank P-1), privi di un vicino su un lato, tipicamente gestiscono l'assenza del vicino con una condizionale (`if (rank > 0) ...` / `if (rank < size-1) ...`) attorno alla rispettiva `MPI_Sendrecv`, oppure — soluzione più elegante e meno soggetta a errori — utilizzando `MPI_PROC_NULL` come valore di `dest`/`source` per il lato mancante: le chiamate MPI verso/da `MPI_PROC_NULL` sono no-op garantite dallo standard (ritornano immediatamente senza effetto), eliminando la necessità di logica condizionale esplicita nel codice di comunicazione.

### 2.6 Criterio di convergenza globale

Ogni processo, dopo aver applicato lo stencil di Jacobi su tutti i nodi interni della propria striscia locale, calcola la propria variazione massima **locale**:

```cpp
double local_max_change = 0.0;
for (/* ogni nodo interno locale i,j */) {
    double diff = std::fabs(u_new[i][j] - u[i][j]);
    local_max_change = std::max(local_max_change, diff);
}
```

Questo valore locale, tuttavia, non è sufficiente a decidere la convergenza globale dell'algoritmo: il criterio d'arresto richiede il massimo su **tutti** i nodi del dominio, distribuiti su tutti i processi. Si utilizza quindi `MPI_Allreduce` con operatore `MPI_MAX` (guida 03, sezione 6) per aggregare i massimi locali in un unico massimo globale, reso disponibile a **tutti** i processi (necessario perché ogni processo deve poter decidere autonomamente, con lo stesso esito, se uscire dal ciclo di iterazione — un `MPI_Reduce` con notifica solo al root richiederebbe una successiva `MPI_Bcast` per comunicare la decisione di arresto a tutti gli altri processi):

```cpp
double global_max_change;
MPI_Allreduce(&local_max_change, &global_max_change, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

if (global_max_change < EPS) break;  // condizione di uscita, identica su ogni processo
```

### 2.7 Algoritmo completo per iterazione

```
Iterazione k:
  ├── MPI_Sendrecv × 2   (scambio halo nord-sud con i processi vicini)
  ├── Aggiornamento stencil di Jacobi (calcolo locale, nodi interni)
  ├── Calcolo local_max_change (riduzione locale, nessuna comunicazione)
  ├── MPI_Allreduce (MPI_MAX)  (riduzione globale, criterio di convergenza)
  └── std::swap(u, u_new)     (scambio dei puntatori/buffer, nessuna copia)
```

Da notare l'uso di `std::swap(u, u_new)` al termine di ogni iterazione, anziché una copia elemento per elemento da `u_new` a `u`: poiché lo stencil di Jacobi (a differenza di Gauss-Seidel) richiede esplicitamente che tutti gli aggiornamenti di un'iterazione usino solo valori "congelati" dell'iterazione precedente, è necessario mantenere due buffer distinti (`u` e `u_new`) durante il calcolo dello stencil, per evitare che un nodo aggiornato per primo influenzi il calcolo di un nodo aggiornato successivamente nella stessa iterazione. Scambiare i puntatori ai due buffer, invece di copiarne il contenuto, è un'ottimizzazione elementare ma importante dal punto di vista prestazionale: riduce il costo per iterazione da `O(righe_locali × N)` operazioni di copia a `O(1)` (uno scambio di due puntatori).

### 2.8 Compilazione, esecuzione e output atteso

```bash
mpicxx -O2 -Wall -o jacobi1d jacobi_1d_strips.cpp
mpirun -np 4 ./jacobi1d
```

Output atteso:

```text
[Iter  100] max change = 1.2346e-02
[Iter  200] max change = 6.2134e-03
...
✓ CONVERGENCE reached in 1847 iterations!
  Final change:      9.87e-07
  Total time:        0.342 seconds
  Processes used:    4
```

Il valore `max change`, stampato periodicamente (tipicamente ogni 100 iterazioni, per non saturare l'output con una riga per iterazione), è il risultato della `MPI_Allreduce` di sezione 2.6: essendo identico su tutti i processi al termine della riduzione, la stampa periodica viene tipicamente condizionata a `if (rank == 0)` per evitare P righe identiche ridondanti per ogni checkpoint. La monotonia decrescente della sequenza (`1.2346e-02` → `6.2134e-03` → ...) è attesa e coerente con la natura del metodo di Jacobi per questo problema: essendo `ρ(T) < 1` (sezione 1), l'errore decresce geometricamente ad ogni iterazione con fattore asintotico pari al raggio spettrale della matrice di iterazione. Il numero di iterazioni necessarie per raggiungere la soglia `ε` dipende sia dalla dimensione della griglia `N` (raffinare la griglia peggiora la velocità di convergenza del metodo di Jacobi applicato al laplaciano discreto, poiché `ρ(T) → 1` per `h → 0`) sia dal valore di `ε` scelto.

## 3. Esercizio 2 — Jacobi su sistema lineare 4×4

### 3.1 Formulazione algebrica e condizione di convergenza

Questo esercizio applica il metodo di Jacobi descritto in astratto nella sezione 1 a un sistema lineare denso, di piccola dimensione fissata (4 equazioni, 4 incognite), risolto **esattamente** (a meno della tolleranza di convergenza) piuttosto che tramite discretizzazione di un problema continuo. Il sistema è:

```
 10 x₀ -  x₁ + 2 x₂          =   6
- x₀  + 11 x₁ -  x₂ + 3 x₃   =  25
 2 x₀ -  x₁ + 10 x₂ -  x₃    = -11
        3 x₁ -  x₂ + 8 x₃    =  15
```

Verifica della dominanza diagonale stretta per righe (condizione sufficiente di convergenza, sezione 1), riga per riga:

```
Riga 0: |10| > |-1| + |2|           →  10 > 3    ✓
Riga 1: |11| > |-1| + |-1| + |3|    →  11 > 5    ✓
Riga 2: |10| > |2| + |-1| + |-1|    →  10 > 4    ✓
Riga 3: |8|  > |3| + |-1|           →   8 > 4    ✓
```

Tutte le righe soddisfano la condizione, garantendo `ρ(D⁻¹(L+U)) < 1` e quindi convergenza del metodo indipendentemente dal vettore iniziale scelto (tipicamente `x^(0) = 0` in assenza di una stima migliore).

Isolando ciascuna incognita sulla propria equazione (equivalente a esplicitare `x = D⁻¹(L+U)x + D⁻¹b` componente per componente, sezione 1):

```
x₀^(k+1) = ( 6  + x₁^(k) - 2x₂^(k)          ) / 10
x₁^(k+1) = ( 25 + x₀^(k) + x₂^(k) - 3x₃^(k) ) / 11
x₂^(k+1) = (-11 - 2x₀^(k) + x₁^(k) + x₃^(k) ) / 10
x₃^(k+1) = ( 15 - 3x₁^(k) + x₂^(k)          ) / 8
```

Si noti, confrontando con la sezione 2.2, che la struttura è identica in natura (ogni incognita è funzione lineare delle altre, pesata dai coefficienti fuori diagonale e normalizzata per il coefficiente diagonale), ma qui la matrice è **densa** (quasi tutti i coefficienti fuori diagonale sono non nulli) anziché sparsa a struttura regolare come nel caso dello stencil a 5 punti: ogni incognita dipende, in generale, da **tutte** le altre, non solo da un piccolo sottoinsieme di "vicini" topologici.

### 3.2 Strategia di parallelizzazione: un'incognita per processo

A differenza dell'esercizio 1, dove la parallelizzazione è **spaziale** (ogni processo possiede una porzione contigua del dominio fisico), qui la parallelizzazione è **algebrica**: ogni processo è responsabile del calcolo di una singola incognita del sistema, indipendentemente da qualunque nozione di prossimità spaziale (che, in un sistema lineare denso e astratto, non è nemmeno definita):

```
Processo 0 → calcola x₀_new
Processo 1 → calcola x₁_new
Processo 2 → calcola x₂_new
Processo 3 → calcola x₃_new
```

Questa corrispondenza 1:1 tra incognite e processi impone un vincolo rigido sul numero di processi con cui l'eseguibile può essere lanciato: **esattamente 4**, pari alla dimensione del sistema. A differenza dell'esercizio 1, dove la decomposizione a strisce si adatta naturalmente a un numero arbitrario di processi P (a patto che `P ≤ N`, e idealmente `N` divisibile per `P` per bilanciare il carico), qui il grado di parallelismo è strutturalmente limitato dalla dimensione del problema stesso: non ha senso lanciare questo eseguibile con un numero di processi diverso da 4, né maggiore (non ci sarebbero incognite aggiuntive da assegnare) né minore (mancherebbero processi per coprire tutte le incognite, nell'assunzione implementativa 1 processo = 1 incognita usata qui).

Ogni processo determina quale equazione/incognita gli compete tipicamente tramite uno `switch(rank)` o una struttura condizionale equivalente, dato che i coefficienti di ciascuna equazione sono strutturalmente diversi (non esiste una formula chiusa uniforme parametrizzata dal solo rank, a differenza dell'esercizio 1 dove lo stesso identico stencil si applica a ogni processo, cambiando solo l'intervallo di righe):

```cpp
double x_new;
switch (rank) {
    case 0: x_new = ( 6.0  + x[1] - 2.0*x[2]          ) / 10.0; break;
    case 1: x_new = ( 25.0 + x[0] + x[2] - 3.0*x[3]    ) / 11.0; break;
    case 2: x_new = (-11.0 - 2.0*x[0] + x[1] + x[3]    ) / 10.0; break;
    case 3: x_new = ( 15.0 - 3.0*x[1] + x[2]           ) / 8.0;  break;
}
```

### 3.3 Pattern di comunicazione: perché Allgather e non halo exchange

Nell'esercizio 1, ogni processo necessita, per aggiornare i propri nodi, esclusivamente dei valori posseduti dai **due** processi topologicamente adiacenti (nord e sud): il pattern di comunicazione è **locale**, e il volume di dati scambiato per processo per iterazione è costante rispetto al numero totale di processi P (dipende solo dalla larghezza N della griglia, non da P). Questo è il motivo per cui l'halo exchange P2P (`MPI_Sendrecv`) è la scelta appropriata: non ha senso, né sarebbe efficiente, forzare una comunicazione collettiva quando ogni processo interagisce strutturalmente solo con un piccolo sottoinsieme fisso degli altri.

Nell'esercizio 2, invece, la formula di aggiornamento di **ciascuna** incognita (sezione 3.1) coinvolge, in generale, **tutte** le altre incognite del sistema (matrice densa, sezione 3.1): il processo 0, per calcolare `x₀_new`, necessita dei valori correnti di `x₁` e `x₂` (posseduti da altri processi); il processo 1 necessita di `x₀`, `x₂` e `x₃`; e così via. Il pattern di dipendenza dati è quindi **completo** (all-to-all): ogni processo deve rendere disponibile il proprio valore aggiornato a **tutti** gli altri processi prima dell'iterazione successiva, e viceversa deve ricevere il valore aggiornato da tutti gli altri.

Implementare questo pattern con comunicazioni P2P esplicite richiederebbe, per ogni processo, l'invio del proprio valore a ciascuno degli altri P-1 processi (e la ricezione simmetrica), per un totale di P·(P-1) messaggi P2P nel communicator — esattamente il pattern generale che la primitiva collettiva `MPI_Alltoall` (guida 03, sezione 7) è progettata per gestire in modo efficiente. In questo caso specifico, tuttavia, ogni processo invia lo **stesso** valore scalare (`x_new` del processo stesso) a tutti gli altri, anziché valori distinti per ciascun destinatario come nel caso generale di `MPI_Alltoall`: questa è esattamente la semantica di `MPI_Allgather` (guida 03, sezione 5), che risulta quindi la primitiva collettiva corretta e più efficiente per questo pattern, preferibile sia a un `MPI_Alltoall` (semanticamente più generale del necessario) sia a una sequenza di comunicazioni P2P scritte manualmente:

```cpp
double x_new;   // valore calcolato localmente da QUESTO processo (sezione 3.2)
double x[4];    // vettore completo delle incognite, replicato su OGNI processo

MPI_Allgather(&x_new, 1, MPI_DOUBLE, x, 1, MPI_DOUBLE, MPI_COMM_WORLD);
// Dopo questa chiamata, x[i] su OGNI processo contiene il valore x_new
// calcolato dal processo di rank i, per i = 0..3
```

Al termine della `MPI_Allgather`, ogni processo possiede una copia locale identica e completa del vettore `x[]` aggiornato, necessaria per calcolare la propria incognita alla successiva iterazione (che, come visto in sezione 3.1, dipende in generale da tutte le componenti del vettore).

### 3.4 Algoritmo completo per iterazione

```
Iterazione k:
  ├── Ogni processo calcola la propria x_new (switch(rank), sezione 3.2)
  ├── Calcolo local diff = |x_new - x_old|            (nessuna comunicazione)
  ├── MPI_Allreduce (MPI_MAX)  → max_diff globale       (criterio di convergenza)
  ├── MPI_Allgather            → distribuzione di x_new a tutti i processi
  └── Verifica: se max_diff < ε, uscita dal ciclo (identica su ogni processo)
```

Da notare che, a differenza dell'esercizio 1 dove `MPI_Allreduce` è l'unica collettiva coinvolta, qui sono necessarie **due** collettive distinte per iterazione: `MPI_Allreduce` per il criterio di arresto globale (esattamente lo stesso ruolo della sezione 2.6, applicato qui a un solo scalare per processo anziché a un massimo su una striscia di griglia) e `MPI_Allgather` per la ridistribuzione del vettore delle incognite (che nell'esercizio 1 non ha un analogo diretto: lì la "ridistribuzione" necessaria è solo verso i due vicini topologici, coperta dall'halo exchange P2P, non da una collettiva).

```cpp
double diff = std::fabs(x_new - x[rank]);
double max_diff;
MPI_Allreduce(&diff, &max_diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

MPI_Allgather(&x_new, 1, MPI_DOUBLE, x, 1, MPI_DOUBLE, MPI_COMM_WORLD);

if (max_diff < EPS) break;
```

Va osservato l'ordine relativo delle due chiamate: il calcolo di `diff` avviene confrontando `x_new` (valore locale appena calcolato da questo processo) con `x[rank]` (valore della stessa incognita all'iterazione precedente, ancora presente nel vettore `x[]` prima che la `MPI_Allgather` lo sovrascriva) — è quindi necessario calcolare `diff` **prima** della `MPI_Allgather` che aggiorna `x[]`, altrimenti il confronto avverrebbe erroneamente tra `x_new` e se stesso.

### 3.5 Compilazione, esecuzione e output atteso

```bash
mpicxx -O2 -Wall -o jacobi_ls Jacobi_linear_system.cpp
mpirun -np 4 ./jacobi_ls
```

> ⚠️ Questo esercizio richiede **esattamente 4 processi** (uno per incognita), per i motivi strutturali discussi in sezione 3.2. Lanciare l'eseguibile con un numero di processi diverso produce un comportamento non definito dal programma stesso (tipicamente un accesso fuori dai casi gestiti dallo `switch(rank)`, oppure un dimensionamento errato dei buffer di `MPI_Allgather`), non un errore rilevato automaticamente da MPI a runtime.

Il programma scrive la soluzione su `output.dat` (a cura del processo 0, unico responsabile dell'I/O su file per evitare scritture concorrenti da più processi sullo stesso file):

```text
x = 1.00...
y = 2.00...
z = -1.00...
t = 1.00...
N of iterations : 26
```

La soluzione esatta del sistema è `x₀=1, x₁=2, x₂=-1, x₃=1`, verificabile per sostituzione diretta nelle quattro equazioni originali di sezione 3.1. Il numero di iterazioni necessarie per la convergenza (`26`) è sensibilmente inferiore rispetto al tipico numero di iterazioni dell'esercizio 1 (dell'ordine delle migliaia): questo è atteso e coerente con la natura dei due problemi, non un'indicazione di un'implementazione più efficiente. Il sistema 4×4 ha un raggio spettrale della matrice di iterazione `ρ(T)` sensibilmente più piccolo (dominanza diagonale "abbondante", si vedano i margini calcolati in sezione 3.1: ad esempio riga 3, `8 > 4`, un margine del 100% rispetto alla soglia di dominanza) rispetto al laplaciano discretizzato dell'esercizio 1, il cui raggio spettrale si avvicina a 1 tanto più la griglia è fine (sezione 2.8): la velocità di convergenza geometrica del metodo di Jacobi è direttamente governata da `ρ(T)`, non dalla dimensione nominale del problema (4 incognite contro N² incognite).

## 4. Confronto tra le due strategie di parallelizzazione

| Aspetto | Esercizio 1 (griglia) | Esercizio 2 (sistema lineare) |
|---|---|---|
| Natura del partizionamento | Spaziale (decomposizione del dominio) | Algebrico (decomposizione per incognita) |
| Scalabilità nel numero di processi | Arbitraria, `P ≤ N` (idealmente `N` divisibile per `P`) | Fissa, `P` deve essere uguale alla dimensione del sistema |
| Dipendenze dati per l'aggiornamento locale | Solo dai vicini topologici immediati (stencil a 5 punti → 2 vicini in 1D-strips) | Da tutte le altre incognite (matrice densa) |
| Pattern di comunicazione | P2P mirato (halo exchange, `MPI_Sendrecv`) verso un numero costante di vicini | Collettivo (`MPI_Allgather`) verso tutti i processi |
| Volume di comunicazione per processo per iterazione | O(N) (larghezza di una riga), indipendente da P | O(P) (un valore da/verso ciascun altro processo) |
| Origine della matrice di iterazione | Sparsa, a struttura regolare (pentadiagonale a blocchi) | Densa, senza struttura regolare sfruttabile |
| Collettive usate | Solo `MPI_Allreduce` (criterio di convergenza) | `MPI_Allreduce` (convergenza) + `MPI_Allgather` (ridistribuzione dati) |

Il punto concettuale di fondo, utile a generalizzare oltre questi due esercizi specifici: la scelta tra comunicazione P2P mirata e comunicazione collettiva non è arbitraria, ma **discende direttamente dalla struttura delle dipendenze dati** dell'algoritmo. Quando le dipendenze sono **locali/sparse** (ogni unità di calcolo dipende solo da un piccolo sottoinsieme fisso di altre unità, tipicamente per prossimità spaziale o topologica), la comunicazione P2P mirata verso i soli vicini rilevanti è la scelta efficiente, poiché evita di coinvolgere processi che non hanno alcuna dipendenza dati reciproca. Quando le dipendenze sono **globali/dense** (ogni unità di calcolo dipende, in generale, da tutte le altre), una comunicazione collettiva che coinvolge l'intero communicator è invece la scelta corretta, perché il pattern di comunicazione P2P equivalente degenererebbe comunque in un all-to-all completo, che le primitive collettive implementano in modo più efficiente di un'analoga sequenza di chiamate P2P scritte a mano (guida 03, sezione 9).

## 5. Errori comuni e come evitarli

| Errore | Causa tipica | Come evitarlo |
|---|---|---|
| Risultato numericamente errato nell'esercizio 1, pur senza crash o deadlock | Aggiornamento in-place dello stencil (scrittura diretta su `u` invece che su un buffer separato `u_new`), che introduce una dipendenza spuria da valori già aggiornati nella stessa iterazione, violando la semantica di Jacobi (sezione 2.7) | Mantenere sempre due buffer distinti per iterazione e scambiarli (`std::swap`) solo al termine, mai scrivere il risultato dello stencil direttamente nel buffer da cui si sta ancora leggendo |
| Deadlock o valori corrotti nelle ghost row | Halo exchange implementato con due `MPI_Send`/`MPI_Recv` bloccanti separati e ordinati in modo non simmetrico tra processi vicini (lo stesso scenario della guida 01a, sezione 6), invece di usare `MPI_Sendrecv` | Usare `MPI_Sendrecv` per l'halo exchange, che gestisce internamente l'ordinamento sicuro di invio/ricezione simultanei; alternativamente, ordinare esplicitamente send/recv come descritto in 01a sezione 6.2 |
| Crash per accesso fuori dai limiti dell'array nei processi ai bordi della decomposizione (rank 0 o rank P-1) | Codice di halo exchange che assume incondizionatamente l'esistenza di entrambi i vicini nord e sud, senza gestire il caso limite dei processi di bordo | Proteggere le chiamate relative al vicino mancante con una condizionale sul rank, oppure usare `MPI_PROC_NULL` come rank sostitutivo per il vicino inesistente (sezione 2.5) |
| Il criterio di convergenza globale non si attiva mai, o si attiva in modo incoerente su processi diversi | Uso di `MPI_Reduce` (solo root) invece di `MPI_Allreduce` per il criterio d'arresto, con la decisione di uscita dal ciclo condizionata solo sul processo root, mentre gli altri processi continuano a iterare indefinitamente in attesa di comunicazioni che non arrivano più | Usare sempre `MPI_Allreduce` (non `MPI_Reduce`) quando la decisione di terminare il ciclo deve essere presa in modo identico e autonomo da ogni processo (sezione 2.6) |
| Esercizio 2 lanciato con un numero di processi diverso da 4 produce risultati insensati o crash silenziosi | Il vincolo "un processo per incognita" (sezione 3.2) non è verificato esplicitamente a runtime dal programma | Aggiungere, in fase di sviluppo, un controllo esplicito a inizio programma (`if (size != 4) { ...termina con messaggio d'errore... }`) invece di affidarsi alla documentazione soltanto |
| Nell'esercizio 2, criterio di convergenza calcolato in modo errato | Calcolo di `diff` **dopo** la `MPI_Allgather` che ha già sovrascritto `x[rank]` con il nuovo valore, confrontando quindi `x_new` con se stesso (differenza sempre nulla, convergenza apparente immediata e non corretta) | Calcolare sempre `diff = |x_new - x[rank]|` **prima** di eseguire la `MPI_Allgather` che aggiorna il vettore condiviso (sezione 3.4) |
| Scrittura concorrente su `output.dat` da più processi, con contenuto del file corrotto o troncato | Ogni processo tenta di scrivere l'intero output su file, invece che un solo processo designato | Delegare l'intera responsabilità dell'output su file a un solo processo (tipicamente il rank 0), proteggendo il blocco di scrittura con `if (rank == 0) { ... }` |
