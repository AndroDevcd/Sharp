#!/bin/bash
# Runs THE SUITE (tests/suite) on the WINDOWS VM: its one program built twice - plain and with -O -
# and each run with the machine-code compiler (the JIT) and with -nojit.   (Git Bash)
#
#     bash build-scripts/windows/suite.sh             the VM just built (cmake-build-windows-vm)
#     bash build-scripts/windows/suite.sh <sharp.exe> another VM
#
# test.sh runs the suite in WSL, on the Linux VM. This is the same check for the Windows VM - the one
# to run after a change to the JIT or to anything in the VM that differs by system. It needs an
# installed Sharp (its compiler builds the suite, against the repository's library).
#
# The suite is built in its own folder (tests/suite/build, deleted first) and runs there, one run at
# a time. A run passes when its exit code is 0 and its last line is `all N tests passed`; a run that
# dies without its summary is run again one top-level group at a time, which names the group
# (compiler/tests/suite_program.sh). A run is killed after 900 s with the JIT, 1800 s without.
# What each run printed: build/windows-suite/<plain|O>/run-<jit|nojit>.txt.
#
# The last line is  WINDOWS SUITE: PASS  or  WINDOWS SUITE: FAIL  with what failed above it.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
source "$ROOT/compiler/tests/suite_program.sh"
VM=${1:-$ROOT/cmake-build-windows-vm/sharp.exe}
INSTALLED="$LOCALAPPDATA/Sharp"
# (COMPILER=<file>: another compiler - the one just built in WSL, when the library uses something the installed one does not know)
COMPILER=${COMPILER:-$(ls "$INSTALLED"/compiler/sharpc-v* 2>/dev/null | grep -v debug | sort -V | tail -1)}
LIBRARY="$ROOT/stdlib"
PROJECT="$ROOT/tests/suite"
WORK=${WORK:-$ROOT/build/windows-suite}
[ -f "$VM" ] || { echo "no VM at $VM: run  bash build-scripts/windows/build_vm.sh  first"; exit 1; }
[ -f "$COMPILER" ] || { echo "no installed compiler in $INSTALLED/compiler: install a release first"; exit 1; }
rm -rf "$WORK"; mkdir -p "$WORK/plain" "$WORK/O"
echo "the Windows suite: $("$VM" -V 2>&1 | head -1), compiled by $(basename "$COMPILER")"

passed=0; failed=0
for kind in plain O; do
    option=""; [ "$kind" = O ] && option="-O"
    if ! suite_build "$PROJECT" "$WORK/$kind/suite" "$WORK/$kind/compile.txt" "$VM" "$COMPILER" -P "$PROJECT" -j 1 -stdlib "$LIBRARY" $option; then
        echo "FAIL  the suite ($kind): did not compile, see $WORK/$kind/compile.txt"; failed=$((failed + 2)); continue
    fi
    for jit in jit nojit; do
        flag=""; seconds=900; [ $jit = nojit ] && { flag="-nojit"; seconds=1800; }
        if suite_way "the suite ($kind, $jit)" "$PROJECT" "$WORK/$kind/run-$jit.txt" $seconds "$VM" $flag "$WORK/$kind/suite"
        then passed=$((passed + 1)); else failed=$((failed + 1)); fi
    done
done

echo "the suite, plain and -O, with the JIT and without: $passed of 4 runs passed, $failed failed"
[ "$failed" = 0 ] && echo "WINDOWS SUITE: PASS" || { echo "WINDOWS SUITE: FAIL"; exit 1; }
