#!/bin/bash
# Compiles and runs cases of the suite (tests/suite/cases) by name, with the compiler and the
# library as they are now, and shows the end of what each printed.
# A PART OF test.sh (the one at the top of the repository:  bash test.sh case <name>).   (Linux / WSL)
#
#     internal/run_case.sh [-O] <name> ...
#
#     TAIL=<n>   how many of the last lines to show (default 15)
source "$(dirname "$0")/common.sh"
option=""; [ "$1" = "-O" ] && { option="-O"; shift; }
# (the VM too: a case that tests a change to the VM must not run on the VM of before it)
bash "$REPO/build-scripts/build.sh" > "$WORK/out/case_build.txt" 2>&1 || { tail -20 "$WORK/out/case_build.txt"; echo "the VM or the compiler does not build"; exit 1; }
OUT="$WORK/out/cases"; mkdir -p "$OUT"
ok=1
for name in "$@"; do
    program="$OUT/$name"; rm -f "$program"
    echo "== $name $option"
    timeout -s KILL 300 "$VM" "$COMPILER" "$WORK/suite/cases/$name.sharp" "$WORK/suite/testing.sharp" -o "$program" -j 1 $option -L "$STDLIB" > "$program.compile.txt" 2>&1
    if [ ! -f "$program" ]; then grep -vE "^(time:|unknown)" "$program.compile.txt" | head -40; echo "$name: DID NOT COMPILE"; ok=0; continue; fi
    ( cd "$OUT" && timeout -s KILL 150 "$VM" "$program" > "$program.txt" 2>&1 )
    tail -${TAIL:-15} "$program.txt"
    grep -q -e "RESULT PASS" -e " tests passed" "$program.txt" || { ok=0; echo "$name: FAILED   (everything it printed: $program.txt)"; }
done
[ $ok = 1 ]
