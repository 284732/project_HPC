/*
 * io_file_text.c
 * --------------
 * I/O su file di testo: fopen, fprintf, fgets, fclose.
 * Il programma crea un file di testo, lo riempie con alcune righe di dati e
 * poi lo rilegge riga per riga, convertendo i valori in numeri.
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 io_file_text.c -o io_file_text
 * Esecuzione:   ./io_file_text
 */

#include <stdio.h>
#include <stdlib.h>

#define NOME_FILE "dati.txt"

int main(void)
{
    /* --- Scrittura (modalità "w": crea o azzera il file) --- */
    FILE *out = fopen(NOME_FILE, "w");
    if (out == NULL) {
        perror("Impossibile aprire il file in scrittura");
        return 1;
    }

    fprintf(out, "temperatura pressione\n");   /* riga di intestazione */
    fprintf(out, "300.0 1.0\n");
    fprintf(out, "400.0 1.5\n");
    fprintf(out, "500.0 2.0\n");
    fclose(out);
    printf("File '%s' creato.\n\n", NOME_FILE);

    /* --- Lettura (modalità "r") riga per riga con fgets --- */
    FILE *in = fopen(NOME_FILE, "r");
    if (in == NULL) {
        perror("Impossibile aprire il file in lettura");
        return 1;
    }

    char riga[128];
    double somma_t = 0.0;
    int conta = 0;

    /* salta la riga di intestazione */
    if (fgets(riga, sizeof(riga), in) != NULL) {
        printf("Intestazione: %s", riga);   /* riga contiene già '\n' */
    }

    printf("Dati letti:\n");
    while (fgets(riga, sizeof(riga), in) != NULL) {
        double t, p;
        /* sscanf interpreta i due numeri contenuti nella riga */
        if (sscanf(riga, "%lf %lf", &t, &p) == 2) {
            printf("  T = %6.1f   P = %4.1f\n", t, p);
            somma_t += t;
            conta++;
        }
    }
    fclose(in);

    if (conta > 0) {
        printf("\nTemperatura media = %.2f (su %d valori)\n",
               somma_t / conta, conta);
    }

    return 0;
}
