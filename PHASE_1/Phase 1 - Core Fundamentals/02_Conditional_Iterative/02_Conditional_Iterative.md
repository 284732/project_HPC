# Costrutti condizionali e iterativi in C

## Introduzione

Il **controllo del flusso** determina quali istruzioni vengono eseguite e quante volte
vengono ripetute. Senza di esso un programma sarebbe solo una sequenza lineare di
istruzioni. In C il controllo del flusso si articola in due grandi famiglie:

- i **costrutti condizionali** (o di selezione), che eseguono blocchi diversi a seconda
  di una condizione logica: `if`/`else`, l'operatore ternario `?:` e `switch`;
- i **costrutti iterativi** (o cicli), che ripetono un blocco finché una condizione
  resta vera: `for`, `while`, `do-while`, insieme alle istruzioni `break` e `continue`.

Una particolarità del C, importante da capire fin da subito, riguarda i **valori di
verità**: il C non ha avuto a lungo un tipo booleano nativo e tratta qualsiasi
espressione numerica come condizione. Il valore `0` significa *falso*; qualsiasi valore
diverso da zero significa *vero*. Lo standard C99 ha introdotto il tipo `bool` (con
`<stdbool.h>`), ma il meccanismo sottostante resta numerico.

## 1. Operatori usati nelle condizioni

Le condizioni si costruiscono con operatori **relazionali** e **logici**:

| Categoria   | Operatori                                    |
| ----------- | -------------------------------------------- |
| relazionali | `==`  `!=`  `<`  `>`  `<=`  `>=`              |
| logici      | `&&` (AND), `||` (OR), `!` (NOT)             |

Un errore classico è confondere l'assegnamento `=` con il confronto `==`. La scrittura
`if (x = 5)` assegna 5 a `x` e risulta sempre vera: i compilatori moderni avvisano con
un warning, motivo per cui conviene sempre compilare con `-Wall`.

Gli operatori `&&` e `||` usano la **valutazione in corto circuito** (*short-circuit*):
in `a && b`, se `a` è falso `b` non viene nemmeno valutato; in `a || b`, se `a` è vero
`b` viene saltato. Questo permette idiomi sicuri come `if (p != NULL && p->valore > 0)`.

## 2. `if`, `else if`, `else`

Il costrutto di selezione fondamentale:

```c
if (condizione) {
    /* eseguito se condizione è vera (diversa da zero) */
} else if (altra_condizione) {
    /* eseguito se la prima è falsa e questa è vera */
} else {
    /* eseguito se tutte le precedenti sono false */
}
```

Le condizioni vengono valutate **in ordine**: appena una risulta vera, il suo blocco
viene eseguito e gli altri rami vengono saltati. Le parentesi graffe `{ }` non sono
obbligatorie per un blocco di una sola istruzione, ma usarle sempre è considerata buona
pratica perché evita errori quando si aggiungono righe in seguito.

## 3. Operatore ternario `?:`

L'operatore ternario è una forma compatta di `if/else` che restituisce un **valore**:

```c
int massimo = (a > b) ? a : b;
```

equivale a "se `a > b` allora `a` altrimenti `b`". È comodo per assegnamenti condizionali
brevi, ma va usato con misura per non compromettere la leggibilità.

## 4. `switch`

Quando si deve scegliere tra molti valori discreti di una stessa espressione **intera**,
`switch` è più chiaro di una lunga catena di `else if`:

```c
switch (espressione) {
    case 1:
        /* ... */
        break;        /* esce dallo switch */
    case 2:
        /* ... */
        break;
    default:
        /* eseguito se nessun case corrisponde */
        break;
}
```

Aspetto cruciale: senza l'istruzione `break`, l'esecuzione **prosegue nel case
successivo** (comportamento detto *fall-through*). Questo a volte è voluto (più etichette
che condividono lo stesso codice), ma più spesso un `break` mancante è un bug.

## 5. Cicli `for`

Il ciclo `for` è la forma più usata quando il numero di iterazioni è noto o legato a un
contatore. Ha tre parti separate da punto e virgola:

```c
for (inizializzazione; condizione; aggiornamento) {
    /* corpo del ciclo */
}

for (int i = 0; i < 10; i++) {
    printf("%d ", i);
}
```

L'inizializzazione viene eseguita una sola volta; la condizione è verificata **prima** di
ogni iterazione; l'aggiornamento è eseguito **dopo** ogni iterazione. In ambito HPC il
ciclo `for` su indici è il "cavallo di battaglia" del calcolo numerico (scorrere vettori
e matrici): la sua struttura prevedibile permette al compilatore ottimizzazioni come
l'*unrolling* e la vettorizzazione.

## 6. Cicli `while` e `do-while`

Il ciclo `while` verifica la condizione **prima** di ogni iterazione: se è falsa già
all'inizio, il corpo non viene eseguito nemmeno una volta.

```c
while (condizione) {
    /* corpo */
}
```

Il ciclo `do-while` verifica la condizione **dopo** il corpo, garantendo quindi
**almeno una** esecuzione:

```c
do {
    /* corpo eseguito almeno una volta */
} while (condizione);
```

`do-while` è utile, per esempio, per ripetere la richiesta di un input finché non è
valido. È tipico dei metodi iterativi che continuano finché non si raggiunge una
precisione richiesta.

## 7. `break`, `continue` e `goto`

All'interno dei cicli:

- `break` interrompe immediatamente il ciclo (esce dal ciclo più interno);
- `continue` salta il resto dell'iterazione corrente e passa alla successiva.

Esiste anche `goto`, che salta a un'etichetta nello stesso corpo di funzione. Nella
programmazione strutturata se ne sconsiglia l'uso, con un'eccezione comune: uscire da
cicli profondamente annidati o gestire la liberazione delle risorse in caso di errore.

## 8. File di esempio in questa cartella

| File              | Cosa mostra                                                     |
| ----------------- | -------------------------------------------------------------- |
| `conditionals.c`  | `if`/`else if`/`else`, operatore ternario, `switch`            |
| `loops.c`         | cicli `for`, `while`, `do-while`                                |
| `break_continue.c`| uso di `break` e `continue`, esempio di parameter sweep        |

## 9. Riepilogo

Il controllo del flusso in C si basa su costrutti condizionali (`if`/`else`, `?:`,
`switch`) e iterativi (`for`, `while`, `do-while`). Le condizioni sono espressioni
numeriche in cui zero significa falso e qualunque altro valore significa vero. Gli
operatori logici `&&` e `||` sono valutati in corto circuito. `switch` è adatto alla
selezione su valori interi discreti, ma richiede attenzione al `break` per evitare il
fall-through. I cicli `for` su indici sono centrali nel calcolo numerico e sono quelli
che il compilatore ottimizza più efficacemente, mentre `do-while` garantisce almeno una
iterazione. `break` e `continue` permettono un controllo fine dell'iterazione.

