/*
 * functions.c
 * -----------
 * Definizione di funzioni, prototipi, valore di ritorno e funzioni void.
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 functions.c -o functions
 * Esecuzione:   ./functions
 */

#include <stdio.h>

/* --- Prototipi (dichiarazioni): permettono di usare le funzioni
       prima della loro definizione e di farne controllare le chiamate. --- */
double area_cerchio(double raggio);
int    massimo(int a, int b);
void   stampa_intestazione(const char *titolo);   /* funzione void */

int main(void)
{
    stampa_intestazione("Esempi di funzioni");

    double r = 2.0;
    printf("Area di un cerchio di raggio %.1f = %.4f\n", r, area_cerchio(r));

    printf("Massimo tra 17 e 42 = %d\n", massimo(17, 42));

    return 0;
}

/* --- Definizioni --- */

double area_cerchio(double raggio)
{
    const double PI = 3.14159265358979;
    return PI * raggio * raggio;
}

int massimo(int a, int b)
{
    return (a > b) ? a : b;
}

/* Funzione void: esegue un'azione ma non restituisce alcun valore. */
void stampa_intestazione(const char *titolo)
{
    printf("=== %s ===\n", titolo);
}
