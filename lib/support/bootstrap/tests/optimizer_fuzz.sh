#!/bin/bash
# Random programs against the optimizer (doc/codegen.md 13.6, "The verifier's pass"). NOT a gates
# step: it is run by hand after a change to backend/optimizer, for as many programs as there is
# time for.
#
#   tests/optimizer_fuzz.sh <first seed> <last seed> [options instead of -O]
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  SHARPC=<the C++ compiler>
#       LIBRARY=<the standard library folder>  OUT=<a folder to work in>  JOBS=<how many at a time: 4>
#       NO_TRY=1   programs without `try` (loop unrolling leaves a function with a `try` alone,
#                  so only these exercise it)
#
# For each seed, optimizer_fuzz.py writes one program (the same seed gives the same program). It
# is built by the bootstrap compiler plain and with -O (or the options given, e.g.
# `-O -ninline-calls` to find the pass at fault), and run with the JIT and with -nojit. All four
# outputs must be the same: the optimizer may make a program faster, never different. One line
# per seed:
#
#   seed 17: same (153 lines)                       nothing to see
#   seed 17: same; (C++ build differs from plain)   the C++-built program prints something else.
#                                                   Not a failure of this test - so far always one
#                                                   of the C++ compiler's own slips (codegen.md 10
#                                                   and 13.6) - but worth a look when it is new
#   seed 17: ... OPT -nojit DIFFERS / OPT jit DIFFERS     an optimizer bug. The program is
#                                                   <OUT>/f17.sharp; -Otrace <function> shows the
#                                                   IR after each pass
#   seed 17: PLAIN jit/nojit differ                 not the optimizer: code generation or the VM
#
# Exits 1 when any seed shows an optimizer difference or was not built.
HERE="$(cd "$(dirname "$0")" && pwd)"
export SHARP=${SHARP:-$HOME/sharp_work/sharp_cl/sharp}
export COMPILER=${COMPILER:-$HOME/sharp_work/out/boot/sharpc_boot}
export SHARPC=${SHARPC:-$HOME/sharp_work/sharpc_cl/sharpc}
export LIBRARY=${LIBRARY:-$HOME/sharp_work/stdlib}
export OUT=${OUT:-$HOME/sharp_work/out/boot/optimizer_fuzz}
export HERE
export LIB=$(find "$LIBRARY" -name "*.sharp" | sort)
first=${1:?optimizer_fuzz.sh <first seed> <last seed> [options instead of -O]}
last=${2:?optimizer_fuzz.sh <first seed> <last seed> [options instead of -O]}
shift; shift
export OPTIONS="${*:--O}"
mkdir -p "$OUT"

one() {
    seed=$1; program=$OUT/f$seed.sharp; print="$HERE/codegen/print.sharp"
    python3 "$HERE/optimizer_fuzz.py" $seed "$program"
    rm -f "$OUT/f$seed.plain" "$OUT/f$seed.opt" "$OUT/f$seed.cpp"
    timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB "$print" "$program" -o "$OUT/f$seed.plain" > "$OUT/f$seed.plain.txt" 2>&1
    if [ ! -f "$OUT/f$seed.plain" ]; then echo "seed $seed: NOT BUILT: $(grep -m1 -A4 '^error' "$OUT/f$seed.plain.txt" | tr '\n' ' ' | cut -c1-300)"; return; fi
    timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB "$print" "$program" -o "$OUT/f$seed.opt" $OPTIONS > "$OUT/f$seed.opt.txt" 2>&1
    if [ ! -f "$OUT/f$seed.opt" ]; then echo "seed $seed: NOT BUILT with $OPTIONS: $(grep -m1 -A6 '^error' "$OUT/f$seed.opt.txt" | tr '\n' ' ' | cut -c1-400)"; return; fi
    "$SHARPC" "$program" "$print" -o "$OUT/f$seed.cpp" -j 1 -L "$LIBRARY" > "$OUT/f$seed.cpp.txt" 2>&1
    # what a program prints, without the VM's time line and the addresses a stack trace ends its lines with
    run() { timeout -s KILL 60 "$SHARP" $2 "$OUT/f$seed.$1" 2>&1 | sed -e '/^Executed in [0-9]*us/d' -e 's/ \[[0-9]*\] \$[0-9]* *$//' > "$OUT/f$seed.$1$2.out"; }
    run plain ""; run plain -nojit; run opt ""; run opt -nojit
    verdict="same"
    cmp -s "$OUT/f$seed.plain.out" "$OUT/f$seed.plain-nojit.out" || verdict="PLAIN jit/nojit differ"
    cmp -s "$OUT/f$seed.plain-nojit.out" "$OUT/f$seed.opt-nojit.out" || verdict="$verdict; OPT -nojit DIFFERS"
    cmp -s "$OUT/f$seed.plain-nojit.out" "$OUT/f$seed.opt.out" || verdict="$verdict; OPT jit DIFFERS"
    if [ -f "$OUT/f$seed.cpp" ]; then
        run cpp -nojit
        cmp -s "$OUT/f$seed.plain-nojit.out" "$OUT/f$seed.cpp-nojit.out" || verdict="$verdict; (C++ build differs from plain)"
    else
        verdict="$verdict; (the C++ compiler wrote no file)"
    fi
    echo "seed $seed: $verdict ($(wc -l < "$OUT/f$seed.plain-nojit.out") lines)"
}
export -f one
seq $first $last | xargs -P ${JOBS:-4} -I{} bash -c 'one {}' | tee "$OUT/results.txt"
if grep -q "DIFFERS\|NOT BUILT\|PLAIN jit" "$OUT/results.txt"; then
    echo "optimizer fuzz: FAILED for $(grep -c "DIFFERS\|NOT BUILT\|PLAIN jit" "$OUT/results.txt") of $(wc -l < "$OUT/results.txt") programs (they are in $OUT)"
    exit 1
fi
echo "optimizer fuzz: $(wc -l < "$OUT/results.txt") programs, plain and $OPTIONS print the same, JIT and -nojit"
