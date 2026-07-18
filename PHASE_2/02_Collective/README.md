# 02 — Comunicazione Collettiva in MPI
---

## Indice

1. [Definizione e proprietà delle operazioni collettive](#1-definizione-e-proprietà-delle-operazioni-collettive)
2. [Panoramica delle operazioni](#2-panoramica-delle-operazioni)
3. [MPI_Bcast](#3-mpi_bcast)
4. [MPI_Scatter e MPI_Gather](#4-mpi_scatter-e-mpi_gather)
5. [MPI_Allgather](#5-mpi_allgather)
6. [MPI_Reduce e MPI_Allreduce](#6-mpi_reduce-e-mpi_allreduce)
7. [MPI_Alltoall](#7-mpi_alltoall)
8. [MPI_Barrier nel contesto delle collettive](#8-mpi_barrier-nel-contesto-delle-collettive)
9. [Considerazioni algoritmiche e di costo](#9-considerazioni-algoritmiche-e-di-costo)
10. [MPI_IN_PLACE e gestione dei buffer](#10-mpi_in_place-e-gestione-dei-buffer)
11. [Esercizi guidati](#11-esercizi-guidati)
12. [Output atteso e come interpretarlo](#12-output-atteso-e-come-interpretarlo)
13. [Errori comuni e come evitarli](#13-errori-comuni-e-come-evitarli)

---

## 1. Definizione e proprietà delle operazioni collettive

Una comunicazione **collettiva** è un'operazione a cui partecipano simultaneamente **tutti** i processi di un communicator, in contrapposizione alle operazioni point-to-point (capitoli 01a/01b), in cui la comunicazione coinvolge esattamente una coppia sender/receiver identificata esplicitamente per rank.

Le collettive non hanno un concetto di "mittente" e "destinatario" nel senso P2P del termine: ogni processo del communicator invoca **la stessa funzione MPI**, con lo stesso `comm`, mettendo a disposizione (o richiedendo) dati secondo un pattern di comunicazione predefinito dalla semantica dell'operazione stessa (broadcast, scatter, gather, reduce, ecc.).

Questo comporta un vincolo stringente, da rispettare sempre:

> ⚠️ **Regola fondamentale**: ogni processo del communicator deve invocare **la stessa funzione collettiva**, con parametri consistenti (stesso `datatype`, stesso `root` dove applicabile, count coerenti). Non esiste una nozione di "partecipazione opzionale": se anche un solo processo del communicator non effettua la chiamata corrispondente, il programma si blocca indefinitamente (gli altri processi restano in attesa di un partecipante che non arriverà mai a quel punto del codice).

A differenza delle operazioni P2P bloccanti, dove il matching avviene tramite tag e rank espliciti, nelle collettive il matching è implicito: è la sequenza di chiamate collettive, nello stesso ordine su tutti i processi, a determinare quale invocazione corrisponde a quale. Chiamare collettive diverse (o la stessa collettiva con parametri strutturalmente diversi, es. datatype incompatibili) su processi diversi nello stesso "punto logico" del programma produce comportamento indefinito, spesso un deadlock o una corruzione silenziosa dei dati.

Va inoltre chiarito un equivoco comune: le operazioni collettive **non implicano necessariamente una sincronizzazione globale in stile `MPI_Barrier`**. Alcune implementazioni possono far ritornare una collettiva a un processo prima che tutti gli altri l'abbiano completata (dipende dall'algoritmo interno usato e dalla topologia di comunicazione). L'unica garanzia semantica è che, al ritorno della chiamata, l'effetto dell'operazione sul processo chiamante è completo e corretto (es. dopo una `MPI_Bcast`, il buffer locale contiene il dato broadcastato) — non che tutti gli altri processi abbiano già completato la propria istanza della chiamata.

## 2. Panoramica delle operazioni

```
MPI_Bcast      → un processo invia a tutti
MPI_Scatter    → un processo distribuisce porzioni diverse a ciascun processo
MPI_Gather     → tutti i processi inviano al root, che li raccoglie
MPI_Allgather  → tutti raccolgono da tutti (equivalente a Gather + Bcast del risultato)
MPI_Reduce     → tutti contribuiscono, il root ottiene il risultato aggregato
MPI_Allreduce  → tutti contribuiscono, TUTTI ottengono il risultato aggregato
MPI_Alltoall   → ogni processo invia dati diversi a ciascun altro processo (transpose)
MPI_Barrier    → sincronizzazione pura: nessun dato scambiato, tutti attendono tutti
```

Concettualmente queste operazioni si possono classificare per il pattern di flusso dati che implementano:

* **One-to-all** (distribuzione): `MPI_Bcast`, `MPI_Scatter`
* **All-to-one** (raccolta/aggregazione): `MPI_Gather`, `MPI_Reduce`
* **All-to-all** (scambio simmetrico): `MPI_Allgather`, `MPI_Allreduce`, `MPI_Alltoall`
* **Sincronizzazione pura, nessun trasferimento dati**: `MPI_Barrier`

## 3. MPI_Bcast

`MPI_Bcast` replica il contenuto del buffer di un singolo processo (`root`) su tutti gli altri processi del communicator.

```
Prima:  Proc 0 [X]  Proc 1 [?]  Proc 2 [?]  Proc 3 [?]
                        ↓
Dopo:   Proc 0 [X]  Proc 1 [X]  Proc 2 [X]  Proc 3 [X]
```

```cpp
int MPI_Bcast(
    void*        buf,      // IN sul root, OUT su tutti gli altri: stesso
                            // buffer usato sia per fornire che per ricevere
                            // il dato, a seconda del rank del chiamante
    int          count,    // numero di elementi da distribuire
    MPI_Datatype datatype,
    int          root,     // rank del processo sorgente del broadcast:
                            // deve essere IDENTICO su tutti i processi
    MPI_Comm     comm
);
```

Osservazioni implementative rilevanti:

* Il parametro `buf` ha una semantica **dipendente dal rank del chiamante**: sul processo `root` è un buffer di **input** (contiene già il dato da distribuire), su tutti gli altri processi è un buffer di **output** (verrà scritto dalla chiamata). Non esistono due parametri distinti `sendbuf`/`recvbuf` come in altre collettive, perché il dato è identico su ogni processo al termine dell'operazione: un solo buffer è sufficiente concettualmente, anche se ogni processo lo alloca nella propria memoria privata.
* Il valore di `root` deve essere coerente su tutti i processi chiamanti. Un valore di `root` diverso tra processi diversi produce comportamento indefinito, tipicamente un deadlock o un broadcast da una sorgente inattesa.
* Dal punto di vista algoritmico, un'implementazione naïve di `MPI_Bcast` come sequenza di `MPI_Send` P2P dal root a ciascun processo avrebbe costo lineare O(P) in numero di processi P, con il root come collo di bottiglia seriale. Le implementazioni MPI di produzione usano invece algoritmi ad albero binario o binomiale (il root invia a 2 processi, ciascuno di questi inoltra a 2 ulteriori processi, e così via), portando il costo a O(log P) passi di comunicazione paralleli, oppure algoritmi a pipeline per messaggi di grandi dimensioni, che segmentano il buffer e sovrappongono la trasmissione dei segmenti successivi lungo l'albero.

## 4. MPI_Scatter e MPI_Gather

Queste due operazioni sono concettualmente inverse l'una dell'altra.

```
Scatter:
  Root [A B C D]  →  Proc 0:[A]  Proc 1:[B]  Proc 2:[C]  Proc 3:[D]

Gather:
  Proc 0:[A]  Proc 1:[B]  Proc 2:[C]  Proc 3:[D]  →  Root [A B C D]
```

`MPI_Scatter` suddivide un buffer contiguo posseduto dal root in P blocchi contigui di uguale dimensione (`sendcount` elementi ciascuno) e ne distribuisce uno per processo, nell'ordine dei rank: il blocco `i`-esimo (a partire dall'offset `i * sendcount` nel buffer del root) va al processo di rank `i`.

`MPI_Gather` esegue l'operazione inversa: ogni processo fornisce un blocco di `sendcount` elementi, e il root li assembla in un buffer contiguo, ordinato per rank crescente (il blocco del processo `i` finisce all'offset `i * recvcount` nel buffer del root).

```cpp
int MPI_Scatter(
    const void*  sendbuf,   // significativo SOLO sul root: buffer sorgente
                             // completo, di dimensione >= sendcount * P
    int          sendcount, // numero di elementi INVIATI a CIASCUN processo
                             // (non il totale!). Significativo solo sul root
    MPI_Datatype sendtype,  // significativo solo sul root
    void*        recvbuf,   // buffer di destinazione locale, su OGNI processo
                             // (incluso il root, che riceve anche lui la
                             // propria porzione tramite questo parametro)
    int          recvcount, // numero di elementi ricevuti da QUESTO processo
    MPI_Datatype recvtype,
    int          root,      // deve essere identico su tutti i processi
    MPI_Comm     comm
);

int MPI_Gather(
    const void*  sendbuf,   // buffer locale di OGNI processo, contenente
                             // il proprio contributo da inviare al root
    int          sendcount, // numero di elementi inviati da QUESTO processo
    MPI_Datatype sendtype,
    void*        recvbuf,   // significativo SOLO sul root: buffer di
                             // destinazione, dimensionato per contenere
                             // recvcount * P elementi
    int          recvcount, // numero di elementi ricevuti DA CIASCUN processo
                             // (non il totale). Significativo solo sul root
    MPI_Datatype recvtype,
    int          root,
    MPI_Comm     comm
);
```

Alcuni punti tecnici da tenere presenti:

* Nonostante `sendbuf` in `MPI_Scatter` (e `recvbuf` in `MPI_Gather`) siano parametri "significativi solo sul root", **ogni processo, incluso il non-root, deve comunque passare un valore valido** per quei parametri nella propria chiamata (tipicamente `nullptr` o un puntatore qualsiasi non dereferenziato, a seconda delle convenzioni del binding linguistico usato): il valore non verrà letto/scritto sui processi non-root, ma il parametro deve essere comunque presente nella firma della chiamata C/C++.
* `sendcount` in `MPI_Scatter` e `recvcount` in `MPI_Gather` rappresentano la quantità di dati **per singolo processo**, non il totale aggregato: un errore comune è passare la dimensione totale del vettore invece della dimensione della singola porzione (vedi sezione 13).
* Se il numero di elementi totali non è esattamente divisibile per il numero di processi P, `MPI_Scatter`/`MPI_Gather` "di base" non gestiscono automaticamente il resto: è necessario ricorrere alle varianti `MPI_Scatterv`/`MPI_Gatherv`, che accettano array di count e displacement per processo, permettendo blocchi di dimensione non uniforme. Queste varianti non sono trattate in questa guida introduttiva.
* Il root, in entrambe le operazioni, partecipa anche lui come "destinatario/mittente" di una propria porzione: nella pratica il root esegue sia il ruolo di coordinatore che quello di normale partecipante, ricevendo/inviando la porzione corrispondente al proprio rank come tutti gli altri.

## 5. MPI_Allgather

`MPI_Allgather` è semanticamente equivalente a una `MPI_Gather` seguita da una `MPI_Bcast` del risultato assemblato: ogni processo fornisce un blocco, e **tutti** i processi (non solo un root) ottengono il vettore completo assemblato.

```cpp
int MPI_Allgather(
    const void*  sendbuf,   // buffer locale di OGNI processo
    int          sendcount, // elementi inviati da QUESTO processo
    MPI_Datatype sendtype,
    void*        recvbuf,   // buffer di destinazione su OGNI processo,
                             // dimensionato per recvcount * P elementi:
                             // TUTTI ricevono il vettore completo
    int          recvcount, // elementi ricevuti da CIASCUN processo
                             // (non il totale)
    MPI_Datatype recvtype,
    MPI_Comm     comm       // nessun parametro root: non ha senso, dato
                             // che tutti ricevono lo stesso risultato
);
```

Da un punto di vista di costo computazionale/di comunicazione, `MPI_Allgather` non è semplicemente "più economica" di una Gather+Bcast eseguite in sequenza: le implementazioni MPI tipicamente usano algoritmi dedicati (ad esempio ring-based o basati su raddoppio ricorsivo, *recursive doubling*) che sfruttano la topologia di rete per ottenere un throughput aggregato superiore rispetto a due operazioni collettive separate ed eseguite in sequenza.

## 6. MPI_Reduce e MPI_Allreduce

Le operazioni di riduzione applicano un operatore associativo (e, nella maggior parte dei casi, anche commutativo) a un insieme di valori distribuiti su più processi, producendo un singolo risultato aggregato.

```
Proc 0:[2]  Proc 1:[5]  Proc 2:[3]  Proc 3:[1]
                    ↓  MPI_SUM
                  Root:[11]
```

```cpp
int MPI_Reduce(
    const void*  sendbuf, // buffer locale di input su OGNI processo
    void*        recvbuf, // significativo SOLO sul root: conterrà il
                           // risultato aggregato al termine della chiamata
    int          count,   // numero di elementi (la riduzione è applicata
                           // elemento per elemento, se count > 1)
    MPI_Datatype datatype,
    MPI_Op       op,       // operatore di riduzione (vedi tabella sotto)
    int          root,
    MPI_Comm     comm
);

int MPI_Allreduce(
    const void*  sendbuf,
    void*        recvbuf, // significativo su TUTTI i processi: ciascuno
                           // ottiene il medesimo risultato aggregato
    int          count,
    MPI_Datatype datatype,
    MPI_Op       op,
    MPI_Comm     comm      // nessun parametro root, per lo stesso motivo
                           // visto in MPI_Allgather
);
```

**Operatori di riduzione predefiniti:**

| Costante MPI | Operazione |
|---|---|
| `MPI_SUM` | somma |
| `MPI_PROD` | prodotto |
| `MPI_MAX` | massimo |
| `MPI_MIN` | minimo |
| `MPI_LAND` | AND logico |
| `MPI_LOR` | OR logico |
| `MPI_BAND` | AND bit a bit |
| `MPI_BOR` | OR bit a bit |
| `MPI_MAXLOC` | massimo + indice del processo/posizione che lo possiede |
| `MPI_MINLOC` | minimo + indice del processo/posizione che lo possiede |

Note tecniche rilevanti:

* `MPI_MAXLOC` e `MPI_MINLOC` richiedono datatype "composti" specifici (es. `MPI_DOUBLE_INT`, `MPI_FLOAT_INT`, coppie valore-indice), non un semplice `MPI_DOUBLE`: il buffer deve contenere sia il valore che l'indice/rank associato, impacchettati secondo il layout previsto da questi datatype speciali.
* È possibile definire operatori di riduzione **custom**, non predefiniti, tramite `MPI_Op_create`, fornendo una funzione utente che implementa la logica di combinazione. Questo esula dagli obiettivi di questa guida introduttiva, ma è utile sapere che l'insieme di operatori sopra non è esaustivo per ogni caso d'uso.
* La standard MPI richiede che l'operatore fornito sia **associativo**; la **commutatività** non è invece un requisito rigido per gli operatori built-in elencati (che sono comunque tutti commutativi), ma diventa rilevante se si definiscono operatori custom: un operatore non commutativo può produrre risultati diversi a seconda dell'ordine con cui l'implementazione MPI combina i contributi dei vari processi, ordine che **non è specificato dallo standard** e può variare tra implementazioni o run diverse dello stesso programma.
* `MPI_Reduce` consegna il risultato solo al root; se il risultato serve a tutti i processi (caso comune, ad esempio per un criterio di convergenza globale in un solver iterativo), usare direttamente `MPI_Allreduce` è preferibile, sia per chiarezza del codice sia per efficienza: un'implementazione naïve "Reduce + Bcast separati" richiede due passate collettive distinte, mentre `MPI_Allreduce` è tipicamente implementata con un algoritmo dedicato a costo inferiore rispetto alla somma dei costi delle due operazioni separate.

## 7. MPI_Alltoall

`MPI_Alltoall` è l'operazione collettiva più generale tra quelle presentate: ogni processo invia una porzione di dati **distinta** a ciascun altro processo del communicator (se stesso incluso), realizzando di fatto una trasposizione distribuita di una matrice logica P×P di blocchi, dove la riga `i` rappresenta i dati posseduti dal processo `i` da distribuire, e la colonna `j` rappresenta i dati che il processo `j` riceverà da ciascuno.

```cpp
int MPI_Alltoall(
    const void*  sendbuf,   // buffer locale: contiene P blocchi contigui di
                             // sendcount elementi ciascuno; il blocco j-esimo
                             // è destinato al processo di rank j
    int          sendcount, // elementi inviati a CIASCUN processo (non il totale)
    MPI_Datatype sendtype,
    void*        recvbuf,   // buffer locale di destinazione: il blocco
                             // ricevuto dal processo i finisce all'offset
                             // i * recvcount
    int          recvcount, // elementi ricevuti DA CIASCUN processo
    MPI_Datatype recvtype,
    MPI_Comm     comm
);
```

Rispetto alle altre collettive presentate, `MPI_Alltoall` non ha un pattern di comunicazione riconducibile a un albero con un singolo punto di origine o destinazione: ogni processo è simultaneamente sorgente e destinazione di P messaggi distinti (incluso, tipicamente, un messaggio "verso se stesso", gestito internamente come una semplice copia locale senza transito di rete). Il volume aggregato di dati scambiati sulla rete scala con O(P²) messaggi complessivi nel communicator, rendendo `MPI_Alltoall` l'operazione collettiva potenzialmente più costosa in termini di traffico di rete generato, specie al crescere di P.

Questo pattern è alla base di algoritmi distribuiti che richiedono una riorganizzazione globale dei dati tra processi, il caso più noto essendo la trasposizione di matrici distribuite necessaria negli algoritmi FFT (Fast Fourier Transform) distribuiti a più dimensioni, dove i dati devono essere ridistribuiti lungo un asse diverso da quello su cui erano originariamente partizionati tra i processi.

## 8. MPI_Barrier nel contesto delle collettive

`MPI_Barrier` è già stata introdotta nella guida 01a come primitiva di sincronizzazione bloccante; qui va inquadrata correttamente come **caso particolare** di operazione collettiva: rispetta la stessa regola fondamentale (tutti i processi del communicator devono invocarla) ma, a differenza di tutte le altre collettive presentate in questa guida, non trasferisce alcun dato applicativo tra i processi — il suo unico effetto è garantire che nessun processo prosegua oltre la barriera finché tutti gli altri non l'hanno raggiunta.

Va sottolineato che `MPI_Barrier` è l'**unica** collettiva per cui lo standard MPI garantisce esplicitamente una sincronizzazione temporale completa tra tutti i partecipanti al ritorno della chiamata. Come discusso nella sezione 1, per le collettive che trasferiscono dati (Bcast, Scatter, Gather, Reduce, ecc.) questa garanzia non sussiste in generale: un processo può in linea di principio ritornare dalla chiamata collettiva prima che altri processi abbiano completato la propria.

## 9. Considerazioni algoritmiche e di costo

Le implementazioni MPI di produzione (OpenMPI, MPICH e derivati) non implementano le collettive come sequenze naïve di operazioni P2P, ma selezionano dinamicamente, a runtime, tra diversi algoritmi interni, sulla base di fattori quali: dimensione del messaggio, numero di processi nel communicator, topologia fisica dell'interconnessione (rete locale al nodo vs rete tra nodi), e talvolta euristiche configurabili dall'utente tramite variabili d'ambiente specifiche dell'implementazione.

Alcuni pattern algoritmici ricorrenti, utili per ragionare sulle prestazioni attese:

* **Algoritmi ad albero (binomiale o binario)**: usati tipicamente per `MPI_Bcast` e `MPI_Reduce` con messaggi di dimensione piccola/media. Il costo di latenza scala come O(log P), poiché l'informazione si propaga raddoppiando (approssimativamente) il numero di processi coinvolti ad ogni passo.
* **Algoritmi a pipeline**: usati per messaggi di grandi dimensioni, dove il buffer viene segmentato in chunk più piccoli che vengono inoltrati lungo l'albero di comunicazione in sequenza sovrapposta, così da saturare la banda disponibile invece di essere limitati dalla sola latenza.
* **Recursive doubling**: usato tipicamente per `MPI_Allreduce`/`MPI_Allgather` con P potenza di 2 (o gestito con varianti per P generico), dove ad ogni passo il numero di processi che possiedono il dato aggregato raddoppia, portando anch'esso a un costo O(log P) passi.
* **Algoritmi diretti (all-to-all completo)**: usati per `MPI_Alltoall`, dove ogni processo comunica direttamente con ogni altro; per P piccolo questo è spesso preferibile a schemi più complessi, mentre per P grande alcune implementazioni adottano varianti a scambio indiretto per ridurre il numero di connessioni di rete simultanee.

Ai fini pratici di questa guida, il punto rilevante non è memorizzare quale algoritmo specifico venga scelto in quale caso (dettaglio dipendente dall'implementazione e non parte dello standard MPI), ma **avere consapevolezza che le collettive non sono equivalenti, in termini di costo, a un ciclo di operazioni P2P scritto manualmente**: preferire sempre la primitiva collettiva corrispondente quando il pattern di comunicazione lo consente, delegando all'implementazione MPI la scelta dell'algoritmo più efficiente per quel caso specifico.

## 10. MPI_IN_PLACE e gestione dei buffer

In diverse collettive (in particolare `MPI_Gather`, `MPI_Allgather`, `MPI_Reduce`, `MPI_Allreduce`, `MPI_Scatter`), MPI mette a disposizione la costante speciale `MPI_IN_PLACE`, utilizzabile al posto del puntatore a `sendbuf` (o, a seconda dell'operazione, `recvbuf`) per indicare che il buffer di input e di output coincidono, evitando così un'allocazione e una copia ridondante.

Esempio con `MPI_Allreduce`, dove ogni processo vuole sovrascrivere il proprio buffer locale con il risultato aggregato invece di usare un buffer separato per l'output:

```cpp
double valore = calcola_valore_locale();

// Variante con buffer separati:
double risultato;
MPI_Allreduce(&valore, &risultato, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

// Variante con MPI_IN_PLACE: 'valore' funge sia da input che da output
MPI_Allreduce(MPI_IN_PLACE, &valore, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
```

L'uso di `MPI_IN_PLACE` non è solo una comodità sintattica: su buffer di grandi dimensioni evita un'allocazione di memoria aggiuntiva e la copia dei dati che ne conseguirebbe, con un beneficio misurabile sia in termini di footprint di memoria che di tempo di esecuzione. La semantica esatta di `MPI_IN_PLACE` varia leggermente tra le diverse collettive (per `MPI_Gather`/`MPI_Scatter` va passato al posto del parametro relativo al processo non-root vs root, con regole specifiche caso per caso): si consiglia di consultare la documentazione della funzione specifica prima di adottarlo, poiché un uso scorretto porta tipicamente a comportamento indefinito piuttosto che a un errore di compilazione.

## 11. Esercizi guidati

### Esercizio 1 — Bcast e calcolo di PI (`ex1_bcast_pi.cpp`)

Il root esegue una `MPI_Bcast` dei parametri di calcolo (in particolare il numero totale di termini della serie), dopodiché ciascun worker calcola in autonomia la propria porzione di sommatoria della serie di Leibniz per l'approssimazione di π, e infine i contributi parziali vengono aggregati con una `MPI_Reduce` (operatore `MPI_SUM`) sul root.

**Obiettivo:** applicare in sequenza due pattern collettivi complementari — distribuzione dei parametri (`Bcast`) e aggregazione dei risultati (`Reduce`) — tipici della quasi totalità dei problemi di calcolo parallelo a workload embarrassingly parallel, in cui i processi lavorano su porzioni indipendenti del dominio e i risultati parziali vengono combinati a fine calcolo.

### Esercizio 2 — Scatter/Gather: somma distribuita (`ex2_scatter_gather.cpp`)

Il root possiede un vettore di grandi dimensioni. Lo distribuisce con `MPI_Scatter`, ciascun worker elabora localmente la propria porzione (calcolo di una somma parziale, o trasformazione elemento per elemento, a seconda dell'implementazione specifica dell'esercizio), e i risultati vengono raccolti con `MPI_Gather` sul root per la ricomposizione finale.

**Obiettivo:** gestire correttamente la relazione tra dimensione totale del dato, numero di processi e dimensione della porzione locale (`sendcount`/`recvcount` per processo, non il totale — vedi sezione 4), incluso il caso in cui la dimensione del vettore sia scelta esattamente divisibile per il numero di processi per evitare la necessità di `MPI_Scatterv`/`MPI_Gatherv`.

### Esercizio 3 — Allreduce: norma distribuita (`ex3_allreduce.cpp`)

Ciascun processo possiede un vettore locale (una porzione di un vettore logicamente più grande, distribuito tra i processi). Si calcola la norma L2 globale del vettore completo tramite una somma dei quadrati locali seguita da una `MPI_Allreduce` con operatore `MPI_SUM`, in modo che ogni processo ottenga il risultato finale (necessario, ad esempio, per un test di convergenza locale identico su tutti i processi, senza dover ridistribuire il risultato con una `Bcast` separata).

**Obiettivo:** riconoscere i casi in cui `MPI_Allreduce` è la scelta corretta rispetto a `MPI_Reduce` — ovvero quando il risultato aggregato serve a **tutti** i processi per proseguire il calcolo, non solo al root — e applicare correttamente il pattern "riduzione locale (somma dei quadrati) seguita da riduzione globale (Allreduce)" comune in molti algoritmi numerici distribuiti (calcolo di norme, prodotti scalari distribuiti, criteri di arresto in solver iterativi).

### Esercizio 4 — Alltoall: trasposizione distribuita (`ex4_alltoall.cpp`)

Ciascun processo invia una porzione di dati distinta a ciascun altro processo del communicator tramite `MPI_Alltoall`, realizzando una trasposizione distribuita dei dati.

**Obiettivo:** comprendere il pattern di comunicazione all-to-all e il suo utilizzo come blocco costruttivo fondamentale per algoritmi FFT distribuiti multi-dimensionali (dove i dati vanno periodicamente ridistribuiti lungo un asse diverso da quello di partizionamento corrente), nonché prendere consapevolezza dell'impatto sul traffico di rete generato (O(P²) messaggi nel communicator, sezione 7), rilevante nella scelta tra questo pattern e alternative basate su comunicazioni P2P mirate quando la matrice di comunicazione è sparsa (ogni processo comunica solo con un sottoinsieme degli altri, non con tutti).

## 12. Output atteso e come interpretarlo

### ex1_bcast_pi (eseguito con `-np 4`)

```text
[Root] N_terms = 1000000, broadcast to all.
[Process 0] Computing terms 0..249999
[Process 1] Computing terms 250000..499999
...
Approximated PI = 3.14159265...  (error: 9.3e-7)
```

Il numero totale di termini (`N_terms = 1000000`) viene stampato una sola volta dal root, prima della `MPI_Bcast`: dopo la broadcast, tutti i processi possiedono lo stesso valore, ma solo il root ne dà conferma esplicita in output (evitare stampe ridondanti identiche da ogni processo è buona pratica quando l'informazione non varia tra i processi). Ogni processo elabora un intervallo contiguo e disgiunto di indici della serie (partizionamento a blocchi contigui: il processo `i` elabora l'intervallo `[i * N_terms/P, (i+1) * N_terms/P)`), coerentemente con la strategia di parallelizzazione a blocchi tipica di questo genere di calcolo. Il valore finale di π è il risultato della `MPI_Reduce` con `MPI_SUM` sulle somme parziali di ciascun processo; l'errore assoluto riportato (`9.3e-7`) è coerente con la velocità di convergenza nota della serie di Leibniz per il numero di termini scelto, e serve come controllo di correttezza numerica dell'implementazione, non solo di correttezza della comunicazione.

## 13. Errori comuni e come evitarli

| Errore | Causa tipica | Come evitarlo |
|---|---|---|
| Il programma si blocca indefinitamente su una chiamata collettiva | Un processo del communicator non raggiunge quella chiamata (es. una collettiva dentro un blocco `if` che esclude alcuni rank), oppure processi diversi invocano collettive diverse nello stesso punto logico | Ogni processo del communicator deve eseguire esattamente la stessa sequenza di chiamate collettive; non condizionare l'invocazione di una collettiva a un test sul rank, salvo il caso in cui la condizione sia strutturalmente identica su tutti i processi (es. un ciclo che tutti eseguono lo stesso numero di volte) |
| Valore di `root` incoerente tra processi produce risultati imprevedibili o deadlock | `root` calcolato dinamicamente in modo diverso su processi diversi (es. da un valore non ancora sincronizzato) | Il valore di `root` deve essere una costante nota a priori, oppure derivare da un dato già sincronizzato in modo identico su tutti i processi prima della chiamata collettiva |
| Dati scambiati risultano troncati, sovrapposti o in posizione sbagliata dopo `Scatter`/`Gather`/`Alltoall` | `sendcount`/`recvcount` confusi con la dimensione totale del buffer invece che con la dimensione della porzione per singolo processo (sezione 4) | Verificare sempre che `count` rappresenti la quantità **per processo**: la dimensione totale del buffer sul root deve essere `count * P`, non `count` |
| Buffer di destinazione troppo piccolo su `MPI_Gather`/`MPI_Allgather`/`MPI_Alltoall` | Allocazione dimensionata sulla quantità di dati di un singolo processo invece che sul totale aggregato (`recvcount * P`) | Dimensionare esplicitamente i buffer di raccolta considerando il numero di processi P, non solo la dimensione del contributo locale |
| Risultato di una riduzione (`MPI_Reduce`/`MPI_Allreduce`) leggermente diverso tra run successive dello stesso programma, con lo stesso input | Operazioni in virgola mobile non associative nella pratica (arrotondamento): l'ordine con cui l'implementazione MPI combina i contributi dei singoli processi non è specificato dallo standard e può variare tra run, algoritmi interni o numero di processi usati | Non assumere riproducibilità bit-a-bit di riduzioni floating-point tra configurazioni o run diverse; se necessaria riproducibilità stretta, valutare implementazioni di riduzione con ordine di combinazione fissato esplicitamente (fuori dallo scope di questa guida) |
| Uso di `MPI_Reduce` quando in realtà serve il risultato su tutti i processi | Al bisogno effettivo dell'algoritmo si risponde con `Reduce` seguito da una `Bcast` manuale separata, invece che con `Allreduce` | Preferire direttamente `MPI_Allreduce` quando tutti i processi necessitano il risultato aggregato: è semanticamente equivalente ma più efficiente delle due chiamate separate (sezione 6) |
| Comportamento indefinito o crash usando `MPI_IN_PLACE` | Uso scorretto del parametro (posizione errata, o collettiva che non supporta `MPI_IN_PLACE` nel modo assunto) | Verificare la semantica specifica di `MPI_IN_PLACE` per la collettiva in uso prima di adottarlo (sezione 10); in caso di dubbio, preferire buffer separati come impostazione di default, ottimizzando solo dove il beneficio è misurabile |
