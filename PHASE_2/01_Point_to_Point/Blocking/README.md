# 01a — Blocking Point-to-Point Communication in MPI

---

## Table of Contents

1. [What MPI is and why it exists](#1-what-mpi-is-and-why-it-exists)
2. [Fundamental concepts: process, rank, communicator](#2-fundamental-concepts-process-rank-communicator)
3. [What Point-to-Point communication is](#3-what-point-to-point-communication-is)
4. [MPI_Send and MPI_Recv](#4-mpi_send-and-mpi_recv)
5. [Blocking semantics: what "blocking" really means](#5-blocking-semantics-what-blocking-really-means)
6. [Deadlock: the most common trap](#6-deadlock-the-most-common-trap)
7. [MPI_Barrier: blocking collective synchronization](#7-mpi_barrier-blocking-collective-synchronization)
8. [MPI_Status: who wrote to me, what, how much](#8-mpi_status-who-wrote-to-me-what-how-much)
9. [Guided exercises](#9-guided-exercises)
10. [Expected output and how to interpret it](#10-expected-output-and-how-to-interpret-it)
11. [Common mistakes and how to avoid them](#11-common-mistakes-and-how-to-avoid-them)

---

## 1. What MPI is and why it exists

**MPI (Message Passing Interface)** is a standard (not a language, not a specific library) that defines how different processes, running on different cores or machines, can **exchange data** to cooperate in solving a problem.

MPI's programming model is called **message passing**: unlike multithreaded programming (e.g. OpenMP), where threads share the same memory, in MPI **every process has its own private memory**. If process A wants process B to know a value, it must *explicitly send it* through a message: there is no shared memory by default.

This model is the foundation of distributed computing on HPC clusters, where hundreds or thousands of nodes, each with its own RAM, must cooperate.

```
Node 0 (own RAM)             Node 1 (own RAM)
┌─────────────────┐         ┌─────────────────┐
│  Process 0       │         │  Process 1       │
│  int x = 42;     │  msg    │  int x = ???;    │
│                  │ ──────► │  MPI_Recv(&x,..) │
└─────────────────┘         └─────────────────┘
```

## 2. Fundamental concepts: process, rank, communicator

Before understanding `MPI_Send`/`MPI_Recv`, three key concepts are needed.

### 2.1 Process

When you launch an MPI program with, say, 4 processes, the operating system creates **4 independent instances** of the same executable, each with its own memory, its own stack, its own variables. They are not threads: they are processes in every respect, often distributed across different physical cores or nodes.

All 4 execute **the same source code** (SPMD model: *Single Program, Multiple Data*), but behave differently depending on their own *rank* — see below.

### 2.2 Rank

The **rank** is a unique integer (from `0` to `size-1`) that identifies a process within a group of processes. It is the equivalent of an "address" or "name" for the process, used to decide who sends to whom.

```cpp
int rank, size;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // "who am I?" -> my rank
MPI_Comm_size(MPI_COMM_WORLD, &size);  // "how many of us are there in total?"
```

Thanks to the rank, a single source file can make processes behave differently:

```cpp
if (rank == 0) {
    // only process 0 executes this block
} else {
    // all others execute this other one
}
```

### 2.3 Communicator

A **communicator** is a group of processes that can communicate with each other. `MPI_COMM_WORLD` is the "default" communicator, automatically created at startup, and contains **all** the processes launched by the program. In general `MPI_COMM_WORLD` is used frequently, but it is useful to know that MPI allows custom sub-groups to be created (see the chapter on virtual topologies).

Every MPI communication operation takes place **within a communicator**: a process's rank is in fact relative to the communicator ("process 2 in `MPI_COMM_WORLD`" may have a different rank in another communicator).

## 3. What Point-to-Point communication is

**Point-to-point (P2P)** communication is the most elementary form of message exchange in MPI: **a single sender process** sends data to **a single receiver process**, explicitly and directly.

It stands in contrast to **collective** communications (e.g. `MPI_Bcast`, `MPI_Reduce`), where an operation simultaneously involves all the processes of a communicator. P2P, on the other hand, is always a **1-to-1** relationship: a sender and a receiver, explicitly identified by their rank.

```
Process 0                            Process 1
    │                                   │
    │  MPI_Send(buf, count, type,       │
    │           dest=1, tag, comm)      │
    │ ──────────────────────────────►   │
    │                                   │  MPI_Recv(buf, count, type,
    │                                   │           src=0, tag, comm, &status)
```

For the communication to succeed, the following must hold:

1. The sender calls `MPI_Send` specifying the rank of the recipient (`dest`).
2. The receiver calls `MPI_Recv` specifying the rank of the expected sender (`source`), or `MPI_ANY_SOURCE` to accept from anyone.
3. The tag and datatype on the two sides **match** (see section 4.3).

If even one of these conditions is not met, the communication does not take place (or, worse, the program remains blocked waiting — see section 6 on deadlocks).

## 4. MPI_Send and MPI_Recv

These two functions are the core of blocking P2P communication. Let's analyze each parameter in detail, since it is essential to understand its meaning before writing code.

### 4.1 MPI_Send — Annotated prototype

```cpp
int MPI_Send(
    const void*  buf,      // pointer to the send buffer: the memory address
                            // from which MPI will read the data to send
    int          count,    // how many elements to send (NOT bytes, but the
                            // number of elements of the type given in datatype)
    MPI_Datatype datatype, // MPI type of the elements (e.g. MPI_INT, MPI_DOUBLE,
                            // MPI_CHAR...). Needed because MPI must know how
                            // to interpret/convert the bytes, especially
                            // between different architectures
    int          dest,     // rank of the destination process, within
                            // the communicator comm
    int          tag,      // an integer >= 0 chosen by the programmer, used
                            // as a "label" for the message (see 4.3)
    MPI_Comm     comm      // the communicator in which the communication
                            // takes place (typically MPI_COMM_WORLD)
);
```

Practical example: process 0 sends a single integer to process 1, with tag 0.

```cpp
int valore = 42;
MPI_Send(&valore, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
//         │       │    │     │  │
//         │       │    │     │  └─ tag = 0
//         │       │    │     └──── destination: rank 1
//         │       │    └────────── type: an MPI_INT integer
//         │       └─────────────── count: sending only 1 element
//         └─────────────────────── address of the data to send
```

### 4.2 MPI_Recv — Annotated prototype

```cpp
int MPI_Recv(
    void*        buf,      // pointer to the receive buffer: the memory
                            // address into which MPI will write the received data
    int          count,    // MAXIMUM capacity of the buffer, in number of
                            // elements (the received message may be
                            // shorter, but not longer)
    MPI_Datatype datatype, // must match the datatype used by the sender
    int          source,   // rank of the expected sender, or MPI_ANY_SOURCE
                            // to accept a message from any process
    int          tag,      // expected tag, or MPI_ANY_TAG to accept
                            // any tag
    MPI_Comm     comm,     // must be the same communicator used by the sender
    MPI_Status*  status    // structure into which MPI writes information about
                            // the received message (actual sender, actual tag,
                            // how many elements actually arrived)
);
```

Corresponding example on the receiving side (process 1):

```cpp
int valore_ricevuto;
MPI_Status status;
MPI_Recv(&valore_ricevuto, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
// After this call, valore_ricevuto == 42
```

### 4.3 How sender and receiver "find" each other: message matching

MPI pairs an `MPI_Send` with its corresponding `MPI_Recv` by comparing **three keys**:

| Key         | Send side                  | Recv side                                |
|-------------|-----------------------------|--------------------------------------------|
| communicator| `comm`                      | `comm` (must be the same)                  |
| rank        | `dest` (who receives)       | `source` (who sends, or `MPI_ANY_SOURCE`)  |
| tag         | `tag`                       | `tag` (or `MPI_ANY_TAG`)                   |

The **tag** is simply an integer chosen by the programmer, useful for distinguishing different messages between the same pair of processes. For example, if process 0 sends both a temperature and a pressure reading to process 1, it can use different tags (`tag=1` for the temperature, `tag=2` for the pressure) so that the receiver always knows what is arriving, even if the two `MPI_Recv` calls are not in the same order as the two `MPI_Send` calls.

## 5. Blocking semantics: what "blocking" really means

The concept of "Blocking" does **not** mean "the message has arrived at its destination", but something more structured.

### 5.1 Blocking MPI_Send

> `MPI_Send` **returns (unblocks the calling process) only when the send buffer (`buf`) can be safely reused** — for example to write a new value into it — **without risking corruption of the message in transit**.

This **does not imply** that the message has already been received by the recipient! Depending on the MPI implementation and the message size:

* For **small** messages, MPI may copy the data into an internal system buffer and return immediately, even if the receiver has not yet called `MPI_Recv`.
* For **large** messages, MPI may instead need to wait until the receiver is ready to receive directly into its buffer, to avoid needlessly copying large amounts of data.

For this reason, the behavior of `MPI_Send` (whether it blocks "for a long time" or "almost immediately") should **never be taken for granted**: a correct program must work regardless of this implementation choice, and must never assume that "the send has returned" is equivalent to "the receiver already has the data".

### 5.2 Blocking MPI_Recv

> `MPI_Recv` **returns only when the entire message has been completely transferred** into the receive buffer.

Here the semantics are simpler and more intuitive: if you are waiting for a message, your process remains stopped (blocked) at that line of code until the message arrives in full. Only after `MPI_Recv` returns is the data in the buffer valid and usable.

### 5.3 Summarizing with an analogy

Imagine having to send a registered letter:

* `MPI_Send` is like handing the letter over at the post office: once the clerk has taken charge of it, you can go back home (your "buffer", i.e. your hands, is free again) — but this does not mean the recipient has already read it.
* `MPI_Recv` is like physically waiting in front of your own mailbox until the letter arrives: you don't move until you have it in hand.

## 6. Deadlock: the most common trap

A **deadlock** occurs when two or more processes remain blocked indefinitely, each waiting for an event that can never happen because it depends on another process that is itself blocked.

### 6.1 The classic scenario

Imagine two processes that want to exchange a value, and **both** write their code like this:

```cpp
// Process 0                         // Process 1
MPI_Send(&a, 1, MPI_INT, 1, 0, ...);  MPI_Send(&b, 1, MPI_INT, 0, 0, ...);
MPI_Recv(&a, 1, MPI_INT, 1, 0, ...);  MPI_Recv(&b, 1, MPI_INT, 0, 0, ...);
```

If, as described in section 5.1, the implementation of `MPI_Send` for this message decides to **wait** for the corresponding receiver to be ready (instead of buffering internally), the following happens:

* Process 0 executes `MPI_Send`, and waits for process 1 to call `MPI_Recv`.
* Process 1, however, also executes `MPI_Send` first, and waits for process 0 to call `MPI_Recv`.
* Neither of the two ever reaches its own `MPI_Recv`, because both are blocked on the preceding `MPI_Send`.

Result: **the program hangs forever** (deadlock). Note that this bug may *not* always manifest: with small messages, MPI's internal buffering might "save you" by making `MPI_Send` return immediately. This makes deadlock a particularly insidious bug, since it can appear only when the message size, the number of processes, or even the machine the program runs on changes.

### 6.2 How to avoid it

Some standard strategies:

1. **Explicitly ordering the operations**, for example by having one process send before receiving, and the other receive before sending:

   ```cpp
   // Process 0                         // Process 1
   MPI_Send(&a, 1, MPI_INT, 1, 0, ...);  MPI_Recv(&b, 1, MPI_INT, 0, 0, ...);
   MPI_Recv(&a, 1, MPI_INT, 1, 0, ...);  MPI_Send(&b, 1, MPI_INT, 0, 0, ...);
   ```

2. **Using `MPI_Sendrecv`**, a function that combines send and receive into a single call handled safely and internally by MPI (not subject to this type of deadlock), explained exhaustively in chapter 3b.

3. **Using non-blocking communications** (`MPI_Isend`/`MPI_Irecv`), the topic of the next chapter.

Exercise 3 concretely shows this scenario and its correction.

## 7. MPI_Barrier: blocking collective synchronization

Besides P2P communication, MPI also provides blocking operations that **do not** transfer data between a specific sender and receiver, but instead serve to **synchronize** a group of processes. The simplest one is `MPI_Barrier`.

```cpp
MPI_Barrier(MPI_COMM_WORLD);
```

A barrier forces all the processes in the communicator to "wait for each other" at that point in the code:

* A process that reaches the barrier stops.
* The call returns **only when all** the processes in the communicator have reached the same barrier.
* No data is exchanged between specific sender/receiver pairs: it is a purely temporal synchronization.

For this reason `MPI_Barrier` is classified as a **blocking collective synchronization operation**, not as a point-to-point communication primitive: there is no "sender" and "receiver", but the entire group synchronizing together. It is useful, for example, before measuring an execution time with `MPI_Wtime()`, to ensure that all processes start from the same "starting line".

## 8. MPI_Status: who wrote to me, what, how much

When an `MPI_Recv` accepts messages from **any** sender (`MPI_ANY_SOURCE`) or with **any** tag (`MPI_ANY_TAG`), after receiving it may be necessary to find out *who actually sent* the message and *with which tag*. This is where the `MPI_Status` structure comes into play.

```cpp
MPI_Status status;
MPI_Recv(buf, count, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

// After MPI_Recv, the status structure contains:
status.MPI_SOURCE;  // the rank of the ACTUAL sender
status.MPI_TAG;     // the ACTUAL tag of the received message

// The number of elements received may differ from the maximum "count" passed
// to MPI_Recv (which only indicated the buffer's capacity): to find out
// precisely, MPI_Get_count is used:
int elementi_ricevuti;
MPI_Get_count(&status, MPI_DOUBLE, &elementi_ricevuti);
```

If this information is not needed (for example because the sender and tag are already known and fixed), allocating and populating an `MPI_Status` can be avoided, by passing the special constant `MPI_STATUS_IGNORE` instead of the address of the structure:

```cpp
MPI_Recv(buf, count, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

This avoids a small amount of overhead and is considered good practice when the status is not needed.

## 9. Guided exercises

### Exercise 1 — Hello with Send/Recv (`ex1_hello.cpp`)

Process 0 sends a greeting message to every other process, one by one (one `MPI_Send` per recipient). Every other process performs a single `MPI_Recv` to receive its own message. This exercise introduces the most common structural pattern in MPI programs:

```cpp
if (rank == 0) {
    // sender logic: a loop of MPI_Send calls to all other ranks
} else {
    // receiver logic: a single MPI_Recv from process 0
}
```

**Objective:** understand how a single SPMD executable takes on different roles depending on the rank, and how a `for` loop on the sender side pairs up with single calls on the receiver side.

### Exercise 2 — Ping-Pong (`ex2_pingpong.cpp`)

Processes 0 and 1 repeatedly exchange an integer value: process 0 sends it to process 1, which increments it and sends it back, and so on for a fixed number of iterations. The total time is measured with `MPI_Wtime()`, the MPI function for high-precision measurement of elapsed time (in seconds, as a `double`).

**Objective:** introduce communication performance measurement and the concept of network **latency**: since in a ping-pong the message travels out and back (Round-Trip Time, RTT), the latency of a single send is estimated by dividing the average time per iteration by 2.

### Exercise 3 — Deadlock and how to avoid it (`ex3_deadlock.cpp`)

Concretely reproduces the scenario described in section 6: two processes both executing `MPI_Send` before `MPI_Recv`, showing (or simulating, depending on the chosen message size) the risk of a stall, followed by the corrected version with the order reversed on one of the two sides.

**Objective:** recognize a deadlock pattern hands-on and internalize the prevention strategies from section 6.2.

### Exercise 4 — Ring communication (`ex4_ring.cpp`)

Each process sends a "token" (an integer) to its next neighbor in the ring, computed as:

```text
destination_rank = (rank + 1) % size
```

Process `0` starts by sending the token to process `1`; each intermediate process receives it, increments it, and forwards it to the next one; the token completes a full loop and returns to process `0`.

**Objective:** apply modular arithmetic to build communication topologies (here, a ring) and observe a sequence of chained P2P communications, laying the groundwork for the more advanced collective patterns (e.g. ring-based `MPI_Allreduce`) covered in later chapters.

## 10. Expected output and how to interpret it

### ex1_hello (run with `-np 4`)

```text
[Process 0] Greeting sent to 3 processes.
[Process 1] Message received from 0: "Hello from process 0!"
[Process 2] Message received from 0: "Hello from process 0!"
[Process 3] Message received from 0: "Hello from process 0!"
```

Process 0 confirms that it sent 3 messages (one for each of the other ranks); each of the other processes confirms that it received its own message, explicitly reporting who it came from (`0`).

### ex2_pingpong (run with `-np 2`)

```text
[Ping-Pong] Iteration 0: value = 1 (sent from 0 to 1)
[Ping-Pong] Iteration 1: value = 2 (sent from 1 to 0)
...
Total time for 100 ping-pongs: 0.00312 seconds
Estimated latency (RTT/2): 15.6 microseconds
```

Notice how the sender alternates on each iteration (0→1, then 1→0): this is the expected behavior of a ping-pong. The total time and estimated latency depend heavily on the hardware and the MPI implementation used: values of this order of magnitude (tens of microseconds) are typical for local communications (same node).

### ex4_ring (run with `-np 4`)

```text
Process 0 sends token=0 to process 1
Process 1 receives 0, increments it to 1, sends it to process 2
Process 2 receives 1, increments it to 2, sends it to process 3
Process 3 receives 2, increments it to 3, sends it to process 0
Process 0 receives final token = 3 ✓
```

The token starts at 0 and is incremented by 1 at each "hop": after a full loop across 4 processes, the final value received by process 0 is `3` (i.e. `size - 1`), confirming that the token correctly passed through every other process exactly once.

## 11. Common mistakes and how to avoid them

| Mistake | Typical cause | How to avoid it |
|---|---|---|
| The program hangs indefinitely | Deadlock: `MPI_Send`/`MPI_Recv` not correctly ordered between processes (section 6) | Verify that for every `MPI_Send` there exists, in a compatible order, a matching `MPI_Recv` ready to receive it |
| Received data looks like "garbage" or truncated | The receiver's `count` is too small relative to the sent data, or the `datatype` does not match between send and recv | Make sure the `datatype` matches exactly and that the receive buffer is large enough; check with `MPI_Get_count` how many elements actually arrived |
| An `MPI_Recv` never receives anything | `source` or `tag` do not match those used by the sender | Double-check that rank and tag match, or use `MPI_ANY_SOURCE`/`MPI_ANY_TAG` during debugging to isolate the problem |
| Process output appears "mixed up" or in unpredictable order | The print order between different processes is not guaranteed by MPI | Do not rely on print order for the program's logical correctness; use `MPI_Barrier` only to synchronize, not to reliably "order" the output |
| The program works with few processes/small data but hangs with large data | Internal buffering behavior of `MPI_Send` (section 5.1): with small messages MPI buffers and returns immediately, masking a latent deadlock | Never write code that implicitly assumes `MPI_Send` buffering; always apply the strategies from section 6.2 |

---
