/*
 * recursion.c
 * -----------
 * Ricorsione in C e confronto con la versione iterativa.
 *   - fattoriale ricorsivo e iterativo;
 *   - Fibonacci ricorsivo (chiaro ma inefficiente) e iterativo.
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 recursion.c -o recursion
 * Esecuzione:   ./recursion
 */

#include <stdio.h>

/* Fattoriale ricorsivo: caso base n <= 1, passo n * fattoriale(n-1). */
long fattoriale_ric(int n)
{
    if (n <= 1) {
        return 1;                       /* caso base */
    }
    return (long)n * fattoriale_ric(n - 1);   /* passo ricorsivo */
}

/* Stessa funzione in forma iterativa. */
long fattoriale_it(int n)
{
    long r = 1;
    for (int i = 2; i <= n; i++) {
        r *= i;
    }
    return r;
}

/* Fibonacci ricorsivo "ingenuo": elegante ma con costo esponenziale. */
long fib_ric(int n)
{
    if (n < 2) {
        return n;
    }
    return fib_ric(n - 1) + fib_ric(n - 2);
}

/* Fibonacci iterativo: costo lineare. */
long fib_it(int n)
{
    long a = 0, b = 1;
    for (int i = 0; i < n; i++) {
        long t = a + b;
        a = b;
        b = t;
    }
    return a;
}

int main(void)
{
    int n = 10;

    printf("Fattoriale di %d: ricorsivo = %ld, iterativo = %ld\n",
           n, fattoriale_ric(n), fattoriale_it(n));

    printf("\nPrimi numeri di Fibonacci (ricorsivo vs iterativo):\n");
    for (int i = 0; i <= 10; i++) {
        printf("  F(%2d) = %ld  (it: %ld)\n", i, fib_ric(i), fib_it(i));
    }

    printf("\nNota: la versione ricorsiva di Fibonacci è chiara ma ricalcola\n"
           "gli stessi valori molte volte; quella iterativa è molto più efficiente.\n");

    return 0;
}
