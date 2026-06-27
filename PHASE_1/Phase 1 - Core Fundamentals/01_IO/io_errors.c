/*
 * io_errors.c
 * -----------
 * Gestione degli errori di I/O in C (senza eccezioni):
 *   - controllo del valore di ritorno di fopen;
 *   - uso di perror per stampare la causa dell'errore;
 *   - distinzione tra fine del file (feof) ed errore di I/O (ferror).
 *
 * Compilazione: gcc -Wall -Wextra -std=c11 io_errors.c -o io_errors
 * Esecuzione:   ./io_errors
 */

#include <stdio.h>

int main(void)
{
    /* --- 1. Tentativo di aprire un file inesistente --- */
    const char *inesistente = "questo_file_non_esiste.txt";
    FILE *f = fopen(inesistente, "r");
    if (f == NULL) {
        /* perror stampa il messaggio seguito dalla descrizione di errno */
        perror("Apertura file inesistente");
        printf("-> Gestito correttamente: il programma continua.\n\n");
    } else {
        fclose(f);
    }

    /* --- 2. Creazione di un file e rilettura controllando EOF --- */
    FILE *out = fopen("numeri.txt", "w");
    if (out == NULL) {
        perror("Creazione file");
        return 1;
    }
    for (int i = 1; i <= 5; i++) {
        fprintf(out, "%d\n", i * i);   /* quadrati: 1 4 9 16 25 */
    }
    fclose(out);

    FILE *in = fopen("numeri.txt", "r");
    if (in == NULL) {
        perror("Apertura file numeri.txt");
        return 1;
    }

    int valore;
    long somma = 0;
    int conta = 0;

    /* fscanf restituisce il numero di campi letti; un valore diverso da 1
       segnala fine del file o input non numerico. */
    while (fscanf(in, "%d", &valore) == 1) {
        somma += valore;
        conta++;
    }

    /* Distinzione tra fine file regolare ed errore di lettura */
    if (ferror(in)) {
        fprintf(stderr, "Errore durante la lettura del file.\n");
        fclose(in);
        return 1;
    }
    if (feof(in)) {
        printf("Raggiunta la fine del file (condizione normale).\n");
    }
    fclose(in);

    printf("Letti %d valori, somma = %ld.\n", conta, somma);
    return 0;
}
