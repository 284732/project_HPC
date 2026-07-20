# 02 — Collective Communication in MPI
---

## Table of Contents

1. [Definition and properties of collective operations](#1-definition-and-properties-of-collective-operations)
2. [Overview of operations](#2-overview-of-operations)
3. [MPI_Bcast](#3-mpi_bcast)
4. [MPI_Scatter and MPI_Gather](#4-mpi_scatter-and-mpi_gather)
5. [MPI_Allgather](#5-mpi_allgather)
6. [MPI_Reduce and MPI_Allreduce](#6-mpi_reduce-and-mpi_allreduce)
7. [MPI_Alltoall](#7-mpi_alltoall)
8. [MPI_Barrier in the context of collectives](#8-mpi_barrier-in-the-context-of-collectives)
9. [MPI_IN_PLACE and buffer management](#9-mpi_in_place-and-buffer-management)
10. [Guided exercises](#10-guided-exercises)
11. [Expected output and how to interpret it](#11-expected-output-and-how-to-interpret-it)
12. [Common mistakes and how to avoid them](#12-common-mistakes-and-how-to-avoid-them)

---

## 1. Definition and properties of collective operations

A **collective** communication is an operation in which **all** the processes of a communicator participate simultaneously, as opposed to point-to-point operations (chapters 01a/01b), in which the communication involves exactly one sender/receiver pair explicitly identified by rank.

Collectives have no notion of "sender" and "recipient" in the P2P sense of the term: every process in the communicator invokes **the same MPI function**, with the same `comm`, providing (or requesting) data according to a communication pattern predefined by the semantics of the operation itself (broadcast, scatter, gather, reduce, etc.).

This entails a strict constraint that must always be respected:

> ⚠️ **Fundamental rule**: every process in the communicator must invoke **the same collective function**, with consistent parameters (same `datatype`, same `root` where applicable, matching counts). There is no notion of "optional participation": if even a single process in the communicator does not make the corresponding call, the program hangs indefinitely (the other processes remain waiting for a participant that will never arrive at that point in the code).

Unlike blocking P2P operations, where matching happens through explicit tags and ranks, in collectives the matching is implicit: it is the sequence of collective calls, in the same order on all processes, that determines which invocation corresponds to which. Calling different collectives (or the same collective with structurally different parameters, e.g. incompatible datatypes) on different processes at the same "logical point" of the program produces undefined behavior, often a deadlock or silent data corruption.

A common misconception should also be clarified: collective operations **do not necessarily imply `MPI_Barrier`-style global synchronization**. Some implementations may allow a collective to return on one process before all the others have completed it (this depends on the internal algorithm used and the communication topology). The only semantic guarantee is that, upon the call's return, the effect of the operation on the calling process is complete and correct (e.g. after an `MPI_Bcast`, the local buffer contains the broadcast data) — not that all other processes have already completed their own instance of the call.

## 2. Overview of operations

```
MPI_Bcast      → one process sends to all
MPI_Scatter    → one process distributes different portions to each process
MPI_Gather     → all processes send to the root, which collects them
MPI_Allgather  → all gather from all (equivalent to Gather + Bcast of the result)
MPI_Reduce     → all contribute, the root obtains the aggregated result
MPI_Allreduce  → all contribute, ALL obtain the aggregated result
MPI_Alltoall   → each process sends different data to every other process (transpose)
MPI_Barrier    → pure synchronization: no data exchanged, all wait for all
```

Conceptually these operations can be classified by the data-flow pattern they implement:

* **One-to-all** (distribution): `MPI_Bcast`, `MPI_Scatter`
* **All-to-one** (collection/aggregation): `MPI_Gather`, `MPI_Reduce`
* **All-to-all** (symmetric exchange): `MPI_Allgather`, `MPI_Allreduce`, `MPI_Alltoall`
* **Pure synchronization, no data transfer**: `MPI_Barrier`

## 3. MPI_Bcast

`MPI_Bcast` replicates the contents of a single process's buffer (`root`) onto all the other processes of the communicator.

```
Before: Proc 0 [X]  Proc 1 [?]  Proc 2 [?]  Proc 3 [?]
                        ↓
After:  Proc 0 [X]  Proc 1 [X]  Proc 2 [X]  Proc 3 [X]
```

```cpp
int MPI_Bcast(
    void*        buf,      // IN on the root, OUT on all others: the same
                            // buffer is used both to provide and to receive
                            // the data, depending on the caller's rank
    int          count,    // number of elements to distribute
    MPI_Datatype datatype,
    int          root,     // rank of the source process of the broadcast:
                            // must be IDENTICAL on all processes
    MPI_Comm     comm
);
```

Relevant implementation notes:

* The `buf` parameter has semantics that **depend on the caller's rank**: on the `root` process it is an **input** buffer (it already contains the data to be distributed), on all other processes it is an **output** buffer (it will be written by the call). There are not two separate `sendbuf`/`recvbuf` parameters as in other collectives, because the data is identical on every process at the end of the operation: conceptually a single buffer is enough, even though each process allocates it in its own private memory.
* The value of `root` must be consistent across all calling processes. A different `root` value on different processes produces undefined behavior, typically a deadlock or a broadcast from an unexpected source.

## 4. MPI_Scatter and MPI_Gather

These two operations are conceptually the inverse of one another.

```
Scatter:
  Root [A B C D]  →  Proc 0:[A]  Proc 1:[B]  Proc 2:[C]  Proc 3:[D]

Gather:
  Proc 0:[A]  Proc 1:[B]  Proc 2:[C]  Proc 3:[D]  →  Root [A B C D]
```

`MPI_Scatter` divides a contiguous buffer owned by the root into P contiguous blocks of equal size (`sendcount` elements each) and distributes one to each process, in rank order: block `i` (starting at offset `i * sendcount` in the root's buffer) goes to the process of rank `i`.

`MPI_Gather` performs the inverse operation: each process supplies a block of `sendcount` elements, and the root assembles them into a contiguous buffer, ordered by increasing rank (the block from process `i` ends up at offset `i * recvcount` in the root's buffer).

```cpp
int MPI_Scatter(
    const void*  sendbuf,   // significant ONLY on the root: complete source
                             // buffer, of size >= sendcount * P
    int          sendcount, // number of elements SENT to EACH process
                             // (not the total!). Significant only on the root
    MPI_Datatype sendtype,  // significant only on the root
    void*        recvbuf,   // local destination buffer, on EVERY process
                             // (including the root, which also receives its
                             // own portion through this parameter)
    int          recvcount, // number of elements received by THIS process
    MPI_Datatype recvtype,
    int          root,      // must be identical on all processes
    MPI_Comm     comm
);

int MPI_Gather(
    const void*  sendbuf,   // local buffer of EVERY process, containing
                             // its own contribution to send to the root
    int          sendcount, // number of elements sent by THIS process
    MPI_Datatype sendtype,
    void*        recvbuf,   // significant ONLY on the root: destination
                             // buffer, sized to hold recvcount * P elements
    int          recvcount, // number of elements received FROM EACH process
                             // (not the total). Significant only on the root
    MPI_Datatype recvtype,
    int          root,
    MPI_Comm     comm
);
```

A few technical points to keep in mind:

* Even though `sendbuf` in `MPI_Scatter` (and `recvbuf` in `MPI_Gather`) are parameters "significant only on the root", **every process, including non-root ones, must still pass a valid value** for those parameters in its own call (typically `nullptr` or some non-dereferenced pointer, depending on the language binding conventions used): the value will not be read/written on non-root processes, but the parameter must still be present in the C/C++ call signature.
* `sendcount` in `MPI_Scatter` and `recvcount` in `MPI_Gather` represent the amount of data **per single process**, not the aggregate total: a common mistake is to pass the total size of the vector instead of the size of the single portion (see section 13).
* If the total number of elements is not exactly divisible by the number of processes P, the "basic" `MPI_Scatter`/`MPI_Gather` do not automatically handle the remainder: it is necessary to resort to the `MPI_Scatterv`/`MPI_Gatherv` variants, which accept per-process count and displacement arrays, allowing blocks of non-uniform size. These variants are not covered in this introductory chapter.
* The root, in both operations, also participates as a "recipient/sender" of its own portion: in practice the root plays both the coordinator role and that of a normal participant, receiving/sending the portion corresponding to its own rank just like all the others.

## 5. MPI_Allgather

`MPI_Allgather` is semantically equivalent to an `MPI_Gather` followed by an `MPI_Bcast` of the assembled result: each process supplies a block, and **all** processes (not just a root) obtain the complete assembled vector.

```cpp
int MPI_Allgather(
    const void*  sendbuf,   // local buffer of EVERY process
    int          sendcount, // elements sent by THIS process
    MPI_Datatype sendtype,
    void*        recvbuf,   // destination buffer on EVERY process,
                             // sized for recvcount * P elements:
                             // ALL receive the complete vector
    int          recvcount, // elements received from EACH process
                             // (not the total)
    MPI_Datatype recvtype,
    MPI_Comm     comm       // no root parameter: it wouldn't make sense, since
                             // everyone receives the same result
);
```

From a computational/communication cost standpoint, `MPI_Allgather` is not simply "cheaper" than a Gather+Bcast performed in sequence: MPI implementations typically use dedicated algorithms (for example ring-based or recursive-doubling algorithms) that exploit the network topology to achieve higher aggregate throughput than two separate collective operations executed in sequence.

## 6. MPI_Reduce and MPI_Allreduce

Reduction operations apply an associative operator (and, in most cases, also a commutative one) to a set of values distributed across multiple processes, producing a single aggregated result.

```
Proc 0:[2]  Proc 1:[5]  Proc 2:[3]  Proc 3:[1]
                    ↓  MPI_SUM
                  Root:[11]
```

```cpp
int MPI_Reduce(
    const void*  sendbuf, // local input buffer on EVERY process
    void*        recvbuf, // significant ONLY on the root: will contain the
                           // aggregated result at the end of the call
    int          count,   // number of elements (the reduction is applied
                           // element by element, if count > 1)
    MPI_Datatype datatype,
    MPI_Op       op,       // reduction operator (see table below)
    int          root,
    MPI_Comm     comm
);

int MPI_Allreduce(
    const void*  sendbuf,
    void*        recvbuf, // significant on ALL processes: each one
                           // obtains the same aggregated result
    int          count,
    MPI_Datatype datatype,
    MPI_Op       op,
    MPI_Comm     comm      // no root parameter, for the same reason
                           // seen in MPI_Allgather
);
```

**Predefined reduction operators:**

| MPI constant | Operation |
|---|---|
| `MPI_SUM` | sum |
| `MPI_PROD` | product |
| `MPI_MAX` | maximum |
| `MPI_MIN` | minimum |
| `MPI_LAND` | logical AND |
| `MPI_LOR` | logical OR |
| `MPI_BAND` | bitwise AND |
| `MPI_BOR` | bitwise OR |
| `MPI_MAXLOC` | maximum + index of the process/position that holds it |
| `MPI_MINLOC` | minimum + index of the process/position that holds it |

Relevant technical notes:

* `MPI_MAXLOC` and `MPI_MINLOC` require specific "composite" datatypes (e.g. `MPI_DOUBLE_INT`, `MPI_FLOAT_INT`, value-index pairs), not a plain `MPI_DOUBLE`: the buffer must contain both the value and the associated index/rank, packed according to the layout expected by these special datatypes.
* It is possible to define **custom** reduction operators, not among the predefined ones, via `MPI_Op_create`, providing a user function that implements the combination logic. This falls outside the scope of this introductory chapter, but it is useful to know that the set of operators above is not exhaustive for every use case.
* The MPI standard requires that the operator provided be **associative**; **commutativity**, on the other hand, is not a strict requirement for the built-in operators listed (which are, in any case, all commutative), but it becomes relevant when defining custom operators: a non-commutative operator can produce different results depending on the order in which the MPI implementation combines the contributions of the various processes, an order that **is not specified by the standard** and can vary between implementations or different runs of the same program.
* `MPI_Reduce` delivers the result only to the root; if the result is needed by all processes (a common case, for example for a global convergence criterion in an iterative solver), using `MPI_Allreduce` directly is preferable, both for code clarity and for efficiency: a naïve "separate Reduce + Bcast" implementation requires two distinct collective passes, whereas `MPI_Allreduce` is typically implemented with a dedicated algorithm whose cost is lower than the sum of the costs of the two separate operations.

## 7. MPI_Alltoall

`MPI_Alltoall` is the most general collective operation among those presented: each process sends a **distinct** portion of data to each other process in the communicator (itself included), effectively implementing a distributed transpose of a logical P×P matrix of blocks, where row `i` represents the data owned by process `i` to be distributed, and column `j` represents the data that process `j` will receive from each of the others.

```cpp
int MPI_Alltoall(
    const void*  sendbuf,   // local buffer: contains P contiguous blocks of
                             // sendcount elements each; block j is destined
                             // for the process of rank j
    int          sendcount, // elements sent to EACH process (not the total)
    MPI_Datatype sendtype,
    void*        recvbuf,   // local destination buffer: the block
                             // received from process i ends up at offset
                             // i * recvcount
    int          recvcount, // elements received FROM EACH process
    MPI_Datatype recvtype,
    MPI_Comm     comm
);
```

Compared to the other collectives presented, `MPI_Alltoall` has no communication pattern reducible to a tree with a single point of origin or destination: each process is simultaneously the source and destination of P distinct messages (typically including a message "to itself", handled internally as a simple local copy with no network transit). The aggregate volume of data exchanged over the network scales with O(P²) total messages within the communicator, making `MPI_Alltoall` the potentially most expensive collective operation in terms of network traffic generated, especially as P grows.

This pattern underlies distributed algorithms that require a global reorganization of data among processes, the best-known case being the distributed matrix transposition required in multi-dimensional distributed FFT (Fast Fourier Transform) algorithms, where the data must be redistributed along a different axis from the one along which it was originally partitioned among the processes.

## 8. MPI_Barrier in the context of collectives

`MPI_Barrier` was already introduced in chapter 01a as a blocking synchronization primitive; here it should be correctly framed as a **special case** of a collective operation: it follows the same fundamental rule (every process in the communicator must invoke it) but, unlike all the other collectives presented in this chapter, it transfers no application data between processes — its sole effect is to guarantee that no process proceeds past the barrier until all the others have reached it.

It should be emphasized that `MPI_Barrier` is the **only** collective for which the MPI standard explicitly guarantees complete temporal synchronization among all participants upon the call's return. As discussed in section 1, for collectives that transfer data (Bcast, Scatter, Gather, Reduce, etc.) this guarantee does not hold in general: a process can, in principle, return from the collective call before other processes have completed their own.

## 9. MPI_IN_PLACE and buffer management

In several collectives (in particular `MPI_Gather`, `MPI_Allgather`, `MPI_Reduce`, `MPI_Allreduce`, `MPI_Scatter`), MPI provides the special constant `MPI_IN_PLACE`, which can be used in place of the pointer to `sendbuf` (or, depending on the operation, `recvbuf`) to indicate that the input and output buffers coincide, thereby avoiding a redundant allocation and copy.

Example with `MPI_Allreduce`, where each process wants to overwrite its own local buffer with the aggregated result instead of using a separate output buffer:

```cpp
double valore = calcola_valore_locale();

// Variant with separate buffers:
double risultato;
MPI_Allreduce(&valore, &risultato, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

// Variant with MPI_IN_PLACE: 'valore' acts as both input and output
MPI_Allreduce(MPI_IN_PLACE, &valore, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
```

The use of `MPI_IN_PLACE` is not merely a syntactic convenience: on large buffers it avoids an additional memory allocation and the resulting data copy, with a measurable benefit both in terms of memory footprint and execution time. The exact semantics of `MPI_IN_PLACE` vary slightly between the different collectives (for `MPI_Gather`/`MPI_Scatter` it must be passed in place of the parameter relating to the non-root vs. root process, with specific rules case by case): it is advisable to consult the documentation of the specific function before adopting it, since incorrect use typically leads to undefined behavior rather than a compilation error.

## 10. Guided exercises

### Exercise 1 — Bcast and PI computation (`ex1_bcast_pi.cpp`)

The root performs an `MPI_Bcast` of the computation parameters (in particular the total number of terms in the series), after which each worker independently computes its own portion of the Leibniz series summation for the approximation of π, and finally the partial contributions are aggregated with an `MPI_Reduce` (`MPI_SUM` operator) on the root.

**Objective:** apply in sequence two complementary collective patterns — distribution of parameters (`Bcast`) and aggregation of results (`Reduce`) — typical of nearly all embarrassingly parallel computing problems, in which processes work on independent portions of the domain and the partial results are combined at the end of the computation.

### Exercise 2 — Scatter/Gather: distributed sum (`ex2_scatter_gather.cpp`)

The root owns a large vector. It distributes it with `MPI_Scatter`, each worker locally processes its own portion (computing a partial sum, or an element-by-element transformation, depending on the specific implementation of the exercise), and the results are collected with `MPI_Gather` on the root for final reassembly.

**Objective:** correctly manage the relationship between the total data size, the number of processes, and the size of the local portion (`sendcount`/`recvcount` per process, not the total — see section 4), including the case where the vector size is chosen to be exactly divisible by the number of processes, to avoid the need for `MPI_Scatterv`/`MPI_Gatherv`.

### Exercise 3 — Allreduce: distributed norm (`ex3_allreduce.cpp`)

Each process owns a local vector (a portion of a logically larger vector, distributed among the processes). The global L2 norm of the complete vector is computed via a sum of local squares followed by an `MPI_Allreduce` with the `MPI_SUM` operator, so that every process obtains the final result (needed, for example, for a local convergence test identical across all processes, without having to redistribute the result with a separate `Bcast`).

**Objective:** recognize the cases in which `MPI_Allreduce` is the correct choice over `MPI_Reduce` — namely when the aggregated result is needed by **all** processes to continue the computation, not just by the root — and correctly apply the "local reduction (sum of squares) followed by global reduction (Allreduce)" pattern common in many distributed numerical algorithms (norm computation, distributed dot products, stopping criteria in iterative solvers).

### Exercise 4 — Alltoall: distributed transposition (`ex4_alltoall.cpp`)

Each process sends a distinct portion of data to each other process in the communicator via `MPI_Alltoall`, implementing a distributed transposition of the data.

**Objective:** understand the all-to-all communication pattern and its use as a fundamental building block for multi-dimensional distributed FFT algorithms (where data must be periodically redistributed along a different axis from the current partitioning axis), as well as gaining awareness of the impact on the network traffic generated (O(P²) messages within the communicator, section 7), relevant when choosing between this pattern and alternatives based on targeted P2P communications when the communication matrix is sparse (each process communicates only with a subset of the others, not with all of them).

## 11. Expected output and how to interpret it

### ex1_bcast_pi (run with `-np 4`)

```text
[Root] N_terms = 1000000, broadcast to all.
[Process 0] Computing terms 0..249999
[Process 1] Computing terms 250000..499999
...
Approximated PI = 3.14159265...  (error: 9.3e-7)
```

The total number of terms (`N_terms = 1000000`) is printed only once by the root, before the `MPI_Bcast`: after the broadcast, all processes hold the same value, but only the root gives explicit confirmation of it in the output (avoiding identical redundant prints from every process is good practice when the information does not vary between processes). Each process handles a contiguous, disjoint range of series indices (contiguous-block partitioning: process `i` handles the range `[i * N_terms/P, (i+1) * N_terms/P)`), consistent with the block-based parallelization strategy typical of this kind of computation. The final value of π is the result of the `MPI_Reduce` with `MPI_SUM` over the partial sums of each process; the reported absolute error (`9.3e-7`) is consistent with the known convergence rate of the Leibniz series for the chosen number of terms, and serves as a check of the implementation's numerical correctness, not just of the correctness of the communication.

## 12. Common mistakes and how to avoid them

| Mistake | Typical cause | How to avoid it |
|---|---|---|
| The program hangs indefinitely on a collective call | A process in the communicator does not reach that call (e.g. a collective inside an `if` block that excludes some ranks), or different processes invoke different collectives at the same logical point | Every process in the communicator must execute exactly the same sequence of collective calls; do not make the invocation of a collective conditional on a rank test, except when the condition is structurally identical across all processes (e.g. a loop that everyone executes the same number of times) |
| An inconsistent `root` value between processes produces unpredictable results or a deadlock | `root` computed dynamically in different ways on different processes (e.g. from a value not yet synchronized) | The value of `root` must be a constant known in advance, or derived from data already synchronized identically across all processes before the collective call |
| Exchanged data turns out truncated, overlapping, or in the wrong position after `Scatter`/`Gather`/`Alltoall` | `sendcount`/`recvcount` confused with the total buffer size instead of the size of the portion per single process (section 4) | Always verify that `count` represents the quantity **per process**: the total buffer size on the root must be `count * P`, not `count` |
| Destination buffer too small on `MPI_Gather`/`MPI_Allgather`/`MPI_Alltoall` | Allocation sized on the amount of data of a single process instead of on the aggregate total (`recvcount * P`) | Explicitly size the collection buffers taking into account the number of processes P, not just the size of the local contribution |
| The result of a reduction (`MPI_Reduce`/`MPI_Allreduce`) is slightly different between successive runs of the same program, with the same input | Floating-point operations that are not associative in practice (rounding): the order in which the MPI implementation combines the contributions of the individual processes is not specified by the standard and can vary between runs, internal algorithms, or number of processes used | Do not assume bit-for-bit reproducibility of floating-point reductions across different configurations or runs; if strict reproducibility is required, consider reduction implementations with an explicitly fixed combination order (outside the scope of this chapter) |
| Using `MPI_Reduce` when the result is actually needed on all processes | The algorithm's actual need is met with `Reduce` followed by a separate manual `Bcast`, instead of with `Allreduce` | Prefer `MPI_Allreduce` directly when all processes need the aggregated result: it is semantically equivalent but more efficient than the two separate calls (section 6) |
| Undefined behavior or crash when using `MPI_IN_PLACE` | Incorrect use of the parameter (wrong position, or a collective that does not support `MPI_IN_PLACE` in the assumed way) | Check the specific semantics of `MPI_IN_PLACE` for the collective in use before adopting it (section 10); when in doubt, prefer separate buffers as the default setting, optimizing only where the benefit is measurable |
