# 01b — Comunicazione Point-to-Point Non Bloccante in MPI
---

## Indice

1. [Perché esiste la comunicazione non bloccante](#1-perché-esiste-la-comunicazione-non-bloccante)
2. [Blocking vs Non-Blocking: la differenza in una frase](#2-blocking-vs-non-blocking-la-differenza-in-una-frase)
3. [Il pattern generale: Start → Lavoro → Wait](#3-il-pattern-generale-start--lavoro--wait)
4. [MPI_Isend e MPI_Irecv](#4-mpi_isend-e-mpi_irecv)
5. [MPI_Request: l'handle dell'operazione in corso](#5-mpi_request-lhandle-delloperazione-in-corso)
6. [La regola d'oro: non toccare il buffer prima del Wait](#6-la-regola-doro-non-toccare-il-buffer-prima-del-wait)
7. [Le funzioni di completamento: Wait, Waitall, Waitany, Test](#7-le-funzioni-di-completamento-wait-waitall-waitany-test)
8. [Il vantaggio: sovrapposizione calcolo-comunicazione](#8-il-vantaggio-sovrapposizione-calcolo-comunicazione)
9. [Esercizi guidati](#9-esercizi-guidati)
10. [Output atteso e come interpretarlo](#10-output-atteso-e-come-interpretarlo)
11. [Errori comuni e come evitarli](#11-errori-comuni-e-come-evitarli)
12. [Confronto riassuntivo: quando usare cosa](#12-confronto-riassuntivo-quando-usare-cosa)

---

## 1. Perché esiste la comunicazione non bloccante

Nel capitolo precedente si nota che `MPI_Send` e `MPI_Recv` sono **bloccanti**: il processo chiamante resta fermo su quella riga di codice finché l'operazione non può considerarsi conclusa (per il send: buffer riutilizzabile; per il recv: dati arrivati per intero).

Questo comportamento ha un costo evidente: mentre un processo è bloccato in attesa di completare una comunicazione, **non può fare nient'altro**. Se un processo deve inviare dati e poi eseguire un calcolo che non dipende da quei dati, con le primitive bloccanti è comunque costretto ad aspettare la fine della comunicazione prima di iniziare a calcolare, sprecando tempo prezioso.

La comunicazione **non bloccante** consente a un processo di iniziare un'operazione di invio o ricezione senza attendere il suo completamento. Dopo la chiamata, il processo può proseguire con altre elaborazioni. L'effettivo avanzamento della comunicazione dipende dall'implementazione MPI e può avvenire durante l'esecuzione di successive chiamate MPI o attraverso meccanismi interni della libreria. Il completamento dell'operazione viene infine verificato mediante primitive dedicate, come MPI_Wait o MPI_Test.

## 2. Blocking vs Non-Blocking: la differenza in una frase

| | Blocking (`MPI_Send`/`MPI_Recv`) | Non-Blocking (`MPI_Isend`/`MPI_Irecv`) |
|---|---|---|
| Quando ritorna la chiamata | Solo a operazione (parzialmente) conclusa | Immediatamente, l'operazione è solo "avviata" |
| Il processo nel frattempo | È fermo, non fa altro | È libero di eseguire altro codice |
| Quando i dati sono garantiti pronti | Al ritorno della funzione stessa | Solo dopo una chiamata esplicita di completamento (`MPI_Wait` e simili) |
| Rischio principale | Deadlock (sezione 6 del capitolo Blocking) | Accesso al buffer prima del completamento (sezione 6 di questo capitolo) |

La "I" iniziale di `MPI_Isend`/`MPI_Irecv` sta per **Immediate**: la chiamata ritorna immediatamente, senza attendere che l'operazione sia realmente conclusa.

## 3. Il pattern generale: Start → Lavoro → Wait

Ogni comunicazione non bloccante segue sempre questa struttura in tre fasi:

```
MPI_Isend / MPI_Irecv   →  avvia l'operazione (ritorna subito)
         [lavoro utile]  →  calcola qualcosa mentre i dati sono in transito
MPI_Wait / MPI_Waitall  →  attendi che l'operazione sia effettivamente completata
```

È fondamentale capire che **avviare** un'operazione non bloccante (fase 1) e **completarla** (fase 3) sono due passi distinti e obbligatori: per ogni `MPI_Isend`/`MPI_Irecv` avviato, prima o poi deve esistere una corrispondente chiamata di completamento. Omettere il completamento è un errore (vedi sezione 11).

```cpp
MPI_Request request;  // handle che identifica l'operazione in corso

// Fase 1: avvio (non bloccante) di un invio
MPI_Isend(buf, count, datatype, dest, tag, comm, &request);

// Fase 2: lavoro utile, eseguito MENTRE la comunicazione procede
// ... calcoli che non toccano 'buf' ...

// Fase 3: attesa esplicita del completamento
MPI_Wait(&request, MPI_STATUS_IGNORE);
```

## 4. MPI_Isend e MPI_Irecv

I prototipi di queste due funzioni sono quasi identici a quelli di `MPI_Send`/`MPI_Recv` visti nel capitolo precedente, con **due sole differenze**: non ritornano più uno stato di completamento immediato, e richiedono un parametro aggiuntivo, `MPI_Request*`.

```cpp
int MPI_Isend(
    const void*  buf,      // buffer di invio: ATTENZIONE, non va modificato
                            // finché l'operazione non è completata (sez. 6)
    int          count,    // numero di elementi da inviare
    MPI_Datatype datatype, // tipo MPI degli elementi (MPI_INT, MPI_DOUBLE...)
    int          dest,     // rank del destinatario
    int          tag,      // etichetta del messaggio
    MPI_Comm     comm,     // communicator (es. MPI_COMM_WORLD)
    MPI_Request* request   // OUTPUT: handle che identifica questa specifica
                            // operazione, da usare più avanti con MPI_Wait
);

int MPI_Irecv(
    void*        buf,      // buffer di ricezione: ATTENZIONE, i dati non sono
                            // validi finché l'operazione non è completata
    int          count,    // capacità massima del buffer
    MPI_Datatype datatype, // deve corrispondere al datatype del sender
    int          source,   // rank del mittente atteso (o MPI_ANY_SOURCE)
    int          tag,      // tag atteso (o MPI_ANY_TAG)
    MPI_Comm     comm,
    MPI_Request* request   // OUTPUT: handle dell'operazione di ricezione
);
```

Nota che `MPI_Irecv`, a differenza di `MPI_Recv`, **non richiede** un puntatore a `MPI_Status` come ultimo parametro: le informazioni sul mittente/tag effettivi e sul numero di elementi ricevuti si ottengono passando uno `MPI_Status` alla successiva chiamata di completamento (`MPI_Wait`), non alla `MPI_Irecv` stessa — semplicemente perché al momento della `MPI_Irecv` il messaggio non è ancora arrivato, quindi quelle informazioni non esistono ancora.

## 5. MPI_Request: l'handle dell'operazione in corso

`MPI_Request` è un tipo definito dallo standard MPI utilizzato per identificare e gestire un'operazione di comunicazione non bloccante. Quando una routine come `MPI_Isend()` o `MPI_Irecv()` viene invocata, la libreria MPI avvia l'operazione di comunicazione e restituisce al chiamante un oggetto di tipo `MPI_Request`, che agisce come un handle associato a tale operazione.

Un oggetto `MPI_Request` non contiene i dati del messaggio né il risultato della comunicazione; esso rappresenta esclusivamente il riferimento attraverso cui MPI può identificare l'operazione avviata. Grazie a questo handle, il processo può successivamente verificare lo stato della comunicazione o attenderne il completamento mediante routine quali `MPI_Test()`, `MPI_Wait()` e le relative varianti.

La caratteristica fondamentale delle comunicazioni non bloccanti è che la chiamata a `MPI_Isend()` o `MPI_Irecv()` restituisce immediatamente il controllo al programma senza attendere il completamento effettivo del trasferimento dei dati. Ciò consente di sovrapporre attività di calcolo e comunicazione, riducendo il tempo trascorso in attesa e migliorando potenzialmente l'efficienza dell'applicazione parallela.

Quando vengono avviate più operazioni non bloccanti contemporaneamente, è pratica comune memorizzare i corrispondenti handle in un array di `MPI_Request`, in cui ogni elemento identifica una specifica operazione di comunicazione. Tale struttura consente di gestire in modo efficiente l'insieme delle comunicazioni pendenti attraverso funzioni come `MPI_Waitall()`, `MPI_Testall()`, `MPI_Waitany()` e `MPI_Testany()`.

```cpp
MPI_Request requests[3];
MPI_Isend(buf0, count, MPI_INT, dest0, tag, comm, &requests[0]);
MPI_Isend(buf1, count, MPI_INT, dest1, tag, comm, &requests[1]);
MPI_Isend(buf2, count, MPI_INT, dest2, tag, comm, &requests[2]);
// ... poi si attenderanno tutte insieme con MPI_Waitall (sezione 7) ...
```

## 6. La regola d'oro: non toccare il buffer prima del Wait

> ⚠️Regola fondamentale
> Tra l'avvio di una comunicazione non bloccante (`MPI_Isend` o `MPI_Irecv`) e il suo completamento mediante `MPI_Wait`, `MPI_Waitall`, `MPI_Test` o funzioni equivalenti, il buffer coinvolto nella comunicazione **non deve essere utilizzato dall'applicazione**.
>
> In particolare:
>
> - il **buffer di invio** non deve essere modificato;
> - il **buffer di ricezione** non deve essere letto né modificato.
>
> Questa restrizione rimane valida fino a quando l'operazione di comunicazione non risulta completata.

## Perché questa regola è fondamentale?

Le primitive di comunicazione non bloccante ritornano immediatamente il controllo al programma, consentendo di **sovrapporre il calcolo alla comunicazione** (*communication-computation overlap*). Tuttavia, il completamento effettivo del trasferimento dei dati può avvenire in un momento successivo e dipende dall'implementazione della libreria MPI e dallo stato della comunicazione.

Di conseguenza:

- Se il programma modifica il **buffer di invio** prima del completamento dell'`MPI_Isend`, la libreria MPI potrebbe non aver ancora terminato di leggere tutti i dati dal buffer. Il messaggio inviato potrebbe quindi contenere dati modificati o incoerenti rispetto a quelli che si intendeva trasmettere.

- Se il programma legge o modifica il **buffer di ricezione** prima del completamento dell'`MPI_Irecv`, il contenuto del buffer non è ancora garantito valido dallo standard MPI. I dati potrebbero non essere ancora stati ricevuti oppure essere ancora in fase di trasferimento, rendendo il contenuto del buffer **non definito**.

Per questo motivo, il completamento della comunicazione tramite `MPI_Wait`, `MPI_Waitall`, `MPI_Test` o funzioni equivalenti rappresenta il punto in cui l'applicazione riacquista la piena disponibilità del buffer e può accedervi in modo sicuro.

> **Regola pratica:** dopo una chiamata a `MPI_Isend` o `MPI_Irecv`, il buffer coinvolto deve essere considerato **temporaneamente indisponibile** fino al completamento della relativa richiesta MPI. 

```cpp
int valore = 10;
MPI_Request req;
MPI_Isend(&valore, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &req);

valore = 20;  // ✗ ERRORE: 'valore' potrebbe essere già stato spedito
              //   come 10, come 20, o in uno stato intermedio indefinito.
              //   Il risultato non è garantito e dipende dai dettagli
              //   implementativi e di timing, esattamente come un
              //   deadlock: può "sembrare" funzionare per caso.

MPI_Wait(&req, MPI_STATUS_IGNORE);
valore = 20;  // ✓ CORRETTO: ora l'invio è completato, il buffer è libero
```

Questo è l'equivalente, nel mondo non-blocking, di ciò che il deadlock rappresenta nel mondo blocking: **il rischio principale da conoscere ed evitare sempre**.

## 7. Le funzioni di completamento: Wait, Waitall, Waitany, Test

Avviare un'operazione non basta: prima o poi bisogna verificarne (o attenderne) il completamento. MPI mette a disposizione diverse funzioni per farlo, a seconda della situazione.

| Funzione | Comportamento |
|---|---|
| `MPI_Wait(&req, &status)` | **Blocca** il processo chiamante finché la specifica richiesta `req` non è completata. È l'equivalente "puntuale" di una `MPI_Recv` bloccante, ma applicato a un'operazione già avviata. |
| `MPI_Waitall(n, reqs[], statuses[])` | **Blocca** finché **tutte** le `n` richieste nell'array `reqs[]` non sono completate. Utile quando si avviano molte comunicazioni insieme e serve solo sapere che sono *tutte* concluse, senza importare l'ordine. |
| `MPI_Waitany(n, reqs[], &idx, &status)` | **Blocca** finché **almeno una** delle `n` richieste è completata, restituendo in `idx` l'indice di quella conclusa. Utile per reagire alla prima comunicazione che termina, processando i risultati man mano che arrivano invece di aspettare tutti insieme. |
| `MPI_Test(&req, &flag, &status)` | **Non blocca mai**: controlla immediatamente se `req` è già completata, impostando `flag = 1` (vero) o `flag = 0` (falso), e ritorna subito in entrambi i casi. Utile per fare *polling*: "controlla se è pronto, e se non lo è continua a fare altro, poi ricontrolla più tardi". |

Esempio con `MPI_Waitall`, usato quando si avviano più invii contemporaneamente (come nell'Esercizio 3):

```cpp
MPI_Request requests[3];
MPI_Status  statuses[3];

for (int i = 0; i < 3; i++) {
    MPI_Isend(&dati[i], 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD, &requests[i]);
}

// ... eventuale lavoro utile qui, mentre i 3 invii procedono in background ...

MPI_Waitall(3, requests, statuses);  // aspetta che TUTTI e 3 siano completati
```

Nota che `MPI_STATUS_IGNORE` (visto nella guida precedente) ha un equivalente per gli array: `MPI_STATUSES_IGNORE`, da usare con `MPI_Waitall` quando gli status dettagliati non servono.

## 8. Il vantaggio: sovrapposizione calcolo-comunicazione

Il motivo principale per cui si usa la comunicazione non bloccante è la possibilità di **sovrapporre** (in inglese *overlap*) il tempo speso a comunicare con il tempo speso a calcolare, invece di sommarli in sequenza.

### 8.1 Caso bloccante: tutto in sequenza

```text
Tempo →
────────────────────────────────────────────────────────
Versione bloccante:
|---- Send ----|---- Receive ----|---- Calcolo ----|
Tempo totale ≈ Tsend + Trecv + Tcompute
```

Con le primitive bloccanti, invio, ricezione e calcolo avvengono uno dopo l'altro: il processo è sempre impegnato in una sola attività alla volta, e il tempo totale è semplicemente la somma dei tempi di ciascuna fase.

### 8.2 Caso non bloccante: comunicazione e calcolo in parallelo

```text
Versione non bloccante:
| Isend/Irecv |
|------------- Calcolo -------------|
                                   | Wait |
Tempo totale ≈ max(Tcomunicazione, Tcompute) + overhead di sincronizzazione
```

Qui l'operazione di comunicazione viene solo *avviata*, e il processo passa subito a eseguire calcolo utile mentre i dati viaggiano "in background". Il tempo di attesa finale (`MPI_Wait`) è spesso molto più breve del tempo di comunicazione totale, perché **buona parte della comunicazione si è già svolta durante il calcolo**.

### 8.3 Perché funziona: due attività, non una

L'idea di fondo è che comunicazione (gestita in gran parte dall'hardware di rete, in modo relativamente indipendente dalla CPU) e calcolo (che invece impegna attivamente la CPU) sono due risorse **diverse**. Se un processo le usa una dopo l'altra (blocking), il tempo totale è la loro somma. Se invece riesce a farle procedere contemporaneamente (non-blocking), il tempo totale tende al **massimo** tra le due, non alla somma:

```text
Blocking:      Tsend + Trecv + Tcompute
Non-blocking:  max(Tcomunicazione, Tcompute) + overhead di sincronizzazione
```

Il beneficio è massimo quando comunicazione e calcolo hanno **costi temporali paragonabili**: se il calcolo dura molto più a lungo della comunicazione, quest'ultima si "nasconde" quasi del tutto dietro al calcolo; se invece il calcolo è trascurabile, il vantaggio dell'overlap si riduce, perché non c'è quasi nulla con cui sovrapporre la comunicazione.

## 9. Esercizi guidati

### Esercizio 1 — Isend/Irecv di base (`ex1_nonblocking_basic.cpp`)

Reimplementazione dell'esercizio di ping-pong (già visto nella guida 01a, sezione 10, Esercizio 2) usando `MPI_Isend`/`MPI_Irecv` al posto delle versioni bloccanti.

**Obiettivo didattico:** confrontare direttamente, sullo stesso problema, la struttura del codice bloccante e non bloccante, per rendere tangibile la differenza descritta nella sezione 2 di questa guida (stessa logica applicativa, meccanismo di attesa completamente diverso).

### Esercizio 2 — Overlap calcolo-comunicazione (`ex2_overlap.cpp`)

Ogni processo avvia un invio non bloccante dei propri dati verso un processo vicino e, mentre l'invio è in transito, calcola una somma locale su un altro insieme di dati (che non dipende da quelli in invio). Il tempo di esecuzione viene confrontato con l'equivalente versione puramente bloccante (send seguito da calcolo).

**Obiettivo didattico:** misurare concretamente il vantaggio descritto nella sezione 8: verificare che il tempo totale della versione non bloccante si avvicini al `max(Tcomunicazione, Tcompute)` invece che alla loro somma.

### Esercizio 3 — Waitall con comunicazioni multiple (`ex3_waitall.cpp`)

Il processo master avvia contemporaneamente N operazioni di `MPI_Isend` (una per ciascun worker), poi esegue del lavoro indipendente, e infine attende il completamento di tutte le comunicazioni con una singola chiamata a `MPI_Waitall`.

**Obiettivo didattico:** gestire un array di `MPI_Request` per operazioni multiple simultanee (sezione 5) e capire quando `MPI_Waitall` è preferibile a più chiamate separate di `MPI_Wait` in sequenza (più semplice da scrivere, e permette a MPI di completare le richieste nell'ordine più efficiente, non necessariamente quello di avvio).

## 10. Output atteso e come interpretarlo

### ex3_waitall (eseguito con `-np 4`)

```text
[Master] Starting 3 non-blocking Isend operations...
[Master] Doing other work while the data is in transit...
[Master] MPI_Waitall: all communications completed.
[Worker 1] Received: 100
[Worker 2] Received: 200
[Worker 3] Received: 300
```

Da notare la sequenza logica delle stampe del master: prima annuncia l'avvio delle 3 `MPI_Isend` (fase 1 del pattern, sezione 3), poi stampa che sta eseguendo "altro lavoro" **prima** di aver ricevuto conferma che le comunicazioni siano concluse (fase 2, il calcolo si sovrappone alla comunicazione), e solo dopo la `MPI_Waitall` conferma che tutte e 3 sono terminate (fase 3). I tre worker, essendo processi indipendenti, possono stampare il proprio messaggio di ricezione in un ordine qualsiasi rispetto agli altri worker (l'ordine relativo tra processi diversi non è garantito, come già visto nella guida 01a), ma ciascuno riceve correttamente il valore a lui destinato (`100`, `200`, `300`).

## 11. Errori comuni e come evitarli

| Errore | Causa tipica | Come evitarlo |
|---|---|---|
| Dati inviati o ricevuti risultano sbagliati, corrotti o "vecchi" | Il buffer è stato letto o scritto prima del completamento dell'operazione (violazione della regola d'oro, sezione 6) | Non accedere mai al buffer tra `MPI_Isend`/`MPI_Irecv` e il relativo `MPI_Wait`/`MPI_Waitall`; se serve un nuovo buffer per nuovi dati mentre la comunicazione precedente è ancora in corso, usarne uno diverso |
| Il programma si blocca (o va in crash) su una `MPI_Wait` | È stata dimenticata la chiamata di completamento per una richiesta avviata, oppure si aspetta una `MPI_Request` mai inizializzata correttamente | Per ogni `MPI_Isend`/`MPI_Irecv` avviato deve sempre esistere, prima o poi, una corrispondente `MPI_Wait` (o `MPI_Waitall`/`MPI_Waitany`/`MPI_Test` fino a completamento) |
| Perdita di memoria o comportamento indefinito nel tempo | Le `MPI_Request` non vengono mai completate, restando "appese" indefinitamente | Tracciare sempre tutte le request avviate (es. in un array) e assicurarsi che ciascuna venga completata prima della fine del programma |
| `MPI_Test` sembra "non funzionare mai" (`flag` sempre 0) | Si controlla il completamento troppo presto rispetto ai tempi reali della rete, oppure si interpreta erroneamente `flag == 0` come errore | `flag == 0` è un esito normale e previsto di `MPI_Test`: significa semplicemente "non ancora completata", non un errore; il polling va ripetuto finché `flag` non diventa 1 |
| Nessun guadagno di prestazioni osservato tra versione blocking e non-blocking | Il calcolo eseguito durante l'attesa (fase 2) è troppo breve rispetto al tempo di comunicazione, oppure non c'è alcun calcolo indipendente da sovrapporre | Verificare che esista un carico di lavoro reale e indipendente dai dati in transito da eseguire tra l'avvio e il completamento; l'overlap non offre benefici se non c'è nulla con cui sovrapporsi (sezione 8.3) |

## 12. Confronto riassuntivo: quando usare cosa

* Usa la comunicazione **bloccante** (guida 01a) quando la logica del programma è semplice, il costo di attesa non è un problema, e la priorità è la chiarezza del codice.
* Usa la comunicazione **non bloccante** (questa guida) quando:
  * hai calcolo utile e indipendente da eseguire mentre i dati viaggiano;
  * devi avviare molte comunicazioni contemporaneamente e vuoi attenderle insieme in modo efficiente (`MPI_Waitall`);
  * vuoi evitare a priori i rischi di deadlock descritti nella guida 01a, dato che `MPI_Isend`/`MPI_Irecv` non si bloccano mai in attesa di un partner pronto.

Il prezzo da pagare per questi vantaggi è una gestione più attenta del ciclo di vita di ogni comunicazione: ricordare sempre di completare ogni operazione avviata, e rispettare rigorosamente la regola d'oro della sezione 6.
