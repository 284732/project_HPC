# 01b — Non-Blocking Point-to-Point Communication in MPI
---

## Table of Contents

1. [Why non-blocking communication exists](#1-why-non-blocking-communication-exists)
2. [Blocking vs Non-Blocking: the difference in one sentence](#2-blocking-vs-non-blocking-the-difference-in-one-sentence)
3. [The general pattern: Start → Work → Wait](#3-the-general-pattern-start--work--wait)
4. [MPI_Isend and MPI_Irecv](#4-mpi_isend-and-mpi_irecv)
5. [MPI_Request: the handle of the ongoing operation](#5-mpi_request-the-handle-of-the-ongoing-operation)
6. [The golden rule: don't touch the buffer before Wait](#6-the-golden-rule-dont-touch-the-buffer-before-wait)
7. [The completion functions: Wait, Waitall, Waitany, Test](#7-the-completion-functions-wait-waitall-waitany-test)
8. [The advantage: computation-communication overlap](#8-the-advantage-computation-communication-overlap)
9. [Guided exercises](#9-guided-exercises)
10. [Expected output and how to interpret it](#10-expected-output-and-how-to-interpret-it)
11. [Common mistakes and how to avoid them](#11-common-mistakes-and-how-to-avoid-them)
12. [Summary comparison: when to use what](#12-summary-comparison-when-to-use-what)

---

## 1. Why non-blocking communication exists

In the previous chapter we noted that `MPI_Send` and `MPI_Recv` are **blocking**: the calling process remains stopped at that line of code until the operation can be considered complete (for send: buffer reusable; for recv: data fully arrived).

This behavior has an obvious cost: while a process is blocked waiting to complete a communication, **it cannot do anything else**. If a process needs to send data and then perform a computation that does not depend on that data, with blocking primitives it is nonetheless forced to wait for the communication to finish before it can start computing, wasting valuable time.

**Non-blocking** communication allows a process to start a send or receive operation without waiting for its completion. After the call, the process can proceed with other work. The actual progress of the communication depends on the MPI implementation and may occur during the execution of subsequent MPI calls or through internal mechanisms of the library. The completion of the operation is ultimately verified through dedicated primitives, such as MPI_Wait or MPI_Test.

## 2. Blocking vs Non-Blocking: the difference in one sentence

| | Blocking (`MPI_Send`/`MPI_Recv`) | Non-Blocking (`MPI_Isend`/`MPI_Irecv`) |
|---|---|---|
| When the call returns | Only once the operation is (partially) complete | Immediately, the operation has only been "started" |
| The process in the meantime | Is stopped, does nothing else | Is free to execute other code |
| When the data is guaranteed ready | When the function itself returns | Only after an explicit completion call (`MPI_Wait` and similar) |
| Main risk | Deadlock (section 6 of the Blocking chapter) | Accessing the buffer before completion (section 6 of this chapter) |

The initial "I" in `MPI_Isend`/`MPI_Irecv` stands for **Immediate**: the call returns immediately, without waiting for the operation to actually be completed.

## 3. The general pattern: Start → Work → Wait

Every non-blocking communication always follows this three-phase structure:

```
MPI_Isend / MPI_Irecv   →  start the operation (returns immediately)
         [useful work]  →  compute something while the data is in transit
MPI_Wait / MPI_Waitall  →  wait until the operation is actually completed
```

It is essential to understand that **starting** a non-blocking operation (phase 1) and **completing it** (phase 3) are two distinct, mandatory steps: for every `MPI_Isend`/`MPI_Irecv` started, there must sooner or later be a corresponding completion call. Omitting the completion is an error (see section 11).

```cpp
MPI_Request request;  // handle identifying the operation in progress

// Phase 1: (non-blocking) start of a send
MPI_Isend(buf, count, datatype, dest, tag, comm, &request);

// Phase 2: useful work, performed WHILE the communication proceeds
// ... computations that don't touch 'buf' ...

// Phase 3: explicit wait for completion
MPI_Wait(&request, MPI_STATUS_IGNORE);
```

## 4. MPI_Isend and MPI_Irecv

The prototypes of these two functions are almost identical to those of `MPI_Send`/`MPI_Recv` seen in the previous chapter, with **only two differences**: they no longer return an immediate completion status, and they require an additional parameter, `MPI_Request*`.

```cpp
int MPI_Isend(
    const void*  buf,      // send buffer: WARNING, must not be modified
                            // until the operation is completed (sec. 6)
    int          count,    // number of elements to send
    MPI_Datatype datatype, // MPI type of the elements (MPI_INT, MPI_DOUBLE...)
    int          dest,     // rank of the recipient
    int          tag,      // message label
    MPI_Comm     comm,     // communicator (e.g. MPI_COMM_WORLD)
    MPI_Request* request   // OUTPUT: handle identifying this specific
                            // operation, to be used later with MPI_Wait
);

int MPI_Irecv(
    void*        buf,      // receive buffer: WARNING, the data is not
                            // valid until the operation is completed
    int          count,    // maximum capacity of the buffer
    MPI_Datatype datatype, // must match the sender's datatype
    int          source,   // rank of the expected sender (or MPI_ANY_SOURCE)
    int          tag,      // expected tag (or MPI_ANY_TAG)
    MPI_Comm     comm,
    MPI_Request* request   // OUTPUT: handle of the receive operation
);
```

Note that `MPI_Irecv`, unlike `MPI_Recv`, **does not require** a pointer to `MPI_Status` as its last parameter: information about the actual sender/tag and the number of elements received is obtained by passing an `MPI_Status` to the subsequent completion call (`MPI_Wait`), not to `MPI_Irecv` itself — simply because at the time of the `MPI_Irecv` call the message has not yet arrived, so that information does not yet exist.

## 5. MPI_Request: the handle of the ongoing operation

`MPI_Request` is a type defined by the MPI standard, used to identify and manage a non-blocking communication operation. When a routine such as `MPI_Isend()` or `MPI_Irecv()` is invoked, the MPI library starts the communication operation and returns to the caller an object of type `MPI_Request`, which acts as a handle associated with that operation.

An `MPI_Request` object does not contain the message data nor the result of the communication; it represents solely the reference through which MPI can identify the operation that was started. Thanks to this handle, the process can subsequently check the status of the communication or wait for its completion using routines such as `MPI_Test()`, `MPI_Wait()`, and their related variants.

The fundamental characteristic of non-blocking communications is that the call to `MPI_Isend()` or `MPI_Irecv()` immediately returns control to the program without waiting for the actual completion of the data transfer. This makes it possible to overlap computation and communication, reducing the time spent waiting and potentially improving the efficiency of the parallel application.

When multiple non-blocking operations are started at the same time, it is common practice to store the corresponding handles in an array of `MPI_Request`, where each element identifies a specific communication operation. This structure makes it possible to efficiently manage the set of pending communications through functions such as `MPI_Waitall()`, `MPI_Testall()`, `MPI_Waitany()`, and `MPI_Testany()`.

```cpp
MPI_Request requests[3];
MPI_Isend(buf0, count, MPI_INT, dest0, tag, comm, &requests[0]);
MPI_Isend(buf1, count, MPI_INT, dest1, tag, comm, &requests[1]);
MPI_Isend(buf2, count, MPI_INT, dest2, tag, comm, &requests[2]);
// ... they will then all be waited on together with MPI_Waitall (section 7) ...
```

## 6. The golden rule: don't touch the buffer before Wait

> ⚠️ Fundamental rule
> Between the start of a non-blocking communication (`MPI_Isend` or `MPI_Irecv`) and its completion via `MPI_Wait`, `MPI_Waitall`, `MPI_Test`, or equivalent functions, the buffer involved in the communication **must not be used by the application**.
>
> Specifically:
>
> - the **send buffer** must not be modified;
> - the **receive buffer** must not be read or modified.
>
> This restriction remains in effect until the communication operation has been completed.

## Why is this rule fundamental?

Non-blocking communication primitives immediately return control to the program, making it possible to **overlap computation and communication** (*communication-computation overlap*). However, the actual completion of the data transfer may occur at a later time and depends on the implementation of the MPI library and the state of the communication.

As a consequence:

- If the program modifies the **send buffer** before the completion of the `MPI_Isend`, the MPI library may not yet have finished reading all the data from the buffer. The message sent could therefore contain data that has been modified or is inconsistent with what was intended to be transmitted.

- If the program reads or modifies the **receive buffer** before the completion of the `MPI_Irecv`, the contents of the buffer are not yet guaranteed to be valid according to the MPI standard. The data may not yet have been received, or may still be in the process of being transferred, making the buffer's contents **undefined**.

For this reason, the completion of the communication via `MPI_Wait`, `MPI_Waitall`, `MPI_Test`, or equivalent functions represents the point at which the application regains full availability of the buffer and can access it safely.

> **Practical rule:** after a call to `MPI_Isend` or `MPI_Irecv`, the buffer involved must be considered **temporarily unavailable** until the completion of the related MPI request.

```cpp
int valore = 10;
MPI_Request req;
MPI_Isend(&valore, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &req);

valore = 20;  // ✗ ERROR: 'valore' may have already been sent
              //   as 10, as 20, or in an undefined intermediate state.
              //   The result is not guaranteed and depends on
              //   implementation and timing details, exactly like a
              //   deadlock: it may "appear" to work by chance.

MPI_Wait(&req, MPI_STATUS_IGNORE);
valore = 20;  // ✓ CORRECT: the send is now completed, the buffer is free
```

This is the non-blocking-world equivalent of what deadlock represents in the blocking world: **the main risk to be aware of and always avoid**.

## 7. The completion functions: Wait, Waitall, Waitany, Test

Starting an operation is not enough: sooner or later its completion must be checked (or waited for). MPI provides several functions to do this, depending on the situation.

| Function | Behavior |
|---|---|
| `MPI_Wait(&req, &status)` | **Blocks** the calling process until the specific request `req` is completed. It is the "pointwise" equivalent of a blocking `MPI_Recv`, but applied to an already-started operation. |
| `MPI_Waitall(n, reqs[], statuses[])` | **Blocks** until **all** `n` requests in the array `reqs[]` are completed. Useful when many communications are started together and all you need to know is that they are *all* finished, without caring about the order. |
| `MPI_Waitany(n, reqs[], &idx, &status)` | **Blocks** until **at least one** of the `n` requests is completed, returning in `idx` the index of the one that finished. Useful for reacting to the first communication that finishes, processing results as they arrive instead of waiting for all of them together. |
| `MPI_Test(&req, &flag, &status)` | **Never blocks**: immediately checks whether `req` is already completed, setting `flag = 1` (true) or `flag = 0` (false), and returns immediately in both cases. Useful for *polling*: "check if it's ready, and if not keep doing other work, then check again later". |

Example with `MPI_Waitall`, used when starting several sends at the same time (as in Exercise 3):

```cpp
MPI_Request requests[3];
MPI_Status  statuses[3];

for (int i = 0; i < 3; i++) {
    MPI_Isend(&dati[i], 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD, &requests[i]);
}

// ... any useful work here, while the 3 sends proceed in the background ...

MPI_Waitall(3, requests, statuses);  // wait until ALL 3 are completed
```

Note that `MPI_STATUS_IGNORE` (seen in the previous chapter) has an array equivalent: `MPI_STATUSES_IGNORE`, to be used with `MPI_Waitall` when detailed statuses are not needed.

## 8. The advantage: computation-communication overlap

The main reason non-blocking communication is used is the ability to **overlap** the time spent communicating with the time spent computing, instead of adding them up sequentially.

### 8.1 Blocking case: everything in sequence

```text
Time →
────────────────────────────────────────────────────────
Blocking version:
|---- Send ----|---- Receive ----|---- Computation ----|
Total time ≈ Tsend + Trecv + Tcompute
```

With blocking primitives, sending, receiving, and computing happen one after another: the process is always busy with only one activity at a time, and the total time is simply the sum of the times of each phase.

### 8.2 Non-blocking case: communication and computation in parallel

```text
Non-blocking version:
| Isend/Irecv |
|------------- Computation -------------|
                                   | Wait |
Total time ≈ max(Tcommunication, Tcompute) + synchronization overhead
```

Here the communication operation is only *started*, and the process immediately moves on to performing useful computation while the data travels "in the background". The final wait time (`MPI_Wait`) is often much shorter than the total communication time, because **most of the communication has already taken place during the computation**.

### 8.3 Why it works: two activities, not one

The underlying idea is that communication (largely handled by the network hardware, relatively independently of the CPU) and computation (which instead actively occupies the CPU) are two **different** resources. If a process uses them one after the other (blocking), the total time is their sum. If instead it manages to have them proceed simultaneously (non-blocking), the total time tends toward the **maximum** of the two, not their sum:

```text
Blocking:      Tsend + Trecv + Tcompute
Non-blocking:  max(Tcommunication, Tcompute) + synchronization overhead
```

The benefit is greatest when communication and computation have **comparable time costs**: if the computation takes much longer than the communication, the latter is almost entirely "hidden" behind the computation; if instead the computation is negligible, the benefit of overlap is reduced, because there is almost nothing to overlap the communication with.

## 9. Guided exercises

### Exercise 1 — Basic Isend/Irecv (`ex1_nonblocking_basic.cpp`)

Reimplementation of the ping-pong exercise (already seen in chapter 01a, section 9, Exercise 2) using `MPI_Isend`/`MPI_Irecv` instead of the blocking versions.

**Objective:** directly compare, on the same problem, the structure of blocking and non-blocking code, to make tangible the difference described in section 2 of this chapter (same application logic, completely different waiting mechanism).

### Exercise 2 — Computation-communication overlap (`ex2_overlap.cpp`)

Each process starts a non-blocking send of its own data to a neighboring process and, while the send is in transit, computes a local sum over another set of data (which does not depend on the data being sent). The execution time is compared with the equivalent purely blocking version (send followed by computation).

**Objective:** concretely measure the advantage described in section 8: verify that the total time of the non-blocking version approaches `max(Tcommunication, Tcompute)` instead of their sum.

### Exercise 3 — Waitall with multiple communications (`ex3_waitall.cpp`)

The master process simultaneously starts N `MPI_Isend` operations (one per worker), then performs independent work, and finally waits for the completion of all communications with a single call to `MPI_Waitall`.

**Objective:** manage an array of `MPI_Request` for multiple simultaneous operations (section 5) and understand when `MPI_Waitall` is preferable to several separate sequential `MPI_Wait` calls (simpler to write, and it allows MPI to complete the requests in whatever order is most efficient, not necessarily the order in which they were started).

## 10. Expected output and how to interpret it

### ex3_waitall (run with `-np 4`)

```text
[Master] Starting 3 non-blocking Isend operations...
[Master] Doing other work while the data is in transit...
[Master] MPI_Waitall: all communications completed.
[Worker 1] Received: 100
[Worker 2] Received: 200
[Worker 3] Received: 300
```

Note the logical sequence of the master's print statements: first it announces the start of the 3 `MPI_Isend` calls (phase 1 of the pattern, section 3), then it prints that it is doing "other work" **before** having received confirmation that the communications are finished (phase 2, computation overlaps with communication), and only after the `MPI_Waitall` does it confirm that all 3 have finished (phase 3). The three workers, being independent processes, may print their own reception message in any order relative to the other workers (the relative order between different processes is not guaranteed, as already seen in part 01a), but each correctly receives the value intended for it (`100`, `200`, `300`).

## 11. Common mistakes and how to avoid them

| Mistake | Typical cause | How to avoid it |
|---|---|---|
| Sent or received data turns out wrong, corrupted, or "stale" | The buffer was read or written before the operation's completion (violation of the golden rule, section 6) | Never access the buffer between `MPI_Isend`/`MPI_Irecv` and the corresponding `MPI_Wait`/`MPI_Waitall`; if a new buffer is needed for new data while the previous communication is still in progress, use a different one |
| The program hangs (or crashes) on an `MPI_Wait` | The completion call for a started request was forgotten, or it is waiting on an `MPI_Request` that was never correctly initialized | For every `MPI_Isend`/`MPI_Irecv` started, there must always eventually be a corresponding `MPI_Wait` (or `MPI_Waitall`/`MPI_Waitany`/`MPI_Test` until completion) |
| Memory leak or undefined behavior over time | The `MPI_Request` objects are never completed, remaining "hanging" indefinitely | Always track all started requests (e.g. in an array) and make sure each one is completed before the program ends |
| `MPI_Test` seems to "never work" (`flag` always 0) | Completion is checked too early relative to actual network timing, or `flag == 0` is mistakenly interpreted as an error | `flag == 0` is a normal, expected outcome of `MPI_Test`: it simply means "not yet completed", not an error; polling should be repeated until `flag` becomes 1 |
| No performance gain observed between the blocking and non-blocking versions | The computation performed during the wait (phase 2) is too short relative to the communication time, or there is no independent computation to overlap | Verify that there is a real workload, independent of the data in transit, to execute between the start and the completion; overlap offers no benefit if there is nothing to overlap it with (section 8.3) |

## 12. Summary comparison: when to use what

* Use **blocking** communication (chapter 01a) when the program logic is simple, the waiting cost is not an issue, and the priority is code clarity.
* Use **non-blocking** communication (this chapter) when:
  * you have useful, independent computation to perform while the data travels;
  * you need to start many communications at once and want to wait for them together efficiently (`MPI_Waitall`);
  * you want to proactively avoid the deadlock risks described in chapter 01a, since `MPI_Isend`/`MPI_Irecv` never block waiting for a ready partner.

The price to pay for these advantages is more careful management of each communication's lifecycle: always remembering to complete every operation that was started.
