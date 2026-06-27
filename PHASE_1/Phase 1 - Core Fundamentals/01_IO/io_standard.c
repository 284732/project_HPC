/*
 * io_standard.c
 * -------------
 * I/O standard in C: printf, scanf, specificatori di formato e flussi standard.
 *
 * Compilazione:  gcc -Wall -Wextra -std=c11 io_standard.c -o io_standard
 * Esecuzione:    ./io_standard
 *                echo "5 3.14" | ./io_standard   (input via redirezione)
 */

#include <stdio.h>

int main(void)
{
    /* --- Output formattato su stdout --- */
    int    interi   = 42;
    double reale    = 3.14159265358979;
    char   carattere = 'C';
    const char *testo = "HPC";

    printf("Esempi di printf:\n");
    printf("  intero      %%d  -> %d\n", interi);
    printf("  reale       %%f  -> %f\n", reale);
    printf("  reale       %%.3f-> %.3f\n", reale);       /* 3 cifre decimali        */
    printf("  reale       %%e  -> %e\n", reale);         /* notazione esponenziale  */
    printf("  carattere   %%c  -> %c\n", carattere);
    printf("  stringa     %%s  -> %s\n", testo);
    printf("  allineato   %%8.3f-> |%8.3f|\n", reale);    /* larghezza minima 8      */

    /* --- Messaggio diagnostico su stderr (flusso separato) --- */
    fprintf(stderr, "[info] questo messaggio va su stderr\n");

    /* --- Input formattato da stdin --- */
    int    n;
    double x;

    printf("\nInserisci un intero e un numero reale (es. 5 3.14): ");
    /* scanf restituisce il numero di valori letti correttamente.
       Si passano gli INDIRIZZI delle variabili con l'operatore &. */
    int letti = scanf("%d %lf", &n, &x);

    if (letti != 2) {
        fprintf(stderr, "Input non valido: attesi 2 valori, letti %d.\n", letti);
        return 1;
    }

    printf("Hai inserito n = %d e x = %.4f\n", n, x);
    printf("Somma = %.4f, prodotto = %.4f\n", n + x, n * x);

    return 0;
}
