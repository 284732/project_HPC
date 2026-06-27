# Makefile - Phase 1 (Topic 2, linguaggio C)
# Compila tutti gli esempi delle tre cartelle con i warning attivati.
#
#   make        compila tutti gli eseguibili
#   make run    compila ed esegue gli esempi non interattivi
#   make clean  rimuove eseguibili e file generati dagli esempi

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2

# Eseguibili (uno per ogni sorgente .c), generati nella rispettiva cartella
BINS = \
	01_IO/io_standard \
	01_IO/io_file_text \
	01_IO/io_file_binary \
	01_IO/io_errors \
	02_Conditional_Iterative/conditionals \
	02_Conditional_Iterative/loops \
	02_Conditional_Iterative/break_continue \
	03_Procedures/functions \
	03_Procedures/pass_by_ref \
	03_Procedures/recursion \
	03_Procedures/scope_static

all: $(BINS)

# Regola generica: ogni eseguibile dipende dal file .c omonimo
%: %.c
	$(CC) $(CFLAGS) $< -o $@

# Esegue gli esempi non interattivi (io_standard richiede input: si salta).
run: all
	@echo "===== 01_IO ====="
	@cd 01_IO && ./io_file_text && echo "" && ./io_file_binary && echo "" && ./io_errors
	@echo ""
	@echo "===== 02_Conditional_Iterative ====="
	@cd 02_Conditional_Iterative && ./conditionals && echo "" && ./loops && echo "" && ./break_continue
	@echo ""
	@echo "===== 03_Procedures ====="
	@cd 03_Procedures && ./functions && echo "" && ./pass_by_ref && echo "" && ./recursion && echo "" && ./scope_static

clean:
	rm -f $(BINS)
	rm -f 01_IO/dati.txt 01_IO/dati.bin 01_IO/numeri.txt

.PHONY: all run clean
