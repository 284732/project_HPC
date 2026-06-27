/*
 * conditionals.c
 * --------------
 * Costrutti di selezione in C: if/else if/else, operatore ternario, switch.
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 conditionals.c -o conditionals
 * Esecuzione:   ./conditionals
 */

#include <stdio.h>

/* Restituisce una lettera in base al voto (esempio di catena if/else if). */
static char valuta(int voto)
{
    if (voto >= 90) {
        return 'A';
    } else if (voto >= 80) {
        return 'B';
    } else if (voto >= 70) {
        return 'C';
    } else {
        return 'F';
    }
}

int main(void)
{
    /* --- if / else if / else --- */
    int voti[] = {95, 83, 72, 50};
    int n = sizeof(voti) / sizeof(voti[0]);

    printf("Valutazione dei voti:\n");
    for (int i = 0; i < n; i++) {
        printf("  voto %3d -> categoria %c\n", voti[i], valuta(voti[i]));
    }

    /* --- Operatore ternario: massimo tra due numeri --- */
    int a = 17, b = 42;
    int massimo = (a > b) ? a : b;
    printf("\nIl massimo tra %d e %d è %d\n", a, b, massimo);

    /* --- Valutazione in corto circuito --- */
    int x = 10;
    if (x != 0 && 100 / x > 5) {
        printf("100/%d > 5 (la divisione è sicura grazie al corto circuito)\n", x);
    }

    /* --- switch su valori interi discreti --- */
    printf("\nGiorni del fine settimana (switch):\n");
    for (int giorno = 1; giorno <= 7; giorno++) {
        switch (giorno) {
            case 6:
            case 7:
                /* fall-through voluto: 6 e 7 condividono lo stesso codice */
                printf("  giorno %d: weekend\n", giorno);
                break;
            default:
                printf("  giorno %d: feriale\n", giorno);
                break;
        }
    }

    return 0;
}
