# 01a — Comunicazione Point-to-Point Bloccante

## Teoria

La comunicazione **point-to-point** (P2P) è la forma più elementare di scambio di messaggi in MPI: un processo **mittente** invia dati a un processo **destinatario** specifico.

### MPI_Send e MPI_Recv

```
Processo 0                        Processo 1
    │                                  │
    │  MPI_Send(buf, count, type,      │
    │           dest=1, tag, comm)     │
    │ ─────────────────────────────►  │
    │                                  │  MPI_Recv(buf, count, type,
    │                                  │           src=0, tag, comm, &status)
```

#### Firma di MPI_Send
```cpp
int MPI_Send(
    const void* buf,      // puntatore al buffer da inviare
    int         count,    // numero di elementi
    MPI_Datatype datatype,// tipo MPI degli elementi
    int         dest,     // rank del destinatario
    int         tag,      // etichetta del messaggio (intero >= 0)
    MPI_Comm    comm      // communicator (di solito MPI_COMM_WORLD)
);
```

#### Firma di MPI_Recv
```cpp
int MPI_Recv(
    void*        buf,     // buffer dove ricevere i dati
    int          count,   // numero massimo di elementi
    MPI_Datatype datatype,
    int          source,  // rank del mittente (o MPI_ANY_SOURCE)
    int          tag,     // tag atteso (o MPI_ANY_TAG)
    MPI_Comm     comm,
    MPI_Status*  status   // informazioni sul messaggio ricevuto
);
```

### Semantica "bloccante"

- `MPI_Send` **blocca** il mittente finché il buffer può essere riutilizzato in sicurezza (non necessariamente finché il destinatario ha ricevuto).
- `MPI_Recv` **blocca** il destinatario finché il messaggio è stato completamente ricevuto nel buffer.

> ⚠️ **Deadlock**: se tutti i processi eseguono `MPI_Send` prima di `MPI_Recv`, nessuno è pronto a ricevere → stallo garantito.

### MPI_Status

La struttura `MPI_Status` contiene informazioni sul messaggio ricevuto:
```cpp
MPI_Status status;
// Dopo MPI_Recv:
status.MPI_SOURCE  // rank effettivo del mittente
status.MPI_TAG     // tag effettivo del messaggio
// Per sapere quanti elementi sono stati ricevuti:
int count;
MPI_Get_count(&status, MPI_DOUBLE, &count);
```
Se non ti servono queste info, puoi passare `MPI_STATUS_IGNORE`.

---

## Esercizi

### Esercizio 1 — Hello con Send/Recv (`ex1_hello.cpp`)
Il processo 0 invia un saluto a tutti gli altri. Introduce la struttura base `if (rank == 0) ... else ...`.

### Esercizio 2 — Ping-Pong (`ex2_pingpong.cpp`)
Processi 0 e 1 si scambiano un valore alternandosi. Misura il tempo con `MPI_Wtime()`.

### Esercizio 3 — Deadlock e come evitarlo (`ex3_deadlock.cpp`)
Mostra il deadlock classico e la sua correzione con l'ordinamento Send/Recv.

### Esercizio 4 — Ring Communication (`ex4_ring.cpp`)
Ogni processo invia un token al successivo in un anello: `rank → (rank+1) % size`.

---

## Output Atteso

### ex1_hello
```
[Processo 0] Ho inviato il saluto a 3 processi.
[Processo 1] Messaggio ricevuto da 0: "Ciao dal processo 0!"
[Processo 2] Messaggio ricevuto da 0: "Ciao dal processo 0!"
[Processo 3] Messaggio ricevuto da 0: "Ciao dal processo 0!"
```

### ex2_pingpong (con 2 processi)
```
[Ping-Pong] Iterazione 0: valore = 1 (inviato da 0 a 1)
[Ping-Pong] Iterazione 1: valore = 2 (inviato da 1 a 0)
...
Tempo totale per 100 ping-pong: 0.00312 secondi
Latenza stimata (RTT/2): 15.6 microsecondi
```

### ex4_ring
```
Processo 0 invia token=0 al processo 1
Processo 1 riceve 0, incrementa a 1, invia al processo 2
Processo 2 riceve 1, incrementa a 2, invia al processo 3
Processo 3 riceve 2, incrementa a 3, invia al processo 0
Processo 0 riceve token finale = 3 ✓
```
