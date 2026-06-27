/*
 * scope_static.c
 * --------------
 * Scope delle variabili e classi di memorizzazione in C.
 *   - variabile globale vs locale;
 *   - variabile locale static (conserva il valore tra le chiamate);
 *   - const per le costanti.
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 scope_static.c -o scope_static
 * Esecuzione:   ./scope_static
 */

#include <stdio.h>

/* Variabile globale: visibile a tutte le funzioni di questo file. */
int contatore_globale = 0;

/* Usa una variabile locale static: viene inizializzata una sola volta e
   mantiene il proprio valore tra una chiamata e l'altra. */
int chiamate(void)
{
    static int n = 0;   /* NON azzerata a ogni chiamata */
    n++;
    return n;
}

void incrementa_globale(void)
{
    contatore_globale++;          /* modifica la variabile globale */
    int locale = 100;             /* variabile locale: esiste solo qui */
    locale++;                     /* usata per evitare warning */
    (void)locale;
}

int main(void)
{
    /* const: valore non modificabile dopo l'inizializzazione */
    const double PI = 3.14159265358979;
    printf("Costante PI = %.5f\n\n", PI);

    /* La static dentro chiamate() ricorda quante volte è stata invocata. */
    printf("Chiamate consecutive alla funzione (variabile static):\n");
    for (int i = 0; i < 5; i++) {
        printf("  chiamata numero %d\n", chiamate());
    }

    /* La globale è condivisa tra le chiamate di funzioni diverse. */
    for (int i = 0; i < 3; i++) {
        incrementa_globale();
    }
    printf("\ncontatore_globale dopo 3 incrementi = %d\n", contatore_globale);

    return 0;
}
