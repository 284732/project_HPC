/*
 * break_continue.c
 * ----------------
 * Controllo fine dell'iterazione con break e continue.
 *   - continue: salta l'iterazione corrente;
 *   - break:    interrompe il ciclo.
 * Include un piccolo "parameter sweep": si cerca il primo valore di un
 * parametro che soddisfa una condizione, e ci si ferma con break.
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 break_continue.c -o break_continue
 * Esecuzione:   ./break_continue
 */

#include <stdio.h>

int main(void)
{
    /* --- continue: somma solo i numeri pari da 1 a 10 --- */
    int somma_pari = 0;
    for (int i = 1; i <= 10; i++) {
        if (i % 2 != 0) {
            continue;          /* salta i numeri dispari */
        }
        somma_pari += i;
    }
    printf("Somma dei pari da 1 a 10 (continue) = %d\n", somma_pari);

    /* --- break: trova il primo intero il cui quadrato supera 50 --- */
    int trovato = -1;
    for (int i = 1; i <= 100; i++) {
        if (i * i > 50) {
            trovato = i;
            break;             /* esce appena trovato */
        }
    }
    printf("Primo intero con quadrato > 50 (break) = %d\n", trovato);

    /* --- Parameter sweep: cerca il passo h che porta l'errore sotto soglia ---
       Esempio didattico: l'errore di una formula numerica diminuisce con h. */
    double soglia = 1e-3;
    double h = 1.0;
    int passo = 0;
    printf("\nParameter sweep (riduzione del passo h):\n");
    while (1) {                /* ciclo infinito interrotto da break */
        double errore = h * h; /* errore ~ h^2 */
        printf("  passo %d: h = %.5f, errore = %.6f\n", passo, h, errore);
        if (errore < soglia) {
            printf("  -> soglia %.0e raggiunta con h = %.5f\n", soglia, h);
            break;
        }
        h /= 2.0;
        passo++;
    }

    return 0;
}
