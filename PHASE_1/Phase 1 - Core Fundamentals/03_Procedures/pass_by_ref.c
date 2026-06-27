/*
 * pass_by_ref.c
 * -------------
 * Passaggio dei parametri in C:
 *   - per valore  (la funzione riceve una copia);
 *   - per riferimento tramite puntatori (la funzione modifica l'originale);
 *   - array come parametri (decadono in puntatori, niente copia).
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 pass_by_ref.c -o pass_by_ref
 * Esecuzione:   ./pass_by_ref
 */

#include <stdio.h>

/* Per valore: modifica solo la copia locale, NON la variabile del chiamante. */
void per_valore(int x)
{
    x = 99;
    printf("  [dentro per_valore] la copia locale ora vale %d\n", x);
}

/* Per riferimento: tramite il puntatore modifica la variabile originale. */
void raddoppia(int *p)
{
    *p = *p * 2;
}

/* Restituire piu' valori usando puntatori come parametri di uscita:
   calcola contemporaneamente minimo e massimo di un array. */
void min_max(const int v[], int n, int *out_min, int *out_max)
{
    int mn = v[0], mx = v[0];
    for (int i = 1; i < n; i++) {
        if (v[i] < mn) mn = v[i];
        if (v[i] > mx) mx = v[i];
    }
    *out_min = mn;
    *out_max = mx;
}

/* Array come parametro: si lavora direttamente sull'array del chiamante.
   Qui ne raddoppiamo ogni elemento (modifica in-place). */
void raddoppia_array(double v[], int n)
{
    for (int i = 0; i < n; i++) {
        v[i] *= 2.0;
    }
}

int main(void)
{
    /* --- per valore vs per riferimento --- */
    int a = 21;
    per_valore(a);
    printf("Dopo per_valore(a):  a = %d (invariato)\n", a);

    raddoppia(&a);
    printf("Dopo raddoppia(&a):  a = %d (modificato)\n", a);

    /* --- piu' valori in uscita tramite puntatori --- */
    int dati[] = {7, 3, 9, 1, 8, 5};
    int n = sizeof(dati) / sizeof(dati[0]);
    int mn, mx;
    min_max(dati, n, &mn, &mx);
    printf("\nArray: ");
    for (int i = 0; i < n; i++) printf("%d ", dati[i]);
    printf("\nMinimo = %d, Massimo = %d\n", mn, mx);

    /* --- array passato per indirizzo (modifica in-place) --- */
    double v[] = {1.0, 2.5, 3.0};
    int m = sizeof(v) / sizeof(v[0]);
    raddoppia_array(v, m);
    printf("\nArray dopo raddoppia_array: ");
    for (int i = 0; i < m; i++) printf("%.1f ", v[i]);
    printf("\n");

    return 0;
}
