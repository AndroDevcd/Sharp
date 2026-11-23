#!/bin/bash
# Compiles one Sharp file with the compiler and the library as they are now, and runs it.
# A PART OF test.sh (the one at the top of the repository:  bash test.sh try <file> [arguments]).   (Linux / WSL)
#
#     internal/try_program.sh <file.sharp, from the top of the repository> [the program's arguments...]
source "$(dirname "$0")/common.sh"
file=$1; shift
case "$file" in /*) ;; *) file="$REPO/$file" ;; esac
[ -f "$file" ] || { echo "no file $file"; exit 2; }
[ -x "$VM" ] || bash "$REPO/build-scripts/build.sh" > /dev/null
bash "$REPO/build-scripts/build.sh" compiler > "$WORK/out/try_build.txt" 2>&1 || { tail -20 "$WORK/out/try_build.txt"; echo "the compiler does not build"; exit 1; }
OUT="$WORK/out/try"; mkdir -p "$OUT"
program="$OUT/$(basename "$file" .sharp)"; rm -f "$program"
timeout -s KILL 300 "$VM" "$COMPILER" "$file" -o "$program" -j 1 -L "$STDLIB" > "$program.compile.txt" 2>&1
if [ ! -f "$program" ]; then grep -vE "^(time:|unknown)" "$program.compile.txt" | head -60; echo "DID NOT COMPILE"; exit 1; fi
( cd "$OUT" && timeout -s KILL 150 "$VM" "$program" "$@" ); code=$?
echo "exit $code"
