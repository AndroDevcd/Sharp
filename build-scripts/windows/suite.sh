#!/bin/bash
# Runs THE SUITE (tests/suite) on the WINDOWS VM: every case compiled and run four ways - plain and
# with -O, each with the machine-code compiler (the JIT) and with -nojit.   (Git Bash)
#
#     bash build-scripts/windows/suite.sh             the VM just built (cmake-build-windows-vm)
#     bash build-scripts/windows/suite.sh <sharp.exe> another VM
#
# test.sh runs the suite in WSL, on the Linux VM. This is the same check for the Windows VM - the one
# to run after a change to the JIT or to anything in the VM that differs by system. It needs an
# installed Sharp (its compiler compiles the cases, against the repository's library).
#
# The last line is  WINDOWS SUITE: PASS  or  WINDOWS SUITE: FAIL  with the cases that failed above it.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
VM=${1:-$ROOT/cmake-build-windows-vm/sharp.exe}
INSTALLED="$LOCALAPPDATA/Sharp"
# (COMPILER=<file>: another compiler - the one just built in WSL, when the library uses something the installed one does not know)
COMPILER=${COMPILER:-$(ls "$INSTALLED"/compiler/sharpc-v* 2>/dev/null | grep -v debug | sort -V | tail -1)}
LIBRARY="$ROOT/stdlib"
WORK=${WORK:-$ROOT/build/windows-suite}
JOBS=${TEST_JOBS:-$(nproc)}
[ -f "$VM" ] || { echo "no VM at $VM: run  bash build-scripts/windows/build_vm.sh  first"; exit 1; }
[ -f "$COMPILER" ] || { echo "no installed compiler in $INSTALLED/compiler: install a release first"; exit 1; }
rm -rf "$WORK"; mkdir -p "$WORK/plain" "$WORK/O"
echo "the Windows suite: $("$VM" -V 2>&1 | head -1), compiled by $(basename "$COMPILER"), $JOBS at a time"

# one case, one way of compiling: built once, run with the JIT and without. Prints a line only on failure.
one() {
    name=$1; kind=$2; option=""
    [ "$kind" = O ] && option="-O"
    program="$WORK/$kind/$name"
    "$VM" "$COMPILER" "$ROOT/tests/suite/cases/$name.sharp" "$ROOT/tests/suite/testing.sharp" -o "$program" $option -L "$LIBRARY" > "$program.compile.txt" 2>&1
    [ -f "$program" ] || { echo "FAIL  $name ($kind): did not compile, see $program.compile.txt"; return; }
    for jit in jit nojit; do
        flag=""; [ $jit = nojit ] && flag="-nojit"
        ( cd "$WORK/$kind" && timeout 120 "$VM" $flag "$program" > "$program.$jit.txt" 2>&1 )
        grep -q -e "RESULT PASS" -e " tests passed" "$program.$jit.txt" || echo "FAIL  $name ($kind, $jit): see $program.$jit.txt"
    done
}
export -f one; export VM COMPILER ROOT LIBRARY WORK

for kind in plain O; do
    ls "$ROOT/tests/suite/cases" | sed -n 's/\.sharp$//p' | xargs -P "$JOBS" -I{} bash -c 'one {} '$kind
done > "$WORK/failures.txt"

cases=$(ls "$ROOT/tests/suite/cases"/*.sharp | wc -l)
failed=$(grep -c "^FAIL" "$WORK/failures.txt")
sort "$WORK/failures.txt"
echo "$cases cases, each plain and -O, with the JIT and without: $((cases * 4 - failed)) passed, $failed failed"
[ "$failed" = 0 ] && echo "WINDOWS SUITE: PASS" || { echo "WINDOWS SUITE: FAIL"; exit 1; }
