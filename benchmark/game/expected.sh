#!/bin/bash
# Writes the folder `expected`: what each program must print for a small input.
# The C programs are the reference - every other language's program is checked
# against these files before it is timed (run.sh).
#
#   expected.sh         (Linux / WSL; needs gcc)
HERE=$(cd "$(dirname "$0")" && pwd)
WORK=${OUT:-$HOME/sharp_game}/reference
mkdir -p "$WORK" "$HERE/expected"

# the test and its small input
SMALL="nbody:1000 spectralnorm:100 fannkuchredux:7 binarytrees:10 mandelbrot:200 fasta:1000"

for pair in $SMALL; do
    name=${pair%%:*}; n=${pair##*:}
    gcc -O3 -ffp-contract=off "$HERE/$name/$name.c" -o "$WORK/$name" -lm || exit 1
    "$WORK/$name" "$n" > "$HERE/expected/${name}_$n.txt"
    echo "$name $n: $(md5sum < "$HERE/expected/${name}_$n.txt" | cut -d' ' -f1)"
done
