# 01a — Blocking Point-to-Point Communication

## Theory

Point-to-point (P2P) communication is the most basic form of message exchange in MPI: a **sender** process transmits data to a specific **receiver** process.

### MPI_Send and MPI_Recv

```text
Process 0                          Process 1
    │                                  │
    │  MPI_Send(buf, count, type,      │
    │           dest=1, tag, comm)     │
    │ ─────────────────────────────►   │
    │                                  │  MPI_Recv(buf, count, type,
    │                                  │           src=0, tag, comm, &status)
```

#### MPI_Send Prototype

```cpp
int MPI_Send(
    const void* buf,       // pointer to the send buffer
    int         count,     // number of elements
    MPI_Datatype datatype, // MPI datatype of the elements
    int         dest,      // destination rank
    int         tag,       // message tag (integer >= 0)
    MPI_Comm    comm       // communicator (usually MPI_COMM_WORLD)
);
```

#### MPI_Recv Prototype

```cpp
int MPI_Recv(
    void*        buf,      // buffer where data will be received
    int          count,    // maximum number of elements
    MPI_Datatype datatype,
    int          source,   // sender rank (or MPI_ANY_SOURCE)
    int          tag,      // expected tag (or MPI_ANY_TAG)
    MPI_Comm     comm,
    MPI_Status*  status    // information about the received message
);
```

### Blocking Semantics

* `MPI_Send` blocks the sender until the send buffer can be safely reused (not necessarily until the receiver has processed the message).
* `MPI_Recv` blocks the receiver until the message has been completely transferred into the receive buffer.

> ⚠️ **Deadlock:** if all processes execute `MPI_Send` before posting a matching `MPI_Recv`, no process may be ready to receive, leading to a deadlock.

### MPI_Barrier: A Blocking Synchronization Operation

MPI also provides blocking operations that are not point-to-point communications.

```cpp
MPI_Barrier(MPI_COMM_WORLD);
```

A barrier forces all processes in a communicator to synchronize:

* A process entering the barrier waits.
* The call returns only when every process in the communicator has reached the same barrier.
* No data is exchanged between specific sender/receiver pairs.

For this reason, `MPI_Barrier` is classified as a **blocking collective synchronization operation**, not as a point-to-point communication primitive.

### MPI_Status

The `MPI_Status` structure contains information about the received message:

```cpp
MPI_Status status;

// After MPI_Recv:
status.MPI_SOURCE;  // actual sender rank
status.MPI_TAG;     // actual message tag

// Determine how many elements were received:
int count;
MPI_Get_count(&status, MPI_DOUBLE, &count);
```

If this information is not needed, `MPI_STATUS_IGNORE` can be used instead.

---

## Exercises

### Exercise 1 — Hello with Send/Recv (`ex1_hello.cpp`)

Process 0 sends a greeting message to every other process. This exercise introduces the basic

```cpp
if (rank == 0) { ... }
else { ... }
```

structure commonly used in MPI programs.

### Exercise 2 — Ping-Pong (`ex2_pingpong.cpp`)

Processes 0 and 1 repeatedly exchange a value. Measure the communication time using `MPI_Wtime()`.

### Exercise 3 — Deadlock and How to Avoid It (`ex3_deadlock.cpp`)

Demonstrates a classic deadlock scenario and shows how proper ordering of `MPI_Send` and `MPI_Recv` prevents it.

### Exercise 4 — Ring Communication (`ex4_ring.cpp`)

Each process sends a token to its neighbor in a ring:

```text
rank → (rank + 1) % size
```

### Exercise 5 — Barrier Synchronization (`ex5_barrier.cpp`)

Each process performs a task with a different execution time and then calls `MPI_Barrier()`. Observe how faster processes wait for slower ones before continuing.

---

## Expected Output

### ex1_hello

```text
[Process 0] Greeting sent to 3 processes.
[Process 1] Message received from 0: "Hello from process 0!"
[Process 2] Message received from 0: "Hello from process 0!"
[Process 3] Message received from 0: "Hello from process 0!"
```

### ex2_pingpong (2 processes)

```text
[Ping-Pong] Iteration 0: value = 1 (sent from 0 to 1)
[Ping-Pong] Iteration 1: value = 2 (sent from 1 to 0)
...
Total time for 100 ping-pongs: 0.00312 seconds
Estimated latency (RTT/2): 15.6 microseconds
```

### ex4_ring

```text
Process 0 sends token=0 to process 1
Process 1 receives 0, increments it to 1, sends it to process 2
Process 2 receives 1, increments it to 2, sends it to process 3
Process 3 receives 2, increments it to 3, sends it to process 0
Process 0 receives final token = 3 ✓
```

### ex5_barrier

```text
Process 0 reached the barrier
Process 1 reached the barrier
Process 2 reached the barrier
Process 3 reached the barrier

All processes synchronized successfully.
```
