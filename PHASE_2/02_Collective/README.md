# 03 — Collective Communication

## Theory

Collective operations involve **all** processes in a communicator simultaneously. They are more efficient than equivalent P2P operations (they use tree-based algorithms, pipelines, etc.) and simplify the code.

> ⚠️ **Fundamental rule**: every process in the communicator **must** call the same collective function. There is no sender and receiver: everyone participates.

---

### Overview of Operations

```
MPI_Bcast      → 1 process sends to all
MPI_Scatter    → 1 process distributes parts to each process
MPI_Gather     → all processes send to the root process, which collects them
MPI_Allgather  → all processes collect from all
MPI_Reduce     → all contribute, root obtains the result
MPI_Allreduce  → all contribute, ALL obtain the result
MPI_Alltoall   → each process sends different data to every other process
MPI_Barrier    → synchronization: everyone waits until everyone arrives
```

---

### MPI_Bcast

```
Before: Proc 0 [X]  Proc 1 [?]  Proc 2 [?]  Proc 3 [?]
                  ↓
After:  Proc 0 [X]  Proc 1 [X]  Proc 2 [X]  Proc 3 [X]
```

```cpp
MPI_Bcast(buf, count, datatype, root, comm);
// root: the process that initially owns the data (the broadcast "sender")
// all other processes receive the data into buf
```

### MPI_Scatter / MPI_Gather

```
Scatter:
  Root [A B C D]  →  Proc 0:[A]  Proc 1:[B]  Proc 2:[C]  Proc 3:[D]

Gather:
  Proc 0:[A]  Proc 1:[B]  Proc 2:[C]  Proc 3:[D]  →  Root [A B C D]
```

```cpp
MPI_Scatter(sendbuf, sendcount, sendtype,
            recvbuf, recvcount, recvtype, root, comm);

MPI_Gather(sendbuf, sendcount, sendtype,
           recvbuf, recvcount, recvtype, root, comm);
```

### MPI_Reduce

```
Proc 0:[2]  Proc 1:[5]  Proc 2:[3]  Proc 3:[1]
              ↓  MPI_SUM
            Root:[11]
```

**Available reduction operations:**

| MPI Constant | Operation   |
| ------------ | ----------- |
| `MPI_SUM`    | sum         |
| `MPI_PROD`   | product     |
| `MPI_MAX`    | maximum     |
| `MPI_MIN`    | minimum     |
| `MPI_LAND`   | logical AND |
| `MPI_LOR`    | logical OR  |
| `MPI_MAXLOC` | max + index |
| `MPI_MINLOC` | min + index |

```cpp
MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
// Allreduce: all processes obtain the result, not only the root
```

---

## Exercises

### Exercise 1 — Bcast and PI Computation (`ex1_bcast_pi.cpp`)

The root broadcasts the parameters, each worker computes a portion of PI using the Leibniz series, and then a final Reduce is performed.

### Exercise 2 — Scatter/Gather: Distributed Sum (`ex2_scatter_gather.cpp`)

The root owns a large vector. It scatters it, each worker processes its own portion, and then the results are gathered.

### Exercise 3 — Allreduce: Distributed Norm (`ex3_allreduce.cpp`)

Each process owns a local vector. Compute the global L2 norm using Allreduce (all processes obtain the result).

### Exercise 4 — Alltoall: Distributed Transposition (`ex4_alltoall.cpp`)

Each process sends different data to every other process: the basis of distributed FFT.

---

## Expected Output

### ex1_bcast_pi (4 processes)

```
[Root] N_terms = 1000000, broadcast to all.
[Process 0] Computing terms 0..249999
[Process 1] Computing terms 250000..499999
...
Approximated PI = 3.14159265...  (error: 9.3e-7)
```
