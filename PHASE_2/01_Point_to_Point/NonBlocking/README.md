# 01b — Non-Blocking Point-to-Point Communication

## Theory

**Non-blocking** operations return control immediately, allowing the process to continue working while the communication takes place in the background. Completion of the operation is checked at a later time.

### The General Pattern

```
MPI_Isend / MPI_Irecv   →  start the operation (returns immediately)
         [useful work]  →  compute something while data is in transit
MPI_Wait / MPI_Waitall  →  wait until the operation has completed
```

### MPI_Isend and MPI_Irecv

```cpp
MPI_Request request;  // handle identifying the ongoing operation

// Start a non-blocking send
MPI_Isend(buf, count, datatype, dest, tag, comm, &request);

// Start a non-blocking receive
MPI_Irecv(buf, count, datatype, source, tag, comm, &request);

// ... perform some computation ...

// Wait for completion
MPI_Wait(&request, MPI_STATUS_IGNORE);
```

> ⚠️ **Important**: do not access the buffer between `MPI_Isend`/`MPI_Irecv` and `MPI_Wait`! The buffer contents are "in use" by MPI.

### Completion Functions

| Function                                | Description                                                |
| --------------------------------------- | ---------------------------------------------------------- |
| `MPI_Wait(&req, &status)`               | Blocks until `req` is complete                             |
| `MPI_Waitall(n, reqs[], statuses[])`    | Waits until **all** `n` requests are complete              |
| `MPI_Waitany(n, reqs[], &idx, &status)` | Waits until **at least one** request is complete           |
| `MPI_Test(&req, &flag, &status)`        | Non-blocking: checks whether it is complete (`flag = 0/1`) |

### Advantage: Computation–Communication Overlap

```text
Time →
────────────────────────────────────────────────────────

Blocking version:
|---- Send ----|---- Receive ----|---- Computation ----|
Total execution time ≈ 3T

Non-blocking version:
| Isend/Irecv |
|---- Computation ----|
                    | Wait |
Total execution time ≈ T + communication overhead
```

In the blocking case, communication and computation occur one after the other.

In the non-blocking case, communication progresses in the background while useful computation is performed. The waiting time at the end is often much smaller than the communication time itself, because part of the communication has already completed during computation.

Therefore:

```text
Blocking:      Tsend + Trecv + Tcompute
Non-blocking:  max(Tcommunication, Tcompute)
               + synchronization overhead
```

When computation and communication have comparable costs, the overlap can significantly reduce the total execution time.


## Exercises

### Exercise 1 — Basic Isend/Irecv (`ex1_nonblocking_basic.cpp`)

Reimplementation of the ping-pong example using non-blocking operations.

### Exercise 2 — Compute-Communicate Overlap (`ex2_overlap.cpp`)

Compute a local sum while sending data to a neighbor. Compare execution times.

### Exercise 3 — Waitall with Multiple Communications (`ex3_waitall.cpp`)

The master launches N non-blocking sends simultaneously, then waits using `MPI_Waitall`.

---

## Expected Output

### ex3_waitall (with 4 processes)

```text
[Master] Starting 3 non-blocking Isend operations...
[Master] Doing other work while the data is in transit...
[Master] MPI_Waitall: all communications completed.
[Worker 1] Received: 100
[Worker 2] Received: 200
[Worker 3] Received: 300
```
