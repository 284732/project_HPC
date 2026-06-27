/*
 * io_file_binary.c
 * ----------------
 * I/O binario su file: fwrite e fread.
 * Il programma scrive un array di double in formato binario e poi lo rilegge,
 * verificando che i dati coincidano. L'I/O binario è tipico in HPC perché è
 * piu' compatto e veloce dell'I/O testuale (nessuna conversione numero<->testo).
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 io_file_binary.c -o io_file_binary
 * Esecuzione:   ./io_file_binary
 */

#include <stdio.h>
#include <stdlib.h>

#define NOME_FILE "dati.bin"
#define N 8

int main(void)
{
    double originale[N];
    for (int i = 0; i < N; i++) {
        originale[i] = i * 1.5;   /* 0.0, 1.5, 3.0, ... */
    }

    /* --- Scrittura binaria (modalità "wb") --- */
    FILE *out = fopen(NOME_FILE, "wb");
    if (out == NULL) {
        perror("Apertura in scrittura fallita");
        return 1;
    }
    /* fwrite restituisce il numero di elementi effettivamente scritti */
    size_t scritti = fwrite(originale, sizeof(double), N, out);
    fclose(out);

    if (scritti != N) {
        fprintf(stderr, "Scrittura incompleta: %zu/%d elementi.\n", scritti, N);
        return 1;
    }
    printf("Scritti %d double (%zu byte) su '%s'.\n",
           N, N * sizeof(double), NOME_FILE);

    /* --- Lettura binaria (modalità "rb") --- */
    double riletto[N];
    FILE *in = fopen(NOME_FILE, "rb");
    if (in == NULL) {
        perror("Apertura in lettura fallita");
        return 1;
    }
    size_t letti = fread(riletto, sizeof(double), N, in);
    fclose(in);

    if (letti != N) {
        fprintf(stderr, "Lettura incompleta: %zu/%d elementi.\n", letti, N);
        return 1;
    }

    /* --- Verifica --- */
    int uguali = 1;
    for (int i = 0; i < N; i++) {
        if (originale[i] != riletto[i]) {
            uguali = 0;
            break;
        }
    }

    printf("Valori riletti: ");
    for (int i = 0; i < N; i++) {
        printf("%.1f ", riletto[i]);
    }
    printf("\nVerifica: i dati riletti %s a quelli scritti.\n",
           uguali ? "coincidono" : "NON coincidono");

    return uguali ? 0 : 1;
}
