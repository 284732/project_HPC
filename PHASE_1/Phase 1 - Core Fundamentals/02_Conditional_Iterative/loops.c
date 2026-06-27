/*
 * loops.c
 * -------
 * Costrutti iterativi in C: for, while, do-while.
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 loops.c -o loops
 * Esecuzione:   ./loops
 */

#include <stdio.h>

int main(void)
{
    /* --- Ciclo for: somma dei primi N interi (numero di iterazioni noto) --- */
    int N = 10;
    long somma = 0;
    for (int i = 1; i <= N; i++) {
        somma += i;
    }
    printf("Somma dei primi %d interi (for)   = %ld\n", N, somma);

    /* --- Ciclo while: la condizione è verificata PRIMA di ogni iterazione --- */
    /* Calcolo del fattoriale di 5. */
    int n = 5;
    long fattoriale = 1;
    int k = n;
    while (k > 1) {
        fattoriale *= k;
        k--;
    }
    printf("Fattoriale di %d (while)          = %ld\n", n, fattoriale);

    /* --- Ciclo do-while: il corpo è eseguito ALMENO una volta --- */
    /* Conta quante volte si può dimezzare un numero prima di scendere sotto 1. */
    double valore = 100.0;
    int dimezzamenti = 0;
    do {
        valore /= 2.0;
        dimezzamenti++;
    } while (valore >= 1.0);
    printf("Dimezzamenti di 100 fino a <1 (do-while) = %d\n", dimezzamenti);

    /* --- for annidato: piccola tabellina (uso tipico su matrici) --- */
    printf("\nTabella di moltiplicazione 1..4:\n");
    for (int r = 1; r <= 4; r++) {
        for (int c = 1; c <= 4; c++) {
            printf("%4d", r * c);
        }
        printf("\n");
    }

    return 0;
}
