# Procedure (funzioni) in C

## Introduzione

Una **procedura** è un blocco di codice con un nome, che esegue un compito ben definito e
può essere richiamato più volte da punti diversi del programma. Suddividere un programma
in procedure ne migliora la leggibilità, evita la duplicazione del codice e permette di
ragionare su un problema un pezzo alla volta.

In C tutte le procedure si chiamano **funzioni**: non esiste la distinzione, presente in
altri linguaggi come il Fortran, tra `function` (che restituisce un valore) e
`subroutine` (che non lo restituisce). In C una funzione che non deve restituire nulla ha
semplicemente tipo di ritorno `void`. Lo stesso punto di ingresso del programma, `main`,
è una funzione.

## 1. Definizione di una funzione

La forma generale di una funzione è:

```c
tipo_di_ritorno nome_funzione(elenco_parametri)
{
    /* corpo della funzione */
    return valore;   /* coerente con tipo_di_ritorno */
}
```

Esempio:

```c
double area_cerchio(double raggio)
{
    const double PI = 3.14159265358979;
    return PI * raggio * raggio;
}
```

Qui `double` è il tipo del valore restituito, `area_cerchio` è il nome, e `double raggio`
è l'unico parametro. L'istruzione `return` termina la funzione e ne restituisce il
valore al chiamante. Una funzione `void` può usare `return;` senza valore (o terminare
semplicemente raggiungendo la fine del corpo).

## 2. Dichiarazione (prototipo) e header

In C una funzione deve essere **dichiarata prima di essere usata**, perché il compilatore
analizza il file dall'alto verso il basso. La dichiarazione, detta **prototipo**, indica
nome, tipo di ritorno e tipi dei parametri, senza corpo:

```c
double area_cerchio(double raggio);   /* prototipo: notare il ';' finale */
```

Il prototipo permette al compilatore di verificare che le chiamate usino il numero e il
tipo di argomenti corretti. Nei progetti reali i prototipi si raccolgono in **file
header** (`.h`), inclusi con `#include "mio_header.h"`, mentre le definizioni stanno nei
file sorgente (`.c`). Questo è l'analogo dei `module` del Fortran: un modo per dichiarare
l'interfaccia separatamente dall'implementazione.

## 3. Passaggio dei parametri: per valore

In C i parametri sono **sempre passati per valore**: alla funzione viene consegnata una
*copia* dell'argomento. Modificare il parametro all'interno della funzione **non** ha
effetto sulla variabile del chiamante.

```c
void prova(int x) { x = 99; }   /* modifica solo la copia locale */
...
int a = 1;
prova(a);
/* a vale ancora 1 */
```

Questo è un punto centrale del C e spiega molte scelte del linguaggio.

## 4. Passaggio per riferimento tramite puntatori

Per permettere a una funzione di modificare una variabile del chiamante, si passa il suo
**indirizzo** (un puntatore). La funzione riceve la copia di un indirizzo, ma attraverso
quell'indirizzo può accedere e modificare la variabile originale:

```c
void raddoppia(int *p)   /* p è un puntatore a int */
{
    *p = *p * 2;         /* *p accede alla variabile puntata */
}
...
int a = 21;
raddoppia(&a);           /* si passa l'indirizzo di a */
/* ora a vale 42 */
```

Questo meccanismo è ciò che permette a `scanf("%d", &n)` di riempire la variabile `n`.
È anche il modo in cui una funzione può "restituire" più di un valore: si passano più
puntatori come parametri di uscita. Concettualmente corrisponde al passaggio per
riferimento che in Fortran è il comportamento predefinito.

## 5. Array come parametri

Quando si passa un array a una funzione, in C l'array **decade in un puntatore** al suo
primo elemento. La funzione non riceve quindi la dimensione dell'array, che va passata
come parametro separato:

```c
double media(const double v[], int n)
{
    double somma = 0.0;
    for (int i = 0; i < n; i++) {
        somma += v[i];
    }
    return somma / n;
}
```

Poiché viene passato solo l'indirizzo, la funzione lavora **direttamente** sull'array del
chiamante (non su una copia): può quindi modificarne gli elementi. La parola chiave
`const` nel prototipo (`const double v[]`) segnala che la funzione non modificherà
l'array, aumentando sicurezza e leggibilità. Il fatto che gli array siano passati per
indirizzo, senza copia, è efficiente ed è uno dei motivi per cui il C è adatto al calcolo
numerico su grandi quantità di dati.

## 6. Ricorsione

Una funzione può richiamare sé stessa: si parla di **ricorsione**. Ogni chiamata
ricorsiva deve avvicinarsi a un **caso base** che ferma la ricorsione, altrimenti il
programma non termina (e si esaurisce lo spazio dello stack).

```c
long fattoriale(int n)
{
    if (n <= 1) {
        return 1;          /* caso base */
    }
    return n * fattoriale(n - 1);   /* passo ricorsivo */
}
```

La ricorsione è elegante per problemi definiti in modo ricorsivo, ma in contesti ad alte
prestazioni si preferisce spesso la versione iterativa, che evita l'overhead delle
chiamate di funzione e il rischio di overflow dello stack.

## 7. Scope e classi di memorizzazione

Lo **scope** è la regione del codice in cui un nome è visibile:

- le variabili dichiarate dentro una funzione sono **locali**: esistono solo durante
  l'esecuzione della funzione e non sono visibili altrove;
- le variabili dichiarate fuori da ogni funzione sono **globali**: visibili da tutto il
  file (e da altri file se non `static`).

La parola chiave `static` ha due usi rilevanti:

- su una variabile **locale**, ne rende la durata pari all'intero programma: il valore
  viene conservato tra una chiamata e l'altra della funzione (utile per esempio per un
  contatore di chiamate);
- su una variabile o funzione **globale**, ne limita la visibilità al file corrente,
  evitando conflitti di nomi tra file diversi (incapsulamento).

Esiste anche `const`, che rende un valore non modificabile dopo l'inizializzazione, utile
per definire costanti in modo sicuro.

## 8. File di esempio in questa cartella

| File              | Cosa mostra                                                       |
| ----------------- | ---------------------------------------------------------------- |
| `functions.c`     | definizione e prototipi, valore di ritorno, funzione `void`      |
| `pass_by_ref.c`   | passaggio per valore vs per riferimento, array come parametri    |
| `recursion.c`     | ricorsione (fattoriale, Fibonacci) e confronto con la versione iterativa |
| `scope_static.c`  | scope locale/globale e variabili `static`                        |

## 9. Riepilogo

In C ogni procedura è una funzione, eventualmente di tipo `void` se non restituisce un
valore. Le funzioni vanno dichiarate (prototipo) prima dell'uso, tipicamente tramite file
header che separano l'interfaccia dall'implementazione. I parametri sono sempre passati
per valore: per modificare una variabile del chiamante si passa il suo indirizzo
(puntatore), e questo è anche il modo per restituire più risultati. Gli array decadono in
puntatori, quindi vengono passati per indirizzo senza copia, cosa efficiente per il
calcolo numerico. La ricorsione è possibile ma più costosa dell'iterazione. Infine,
scope e classi di memorizzazione (`static`, `const`, variabili locali/globali) governano
visibilità e durata delle variabili.
