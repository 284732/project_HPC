# 01a — Comunicazione Point-to-Point Bloccante in MPI

---

## Indice

1. [Cos'è MPI e perché esiste](#1-cosè-mpi-e-perché-esiste)
2. [Concetti fondamentali: processo, rank, communicator](#2-concetti-fondamentali-processo-rank-communicator)
3. [Cos'è la comunicazione Point-to-Point](#3-cosè-la-comunicazione-point-to-point)
4. [MPI_Send e MPI_Recv](#4-mpi_send-e-mpi_recv)
5. [Semantica Bloccante: cosa significa davvero "blocking"](#5-semantica-bloccante-cosa-significa-davvero-blocking)
6. [Deadlock: la trappola più comune](#6-deadlock-la-trappola-più-comune)
7. [MPI_Barrier: sincronizzazione collettiva bloccante](#7-mpi_barrier-sincronizzazione-collettiva-bloccante)
8. [MPI_Status: chi mi ha scritto, cosa, quanto](#8-mpi_status-chi-mi-ha-scritto-cosa-quanto)
9. [Compilare ed eseguire un programma MPI](#9-compilare-ed-eseguire-un-programma-mpi)
10. [Esercizi guidati](#10-esercizi-guidati)
11. [Output atteso e come interpretarlo](#11-output-atteso-e-come-interpretarlo)
12. [Errori comuni e come evitarli](#12-errori-comuni-e-come-evitarli)

---

## 1. Cos'è MPI e perché esiste

**MPI (Message Passing Interface)** è uno standard (non un linguaggio, non una libreria specifica) che definisce come processi diversi, in esecuzione su core o macchine diverse, possano **scambiarsi dati** per collaborare alla soluzione di un problema.

Il modello di programmazione di MPI si chiama **message passing** ("scambio di messaggi"): a differenza della programmazione multithread (es. OpenMP), dove i thread condividono la stessa memoria, in MPI **ogni processo ha la propria memoria privata**. Se il processo A vuole che il processo B conosca un valore, deve *inviarglielo esplicitamente* attraverso un messaggio: non esiste memoria condivisa di default.

Questo modello è alla base del calcolo distribuito su cluster HPC, dove centinaia o migliaia di nodi, ciascuno con la propria RAM, devono cooperare.

```
Nodo 0 (RAM propria)        Nodo 1 (RAM propria)
┌─────────────────┐         ┌─────────────────┐
│  Processo 0      │         │  Processo 1      │
│  int x = 42;     │  msg    │  int x = ???;    │
│                  │ ──────► │  MPI_Recv(&x,..) │
└─────────────────┘         └─────────────────┘
```

## 2. Concetti fondamentali: processo, rank, communicator

Prima di poter capire `MPI_Send`/`MPI_Recv` servono tre concetti chiave.

### 2.1 Processo

Quando lanci un programma MPI con, ad esempio, 4 processi, il sistema operativo crea **4 istanze indipendenti** dello stesso eseguibile, ciascuna con la propria memoria, il proprio stack, le proprie variabili. Non sono thread: sono processi a tutti gli effetti, spesso distribuiti su core o nodi fisici diversi.

Tutti e 4 eseguono **lo stesso codice sorgente** (modello SPMD: *Single Program, Multiple Data*), ma si comportano in modo diverso a seconda del proprio *rank* — vedi sotto.

### 2.2 Rank

Il **rank** è un intero univoco (da `0` a `size-1`) che identifica un processo all'interno di un gruppo di processi. È l'equivalente di un "indirizzo" o "nome" del processo, usato per decidere chi invia a chi.

```cpp
int rank, size;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // "chi sono io?" -> il mio rank
MPI_Comm_size(MPI_COMM_WORLD, &size);  // "quanti siamo in totale?"
```

Grazie al rank, un unico file sorgente può far comportare i processi in modo diverso:

```cpp
if (rank == 0) {
    // solo il processo 0 esegue questo blocco
} else {
    // tutti gli altri eseguono quest'altro
}
```

### 2.3 Communicator

Un **communicator** è un gruppo di processi che possono comunicare tra loro. `MPI_COMM_WORLD` è il communicator "di default", creato automaticamente all'avvio, e contiene **tutti** i processi lanciati dal programma. In generale è spesso usato `MPI_COMM_WORLD`, ma è utile sapere che MPI permette di creare sotto-gruppi personalizzati (vedi capitolo relativo a virtual topologies).

Ogni operazione di comunicazione MPI avviene **all'interno di un communicator**: il rank di un processo è infatti relativo al communicator ("processo 2 in `MPI_COMM_WORLD`" può avere un rank diverso in un altro communicator).

## 3. Cos'è la comunicazione Point-to-Point

La comunicazione **point-to-point (P2P)** è la forma più elementare di scambio messaggi in MPI: **un solo processo mittente (sender)** invia dati a **un solo processo destinatario (receiver)**, in modo esplicito e diretto.

Si contrappone alle comunicazioni **collettive** (es. `MPI_Bcast`, `MPI_Reduce`), dove un'operazione coinvolge simultaneamente tutti i processi di un communicator. La P2P, invece, è sempre una relazione **1 a 1**: un sender e un receiver, identificati esplicitamente tramite il loro rank.

```
Processo 0                          Processo 1
    │                                   │
    │  MPI_Send(buf, count, type,       │
    │           dest=1, tag, comm)      │
    │ ──────────────────────────────►   │
    │                                   │  MPI_Recv(buf, count, type,
    │                                   │           src=0, tag, comm, &status)
```

Affinché la comunicazione avvenga con successo, è necessario che:

1. Il sender chiami `MPI_Send` specificando il rank del destinatario (`dest`).
2. Il receiver chiami `MPI_Recv` specificando il rank del mittente atteso (`source`), oppure `MPI_ANY_SOURCE` per accettare da chiunque.
3. Tag e datatype dei due lati **combacino** (vedi sezione 4.3).

Se anche una sola di queste condizioni non è soddisfatta, la comunicazione non avviene (o, peggio, il programma resta bloccato in attesa — vedi sezione 6 sui deadlock).

## 4. MPI_Send e MPI_Recv

Queste due funzioni sono il cuore della comunicazione P2P bloccante. Analizziamo ogni parametro nel dettaglio, perché è essenziale capirne il significato prima di scrivere codice.

### 4.1 MPI_Send — Prototipo commentato

```cpp
int MPI_Send(
    const void*  buf,      // puntatore al buffer di invio: l'indirizzo di
                            // memoria da cui MPI leggerà i dati da spedire
    int          count,    // quanti elementi inviare (NON byte, ma numero
                            // di elementi del tipo indicato in datatype)
    MPI_Datatype datatype, // tipo MPI degli elementi (es. MPI_INT, MPI_DOUBLE,
                            // MPI_CHAR...). Serve perché MPI deve sapere come
                            // interpretare/convertire i byte, specie tra
                            // architetture diverse
    int          dest,     // rank del processo destinatario, all'interno
                            // del communicator comm
    int          tag,      // un intero >= 0 scelto dal programmatore, usato
                            // come "etichetta" del messaggio (vedi 4.3)
    MPI_Comm     comm      // il communicator in cui avviene la comunicazione
                            // (tipicamente MPI_COMM_WORLD)
);
```

Esempio pratico: il processo 0 invia un singolo intero al processo 1, con tag 0.

```cpp
int valore = 42;
MPI_Send(&valore, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
//         │       │    │     │  │
//         │       │    │     │  └─ tag = 0
//         │       │    │     └──── destinatario: rank 1
//         │       │    └────────── tipo: un intero MPI_INT
//         │       └─────────────── count: invio 1 solo elemento
//         └─────────────────────── indirizzo del dato da inviare
```

### 4.2 MPI_Recv — Prototipo commentato

```cpp
int MPI_Recv(
    void*        buf,      // puntatore al buffer di ricezione: l'indirizzo
                            // di memoria in cui MPI scriverà i dati ricevuti
    int          count,    // capacità MASSIMA del buffer, in numero di
                            // elementi (il messaggio ricevuto può essere
                            // più corto, ma non più lungo)
    MPI_Datatype datatype, // deve corrispondere al datatype usato dal sender
    int          source,   // rank del mittente atteso, oppure MPI_ANY_SOURCE
                            // per accettare un messaggio da qualsiasi processo
    int          tag,      // tag atteso, oppure MPI_ANY_TAG per accettare
                            // qualsiasi tag
    MPI_Comm     comm,     // deve essere lo stesso communicator usato dal sender
    MPI_Status*  status    // struttura in cui MPI scrive informazioni sul
                            // messaggio ricevuto (mittente reale, tag reale,
                            // quanti elementi sono arrivati davvero)
);
```

Esempio corrispondente lato ricevente (processo 1):

```cpp
int valore_ricevuto;
MPI_Status status;
MPI_Recv(&valore_ricevuto, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
// Dopo questa chiamata, valore_ricevuto == 42
```

### 4.3 Come sender e receiver si "trovano": il matching dei messaggi

MPI abbina un `MPI_Send` al suo `MPI_Recv` corrispondente confrontando **tre chiavi**:

| Chiave      | Lato Send                | Lato Recv                              |
|-------------|---------------------------|------------------------------------------|
| communicator| `comm`                     | `comm` (deve essere lo stesso)           |
| rank        | `dest` (chi riceve)        | `source` (chi invia, o `MPI_ANY_SOURCE`) |
| tag         | `tag`                      | `tag` (o `MPI_ANY_TAG`)                  |

Il **tag** è semplicemente un numero intero a scelta del programmatore, utile per distinguere messaggi diversi tra la stessa coppia di processi. Ad esempio, se il processo 0 invia sia una temperatura sia una pressione al processo 1, può usare tag diversi (`tag=1` per la temperatura, `tag=2` per la pressione) così il receiver sa sempre cosa sta arrivando, anche se i due `MPI_Recv` non sono nello stesso ordine dei due `MPI_Send`.

## 5. Semantica Bloccante: cosa significa davvero "blocking"

Questo è il concetto più frainteso da chi inizia con MPI. "Bloccante" **non** significa "il messaggio è arrivato a destinazione". Significa qualcosa di più sottile e specifico per ciascuna delle due funzioni.

### 5.1 MPI_Send bloccante

> `MPI_Send` **ritorna (sblocca il processo chiamante) solo quando il buffer di invio (`buf`) può essere riutilizzato in sicurezza** — ad esempio per scriverci sopra un nuovo valore — **senza rischiare di corrompere il messaggio in transito**.

Questo **non implica** che il messaggio sia già stato ricevuto dal destinatario! A seconda dell'implementazione MPI e della dimensione del messaggio:

* Per messaggi **piccoli**, MPI può copiare i dati in un buffer di sistema interno e ritornare subito, anche se il receiver non ha ancora chiamato `MPI_Recv`.
* Per messaggi **grandi**, MPI potrebbe invece dover aspettare che il receiver sia pronto a ricevere direttamente nel suo buffer, per evitare di copiare grandi quantità di dati inutilmente.

Per questo motivo il comportamento di `MPI_Send` (se blocca "a lungo" o "quasi subito") **non va mai dato per scontato**: un programma corretto deve funzionare indipendentemente da questa scelta implementativa, e non deve mai assumere che "il send è tornato" equivalga a "il receiver ha già i dati".

### 5.2 MPI_Recv bloccante

> `MPI_Recv` **ritorna solo quando l'intero messaggio è stato trasferito completamente** nel buffer di ricezione.

Qui la semantica è più semplice e intuitiva: se stai aspettando un messaggio, il tuo processo resta fermo (bloccato) su quella riga di codice finché il messaggio non arriva per intero. Solo dopo il ritorno di `MPI_Recv` i dati nel buffer sono validi e utilizzabili.

### 5.3 Riassumendo con un'analogia

Immagina di dover spedire una lettera raccomandata:

* `MPI_Send` è come consegnare la lettera alle poste: una volta che l'impiegato l'ha presa in carico, puoi tornartene a casa (il tuo "buffer", cioè le tue mani, è di nuovo libero) — ma questo non significa che il destinatario l'abbia già letta.
* `MPI_Recv` è come aspettare fisicamente davanti alla propria cassetta della posta finché la lettera non arriva: non ti muovi finché non ce l'hai in mano.

## 6. Deadlock: la trappola più comune

Un **deadlock** si verifica quando due o più processi restano bloccati indefinitamente, ciascuno in attesa di un evento che non potrà mai verificarsi perché dipende da un altro processo bloccato a sua volta.

### 6.1 Lo scenario classico

Immagina due processi che vogliono scambiarsi un valore, e **entrambi** scrivono il codice così:

```cpp
// Processo 0                       // Processo 1
MPI_Send(&a, 1, MPI_INT, 1, 0, ...);  MPI_Send(&b, 1, MPI_INT, 0, 0, ...);
MPI_Recv(&a, 1, MPI_INT, 1, 0, ...);  MPI_Recv(&b, 1, MPI_INT, 0, 0, ...);
```

Se, come descritto nella sezione 5.1, l'implementazione di `MPI_Send` per questo messaggio decide di **aspettare** che il receiver corrispondente sia pronto (invece di bufferizzare internamente), succede questo:

* Il processo 0 esegue `MPI_Send`, e resta in attesa che il processo 1 chiami `MPI_Recv`.
* Il processo 1, però, esegue anch'esso `MPI_Send` per primo, e resta in attesa che il processo 0 chiami `MPI_Recv`.
* Nessuno dei due arriva mai alla propria `MPI_Recv`, perché entrambi sono bloccati sulla `MPI_Send` precedente.

Risultato: **il programma si blocca per sempre** (deadlock). Da notare che questo bug può *non* manifestarsi sempre: con messaggi piccoli, il buffering interno di MPI potrebbe "salvarti" facendo tornare subito la `MPI_Send`. Questo rende il deadlock un bug particolarmente insidioso, perché può apparire solo cambiando la dimensione del messaggio, il numero di processi, o persino la macchina su cui gira il programma.

### 6.2 Come evitarlo

Alcune strategie standard:

1. **Ordinare esplicitamente le operazioni**, ad esempio facendo sì che un processo invii prima di ricevere, e l'altro riceva prima di inviare:

   ```cpp
   // Processo 0                       // Processo 1
   MPI_Send(&a, 1, MPI_INT, 1, 0, ...);  MPI_Recv(&b, 1, MPI_INT, 0, 0, ...);
   MPI_Recv(&a, 1, MPI_INT, 1, 0, ...);  MPI_Send(&b, 1, MPI_INT, 0, 0, ...);
   ```

2. **Usare `MPI_Sendrecv`**, una funzione che combina invio e ricezione in un'unica chiamata gestita internamente da MPI in modo sicuro (non soggetta a questo tipo di deadlock).

3. **Usare comunicazioni non bloccanti** (`MPI_Isend`/`MPI_Irecv`), argomento che esula da questa guida mirata alla comunicazione bloccante, ma che è la soluzione più generale al problema.

L'esercizio 3 (sezione 10) mostra concretamente questo scenario e la sua correzione.

## 7. MPI_Barrier: sincronizzazione collettiva bloccante

Oltre alla comunicazione P2P, MPI fornisce anche operazioni bloccanti che **non** trasferiscono dati tra un mittente e un destinatario specifici, ma servono a **sincronizzare** un gruppo di processi. La più semplice è `MPI_Barrier`.

```cpp
MPI_Barrier(MPI_COMM_WORLD);
```

Una barriera obbliga tutti i processi del communicator a "aspettarsi a vicenda" in quel punto del codice:

* Un processo che raggiunge la barriera si ferma.
* La chiamata ritorna **solo quando tutti** i processi del communicator hanno raggiunto la stessa barriera.
* Nessun dato viene scambiato tra coppie specifiche di sender/receiver: è una pura sincronizzazione temporale.

Per questo motivo `MPI_Barrier` è classificata come **operazione collettiva di sincronizzazione bloccante**, e non come una primitiva di comunicazione point-to-point: non c'è un "mittente" e un "destinatario", ma l'intero gruppo che si sincronizza insieme. È utile ad esempio prima di misurare un tempo di esecuzione con `MPI_Wtime()`, per garantire che tutti i processi partano dalla stessa "linea di partenza".

## 8. MPI_Status: chi mi ha scritto, cosa, quanto

Quando un `MPI_Recv` accetta messaggi da **qualsiasi** mittente (`MPI_ANY_SOURCE`) o con **qualsiasi** tag (`MPI_ANY_TAG`), dopo la ricezione può essere necessario scoprire *chi ha effettivamente inviato* il messaggio e *con quale tag*. È qui che entra in gioco la struttura `MPI_Status`.

```cpp
MPI_Status status;
MPI_Recv(buf, count, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

// Dopo la MPI_Recv, la struttura status contiene:
status.MPI_SOURCE;  // il rank del mittente EFFETTIVO
status.MPI_TAG;     // il tag EFFETTIVO del messaggio ricevuto

// Il numero di elementi ricevuti può differire dal "count" massimo passato
// a MPI_Recv (che indicava solo la capacità del buffer): per saperlo con
// precisione si usa MPI_Get_count:
int elementi_ricevuti;
MPI_Get_count(&status, MPI_DOUBLE, &elementi_ricevuti);
```

Se queste informazioni non servono (ad esempio perché sender e tag sono già noti e fissi), si può evitare di allocare e popolare una `MPI_Status`, passando la costante speciale `MPI_STATUS_IGNORE` al posto dell'indirizzo della struttura:

```cpp
MPI_Recv(buf, count, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

Questo evita un piccolo overhead ed è considerata buona pratica quando lo status non serve.

## 9. Compilare ed eseguire un programma MPI

Per chi non ha ancora familiarità con il flusso di lavoro MPI: i programmi si compilano con un wrapper del compilatore C++ (`mpic++`, o `mpicxx`) che aggiunge automaticamente i percorsi di include e le librerie MPI, e si eseguono con `mpirun` (o `mpiexec`), specificando quanti processi lanciare con `-np`.

```bash
# Compilazione
mpic++ -o ex1_hello ex1_hello.cpp

# Esecuzione con 4 processi
mpirun -np 4 ./ex1_hello
```

Ogni riga stampata da un processo diverso è prefissata (in questa repository) con `[Process N]`, per rendere chiaro quale processo ha generato quale output — utile perché **l'ordine di interleaving dell'output tra processi diversi non è garantito** da MPI.

## 10. Esercizi guidati

### Esercizio 1 — Hello con Send/Recv (`ex1_hello.cpp`)

Il processo 0 invia un messaggio di saluto a tutti gli altri processi, uno per uno (un `MPI_Send` per ciascun destinatario). Ogni altro processo esegue un singolo `MPI_Recv` per ricevere il proprio messaggio. Questo esercizio introduce lo schema strutturale più comune nei programmi MPI:

```cpp
if (rank == 0) {
    // logica del mittente: un ciclo di MPI_Send verso tutti gli altri rank
} else {
    // logica del ricevente: un solo MPI_Recv dal processo 0
}
```

**Obiettivo didattico:** capire come un unico eseguibile SPMD assuma ruoli diversi a seconda del rank, e come un ciclo `for` lato sender si accoppi a chiamate singole lato receiver.

### Esercizio 2 — Ping-Pong (`ex2_pingpong.cpp`)

I processi 0 e 1 si scambiano ripetutamente un valore intero: il processo 0 lo invia al processo 1, che lo incrementa e lo rimanda indietro, e così via per un numero fissato di iterazioni. Il tempo totale viene misurato con `MPI_Wtime()`, la funzione MPI per la misurazione ad alta precisione del tempo trascorso (in secondi, come `double`).

**Obiettivo didattico:** introdurre la misurazione delle prestazioni di comunicazione e il concetto di **latenza** di rete: dato che in un ping-pong il messaggio percorre l'andata e il ritorno (Round-Trip Time, RTT), la latenza di un singolo invio si stima dividendo il tempo medio per iterazione per 2.

### Esercizio 3 — Deadlock e come evitarlo (`ex3_deadlock.cpp`)

Riproduce concretamente lo scenario descritto nella sezione 6: due processi che eseguono entrambi `MPI_Send` prima di `MPI_Recv`, mostrando (o simulando, a seconda della dimensione del messaggio scelta) il rischio di stallo, seguito dalla versione corretta con l'ordine invertito su uno dei due lati.

**Obiettivo didattico:** riconoscere sul campo un pattern di deadlock e interiorizzare le strategie di prevenzione della sezione 6.2.

### Esercizio 4 — Comunicazione ad anello (`ex4_ring.cpp`)

Ogni processo invia un "token" (un intero) al proprio vicino successivo nell'anello, calcolato come:

```text
rank_destinatario = (rank + 1) % size
```

Il processo `0` inizia inviando il token al processo `1`, ciascun processo intermedio lo riceve, lo incrementa, e lo inoltra al successivo; il token compie un giro completo e torna al processo `0`.

**Obiettivo didattico:** applicare l'aritmetica modulare per costruire topologie di comunicazione (qui, un anello) e osservare una sequenza di comunicazioni P2P concatenate, propedeutica ai pattern collettivi più avanzati (es. `MPI_Allreduce` ad anello) trattati in guide successive.

## 11. Output atteso e come interpretarlo

### ex1_hello (eseguito con `-np 4`)

```text
[Process 0] Greeting sent to 3 processes.
[Process 1] Message received from 0: "Hello from process 0!"
[Process 2] Message received from 0: "Hello from process 0!"
[Process 3] Message received from 0: "Hello from process 0!"
```

Il processo 0 conferma di aver inviato 3 messaggi (uno per ciascuno degli altri rank); ognuno degli altri processi conferma di aver ricevuto il proprio messaggio, riportando esplicitamente da chi (`0`) proviene.

### ex2_pingpong (eseguito con `-np 2`)

```text
[Ping-Pong] Iteration 0: value = 1 (sent from 0 to 1)
[Ping-Pong] Iteration 1: value = 2 (sent from 1 to 0)
...
Total time for 100 ping-pongs: 0.00312 seconds
Estimated latency (RTT/2): 15.6 microseconds
```

Si nota come il mittente si alterni ad ogni iterazione (0→1, poi 1→0): questo è il comportamento atteso di un ping-pong. Il tempo totale e la latenza stimata dipendono fortemente dall'hardware e dall'implementazione MPI usata: valori di questo ordine di grandezza (decine di microsecondi) sono tipici per comunicazioni locali (stesso nodo).

### ex4_ring (eseguito con `-np 4`)

```text
Process 0 sends token=0 to process 1
Process 1 receives 0, increments it to 1, sends it to process 2
Process 2 receives 1, increments it to 2, sends it to process 3
Process 3 receives 2, increments it to 3, sends it to process 0
Process 0 receives final token = 3 ✓
```

Il token parte da 0 e viene incrementato di 1 ad ogni "salto": dopo un giro completo su 4 processi, il valore finale ricevuto dal processo 0 è `3` (cioè `size - 1`), a conferma che il token ha attraversato correttamente tutti gli altri processi esattamente una volta.

## 12. Errori comuni e come evitarli

| Errore | Causa tipica | Come evitarlo |
|---|---|---|
| Il programma resta bloccato indefinitamente | Deadlock: `MPI_Send`/`MPI_Recv` non correttamente ordinati tra i processi (sezione 6) | Verificare che per ogni `MPI_Send` esista, in un ordine compatibile, un `MPI_Recv` corrispondente pronto ad accoglierlo |
| Dati ricevuti sembrano "spazzatura" o troncati | `count` del receiver troppo piccolo rispetto ai dati inviati, oppure `datatype` non corrispondente tra send e recv | Assicurarsi che `datatype` combaci esattamente e che il buffer di ricezione sia sufficientemente grande; verificare con `MPI_Get_count` quanti elementi sono realmente arrivati |
| Un `MPI_Recv` non riceve mai nulla | `source` o `tag` non corrispondono a quelli usati dal sender | Ricontrollare che rank e tag combacino, oppure usare `MPI_ANY_SOURCE`/`MPI_ANY_TAG` in fase di debug per isolare il problema |
| Output dei processi appare "mescolato" o in ordine imprevedibile | L'ordine di stampa tra processi diversi non è garantito da MPI | Non fare affidamento sull'ordine di stampa per la correttezza logica del programma; usare `MPI_Barrier` solo per sincronizzare, non per "ordinare" l'output in modo affidabile |
| Il programma funziona con pochi processi/dati piccoli ma si blocca con dati grandi | Comportamento di buffering interno di `MPI_Send` (sezione 5.1): con messaggi piccoli MPI bufferizza e ritorna subito, mascherando un deadlock latente | Non scrivere mai codice che assume implicitamente il buffering di `MPI_Send`; applicare sempre le strategie della sezione 6.2 |

---

**Prossimi passi consigliati:** una volta padroneggiata la comunicazione bloccante, il passo successivo naturale è lo studio della comunicazione **non bloccante** (`MPI_Isend`, `MPI_Irecv`, `MPI_Wait`), che permette di sovrapporre comunicazione e calcolo evitando molti dei rischi di deadlock descritti in questa guida.
