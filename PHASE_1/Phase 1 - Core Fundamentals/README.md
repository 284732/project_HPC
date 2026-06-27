# Phase 1: Core Fundamentals — Il linguaggio C

## Panoramica

Questa fase stabilisce le conoscenze fondamentali del linguaggio C necessarie a
comprendere il resto del progetto sul Topic 2. Il C è un linguaggio **compilato** e
**staticamente tipizzato**: il codice sorgente viene tradotto una volta in codice
macchina ottimizzato, e ogni variabile ha un tipo fissato a tempo di compilazione.
Questo lo rende particolarmente adatto al calcolo ad alte prestazioni, dove conta avere
un controllo prevedibile su memoria e tempi di esecuzione.

La Phase 1 copre i tre argomenti a me assegnati nel Topic 2:

1. **I/O** — `01_IO/`
2. **Costrutti condizionali e iterativi** — `02_Conditional_Iterative/`
3. **Procedure (funzioni)** — `03_Procedures/`

Ogni cartella contiene un file Markdown teorico (in italiano) e uno o più file sorgente
`.c` compilabili che illustrano i concetti con esempi eseguibili.

## Argomenti trattati

### 1. I/O (`01_IO/`)

- I/O standard formattato: `printf`, `scanf` e gli specificatori di formato.
- Flussi standard: `stdin`, `stdout`, `stderr`.
- I/O carattere per carattere e per riga: `getchar`/`putchar`, `fgets`/`fputs`.
- I/O su file di testo: `fopen`/`fclose`, modalità di apertura, `fprintf`/`fscanf`.
- I/O binario: `fread`/`fwrite`.
- Gestione degli errori di I/O: valore di ritorno, `EOF`, `feof`, `ferror`, `perror`.

### 2. Costrutti condizionali e iterativi (`02_Conditional_Iterative/`)

- Selezione: `if`, `else if`, `else`, operatore ternario `?:`, `switch`/`case`.
- Operatori relazionali e logici, valutazione in corto circuito.
- Iterazione: cicli `for`, `while`, `do-while`.
- Controllo del flusso nei cicli: `break`, `continue` (e cenni a `goto`).

### 3. Procedure (`03_Procedures/`)

- Definizione di funzioni, tipo di ritorno e parametri.
- Dichiarazione (prototipi) e file header.
- Passaggio per valore e passaggio per riferimento tramite puntatori.
- Array come parametri (decadimento a puntatore).
- Ricorsione.
- Scope delle variabili e classi di memorizzazione (`static`, variabili locali/globali).

## Compilazione ed esecuzione

Gli esempi richiedono solo un compilatore C standard. Il `Makefile` incluso compila
tutti i sorgenti con i warning attivati (`-Wall -Wextra -std=c11`):

```bash
make        # compila tutti gli esempi nelle tre cartelle
make run    # esegue gli esempi non interattivi e ne mostra l'output
make clean  # rimuove gli eseguibili generati
```

In alternativa, ogni file può essere compilato singolarmente, ad esempio:

```bash
gcc -Wall -Wextra -std=c11 01_IO/io_standard.c -o io_standard
./io_standard
```

Alcuni esempi (in particolare `io_standard.c` con `scanf`) leggono da tastiera: in quel
caso si può fornire l'input da terminale oppure tramite redirezione, ad esempio
`echo "5 3.14" | ./io_standard`.

## Nota sul contesto HPC

Il C condivide con il Fortran le caratteristiche che lo rendono adatto all'HPC:
compilazione ahead-of-time, tipizzazione statica, accesso diretto alla memoria e assenza
di un runtime "pesante" tra il programma e la macchina. A differenza del Fortran, il C
espone i puntatori in modo esplicito, dando al programmatore un controllo molto fine
(ma anche più responsabilità) sulla gestione della memoria. Questi aspetti vengono
richiamati nei singoli documenti quando rilevanti.
