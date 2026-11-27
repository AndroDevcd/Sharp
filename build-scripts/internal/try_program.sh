#!/bin/bash
# Compiles one Sharp file - or one PROJECT folder - with the compiler and the library as they are
# now, and runs it.
# A PART OF test.sh (the one at the top of the repository:  bash test.sh try ...).   (Linux / WSL)
#
#     internal/try_program.sh <file.sharp> [the program's arguments...]
#     internal/try_program.sh <project folder> [compiler options...] [-- the program's arguments...]
#         a folder is built as a project (sharpc -P): `try shop -tests` builds and runs its tests
#
# The path is taken from the top of the repository unless it starts with /.
source "$(dirname "$0")/common.sh"
what=$1; shift
case "$what" in /*) ;; *) what="$REPO/$what" ;; esac
[ -e "$what" ] || { echo "no file or folder $what"; exit 2; }
# (the VM too: a program that tries a change to the VM must not run on the VM of before it)
bash "$REPO/build-scripts/build.sh" > "$WORK/out/try_build.txt" 2>&1 || { tail -20 "$WORK/out/try_build.txt"; echo "the VM or the compiler does not build"; exit 1; }
OUT="$WORK/out/try"; mkdir -p "$OUT"

if [ -d "$what" ]; then
    # a project: the compiler's options up to `--`, the program's arguments after it
    options=(); while [ $# -gt 0 ] && [ "$1" != "--" ]; do options+=("$1"); shift; done; [ "$1" = "--" ] && shift
    rm -rf "$what/build/outputs"
    timeout -s KILL 300 "$VM" "$COMPILER" -P "$what" -j 1 -stdlib "$STDLIB" "${options[@]}" > "$OUT/project.compile.txt" 2>&1
    grep -vE "^(time:|unknown)" "$OUT/project.compile.txt" | head -60
    program=$(ls -t "$what"/build/outputs/* 2>/dev/null | grep -v "\.names$\|/sni$" | head -1)
    [ -f "$program" ] || { echo "DID NOT COMPILE"; exit 1; }
    echo "built: build/outputs/$(basename "$program")"
    ( cd "$what" && timeout -s KILL 150 "$VM" "$program" "$@" ); code=$?
else
    program="$OUT/$(basename "$what" .sharp)"; rm -f "$program"
    timeout -s KILL 300 "$VM" "$COMPILER" "$what" -o "$program" -j 1 -L "$STDLIB" > "$program.compile.txt" 2>&1
    if [ ! -f "$program" ]; then grep -vE "^(time:|unknown)" "$program.compile.txt" | head -60; echo "DID NOT COMPILE"; exit 1; fi
    ( cd "$OUT" && timeout -s KILL 150 "$VM" "$program" "$@" ); code=$?
fi
echo "exit $code"
