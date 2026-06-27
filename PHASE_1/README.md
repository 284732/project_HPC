# Il linguaggio C nell'High-Performance Computing

## Panoramica del progetto (Topic 2)

Questo progetto discute l'uso del linguaggio **C** nel contesto dell'High-Performance
Computing (HPC), partendo da quanto è stato fatto durante il corso per il linguaggio
Fortran. Il C è uno dei linguaggi compilati più usati nel calcolo scientifico e di
sistema: è vicino alla macchina, offre un controllo esplicito della memoria attraverso
i puntatori ed è il linguaggio in cui sono scritte molte delle librerie numeriche e dei
runtime (incluso lo stesso MPI) utilizzati in ambito HPC.

Secondo la consegna, il Topic 2 copre i seguenti argomenti: **I/O, costrutti condizionali
e iterativi, procedure, array e MPI** (comunicazioni point-to-point e collettive), con un
caso di studio in cui il C viene usato per risolvere un problema in parallelo.

Il lavoro è organizzato in fasi.

## Struttura della repository

```text
PHASE_1/
|-- Phase 1 - Core Fundamentals/
|   |-- 01_IO/
|   |-- 02_Conditional_Iterative/
|   |-- 03_Procedures/
|   |-- Makefile
|   `-- README.md
|
`-- README.md
```

## Phase 1 - Core Fundamentals

Questa repository contiene la **Phase 1** del Topic 2. La fase introduce i fondamenti del linguaggio C necessari a
comprendere il resto del progetto e copre in particolare i tre argomenti a me assegnati:

1. **I/O** – input/output standard (`printf`/`scanf`) e su file (`fopen`, `fprintf`,
   `fread`/`fwrite`, ...).
2. **Costrutti condizionali e iterativi** – `if`/`else`, `switch`, operatore ternario,
   cicli `for`, `while`, `do-while`, `break`/`continue`.
3. **Procedure (funzioni)** – definizione e prototipi, passaggio per valore e per
   riferimento, array come parametri, ricorsione, scope e classi di memorizzazione.

Ogni argomento è documentato in un file Markdown in italiano e accompagnato da file
sorgente `.c` reali, compilabili ed eseguibili. I dettagli su come compilare ed eseguire
gli esempi si trovano nel README della Phase 1.

## Compilazione rapida

Tutti gli esempi sono compilabili con un normale compilatore C (es. `gcc`). Dalla cartella
`Phase 1 - Core Fundamentals` è sufficiente eseguire:

```bash
make        # compila tutti gli esempi
make run    # compila ed esegue gli esempi non interattivi
make clean  # rimuove gli eseguibili
```
