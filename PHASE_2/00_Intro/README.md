# 00 — Introduzione: C++ per HPC e il confronto con Python

## Contesto: perché questo tutorial esiste

Il tuo collega ha costruito una fase di HPC in **Python** usando NumPy, SciPy e `mpi4py`.  
Questo tutorial fa la stessa cosa in **C++**, che è il punto di arrivo naturale di quel percorso:
Python con NumPy/SciPy è potente perché sotto il cofano chiama librerie scritte in **C e Fortran**.  
In C++ quelle librerie si chiamano direttamente, senza lo strato Python in mezzo.

---

## Il problema di Python che questo tutorial risolve

Il notebook del tuo collega identifica due bottleneck di Python puro:

| Problema Python | Soluzione Python (SciPy) | Soluzione C++ |
|----------------|--------------------------|---------------|
| Oggetti `PyObject` pesanti, memoria frammentata | `numpy.ndarray` — blocco contiguo di memoria | `std::vector<double>` — contiguo per definizione |
| GIL blocca il multithreading | Operazioni vettorizzate escono dal GIL | Non esiste GIL: thread reali, SIMD nativo |
| Loop Python lenti | Vectorization → C/Fortran backend | Il loop C++ compila direttamente in istruzioni CPU |
| mpi4py overhead (pickling) | `comm.Send()` uppercase → buffer diretto | `MPI_Send()` — direttamente sul buffer, zero overhead |

---

## La mappa dei concetti: Python → C++

### Memoria e array

```python
# Python (NumPy)
import numpy as np
x = np.array([1.0, 2.0, 3.0], dtype=np.float64)  # blocco contiguo
```

```cpp
// C++: stesso layout, nessuna libreria esterna necessaria
#include <vector>
std::vector<double> x = {1.0, 2.0, 3.0};  // contiguo in memoria
// oppure, per array raw come MPI si aspetta:
double x[3] = {1.0, 2.0, 3.0};
```

> In C++ l'array è **sempre** contiguo. Non c'è il problema di Python list vs ndarray:
> in C++ non esiste una lista di puntatori — `std::vector<double>` è già il formato "NumPy".

---

### Linear Algebra: NumPy/LAPACK → chiamata diretta

```python
# Python: numpy chiama LAPACK sotto
import numpy as np
A = np.array([[4., -2.], [1., 5.]])
eigenvalues, _ = np.linalg.eig(A)
```

```cpp
// C++: chiami LAPACK direttamente (o usi Eigen, libreria header-only)
// Con Eigen (comune in HPC moderno):
#include <Eigen/Dense>
Eigen::Matrix2d A;
A << 4, -2, 1, 5;
Eigen::EigenSolver<Eigen::Matrix2d> solver(A);
auto eigenvalues = solver.eigenvalues();
```

In C++ non hai bisogno di SciPy come intermediario — puoi linkare BLAS/LAPACK direttamente,
oppure usare librerie header-only come **Eigen** o **Armadillo**.

---

### MPI: mpi4py → MPI nativo in C++

Questo è il confronto più importante per questo tutorial:

| Concetto | Python `mpi4py` | C++ MPI |
|----------|----------------|---------|
| Import | `from mpi4py import MPI` | `#include <mpi.h>` |
| Init | automatico all'import | `MPI_Init(&argc, &argv)` |
| Finalize | automatico | `MPI_Finalize()` |
| Communicator | `MPI.COMM_WORLD` | `MPI_COMM_WORLD` |
| Rank | `comm.Get_rank()` | `MPI_Comm_rank(MPI_COMM_WORLD, &rank)` |
| Size | `comm.Get_size()` | `MPI_Comm_size(MPI_COMM_WORLD, &size)` |
| Send bloccante | `comm.Send(buf, dest, tag)` | `MPI_Send(buf, count, type, dest, tag, comm)` |
| Recv bloccante | `comm.Recv(buf, source, tag)` | `MPI_Recv(buf, count, type, src, tag, comm, &status)` |
| Send non-bloccante | `comm.Isend(buf, dest, tag)` | `MPI_Isend(buf, count, type, dest, tag, comm, &req)` |
| Wait | `req.wait()` | `MPI_Wait(&req, &status)` |
| Bcast | `comm.Bcast(buf, root)` | `MPI_Bcast(buf, count, type, root, comm)` |
| Scatter | `comm.Scatter(send, recv, root)` | `MPI_Scatter(send, sc, t, recv, rc, t, root, comm)` |
| Gather | `comm.Gather(send, recv, root)` | `MPI_Gather(send, sc, t, recv, rc, t, root, comm)` |
| Reduce | `comm.Reduce(send, recv, op, root)` | `MPI_Reduce(send, recv, n, t, op, root, comm)` |
| Allreduce | `comm.Allreduce(send, recv, op)` | `MPI_Allreduce(send, recv, n, t, op, comm)` |

**Differenza chiave:** in Python il tipo del dato è inferito dall'array NumPy.
In C++ devi specificarlo esplicitamente (`MPI_DOUBLE`, `MPI_INT`, ecc.) — ma questo
ti dà controllo totale sul layout di memoria, zero overhead di serializzazione.

---

## Il "Two-Language Problem" risolto

Il collega ha identificato un limite di Python+SciPy:

> *"If a SciPy routine crashes deep inside its Fortran backend, Python might just throw
> a generic Segmentation Fault without a clear traceback."*

In C++ questo problema non esiste: scrivi e debuggi nello stesso linguaggio che esegue.
Il compilatore (`g++`, `icpx`) ti dà errori a compile-time, AddressSanitizer a runtime,
`gdb` per il debug — tutto sullo stesso codice.

---

## Cosa **non** hai in C++ rispetto a Python

Essere onesti è importante:

| Feature Python | Equivalente C++ | Costo |
|---------------|-----------------|-------|
| Plot con Matplotlib | gnuplot, matplotlib-cpp, salva CSV e plotta fuori | più verboso |
| Jupyter Notebook interattivo | nessun equivalente nativo | devi ricompilare |
| `scipy.fft` in una riga | FFTW (libreria C, molto più veloce) | richiede linking |
| Prototipazione rapida | — | C++ è più lento da scrivere |

Per un workflow HPC reale il trade-off è: **meno velocità di sviluppo, massima velocità di esecuzione**.
Per la ricerca spesso si usa Python per esplorare e C++/Fortran per produzione.

---

## Come si legge questo tutorial rispetto al progetto del tuo collega

```
Collega (Python)              Questo tutorial (C++)
─────────────────────         ──────────────────────────────
Notebook 1: NumPy/SciPy   →  Questo README (00_intro)
Notebook 2: mpi4py P2P    →  01_point_to_point/
Notebook 2: mpi4py Coll.  →  03_collective/
Notebook 2: mpi4py Comm.  →  02_communicators/
(non presente)             →  04_topologies/ (Jacobi 2D)
```

Il modulo sulle topologie (griglia cartesiana + Jacobi) è un'aggiunta rispetto
al progetto Python — riflette il fatto che in C++ si arriva più facilmente
a implementare algoritmi numerici completi come il solutore di Laplace.
