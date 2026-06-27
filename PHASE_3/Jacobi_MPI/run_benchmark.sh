#!/bin/bash
PROCS=(1 2 4)
OUTFILE="benchmark_results.csv"

echo "lang,np,time_s,iters" > $OUTFILE

for NP in "${PROCS[@]}"; do
    for LANG in cpp f; do
        BIN="./jacobi2d_${LANG}"
        OUTPUT=$(mpirun -np $NP $BIN 2>/dev/null)

        TIME=$(echo  "$OUTPUT" | grep "Total time"   | grep -oP '[0-9]+\.[0-9]+')
        ITERS=$(echo "$OUTPUT" | grep -oP '(?<=after )[0-9]+')

        echo "${LANG},${NP},${TIME},${ITERS}" >> $OUTFILE
        echo "[${LANG}] np=${NP}  time=${TIME}s  iters=${ITERS}"
    done
done

echo ""
echo "Risultati salvati in $OUTFILE"