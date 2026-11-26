#!/bin/bash
# Builds and runs the test of the C-call engine (runtime/cffi) on its own, without the VM:
# seconds, not minutes. Run it from Git Bash (the Windows convention) and from WSL / Linux (the
# Linux one); each prints  ENGINE: PASS (n checks)  or the checks that failed.
#
#     bash tests/cffi/engine_test.sh                 this machine's g++
#     CXX=aarch64-linux-gnu-g++ CC=aarch64-linux-gnu-gcc RUN=qemu-aarch64 bash tests/cffi/engine_test.sh
#                                                    another processor: a cross compiler and what runs its programs
#
# It is built twice, -O0 and -O2: a call that only works at one of them is a bug in how it is made.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
if [ -z "$CXX" ] && [ -f "$ROOT/build-scripts/windows/mingw.sh" ] && [ -n "$WINDIR" ]; then source "$ROOT/build-scripts/windows/mingw.sh"; fi
CXX=${CXX:-g++}; CC=${CC:-gcc}
OUT=${OUT:-$ROOT/build/cffi-test}
mkdir -p "$OUT"
bad=0
for level in -O0 -O2; do
    $CC $level -w $FLAGS -c "$ROOT/tests/cffi/c_test_functions.c" -o "$OUT/c_test_functions.o" || exit 1
    $CXX -std=c++17 $level -w $FLAGS $CRT_FLAG "$ROOT/tests/cffi/engine_test.cpp" "$ROOT/runtime/cffi/c_call.cpp" "$OUT/c_test_functions.o" \
        -o "$OUT/engine_test" $LINK -pthread || exit 1
    echo "--- $level"
    $RUN "$OUT/engine_test" || bad=1
done
exit $bad
