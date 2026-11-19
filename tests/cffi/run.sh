#!/bin/bash
# Calls to C, tested from Sharp: builds the test C library and runs every program of tests/cffi
# against it.   (Linux / WSL and Git Bash)
#
#     bash tests/cffi/run.sh                  the VM and the compiler of the last build (build-scripts/build.sh)
#     VM=<sharp> COMPILER=<sharpc> bash tests/cffi/run.sh
#
# The engine itself is tested without the VM by tests/cffi/engine_test.sh. The last line is
# CFFI: PASS or CFFI: FAIL.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
LIBRARY="$ROOT/lib/support/$(ls "$ROOT/lib/support" | sort -V | tail -1)"
if [ -n "$WINDIR" ]; then
    source "$ROOT/build-scripts/windows/mingw.sh"
    VM=${VM:-$ROOT/cmake-build-windows-vm/sharp.exe}
    COMPILER=${COMPILER:-$(ls "$LOCALAPPDATA"/Sharp/compiler/sharpc-v* 2>/dev/null | grep -v debug | sort -V | tail -1)}
    WORK=${WORK:-$ROOT/build/cffi-test}
    mkdir -p "$WORK"
    gcc -O2 -shared -static -DC_TEST_LIBRARY "$ROOT/tests/cffi/c_test_functions.c" -o "$WORK/c_test.dll" || exit 1
else
    VM=${VM:-$HOME/sharp_work/sharp_dev/sharp}
    COMPILER=${COMPILER:-$HOME/sharp_work/out/boot/sharpc_boot}
    WORK=${WORK:-$HOME/sharp_work/cffi-test}
    mkdir -p "$WORK"
    gcc -O2 -shared -fPIC -DC_TEST_LIBRARY "$ROOT/tests/cffi/c_test_functions.c" -o "$WORK/libc_test.so" -pthread || exit 1
fi
echo "calls to C: $("$VM" -V 2>&1 | head -1)"
bad=0
for source in ${ONLY:-calls structs mistakes}; do
    rm -f "$WORK/$source"
    "$VM" "$COMPILER" "$ROOT/tests/cffi/$source.sharp" -o "$WORK/$source" -L "$LIBRARY" > "$WORK/$source.compile.txt" 2>&1
    if [ ! -f "$WORK/$source" ]; then echo "FAIL  $source did not compile:"; grep -A12 "^error" "$WORK/$source.compile.txt" | head -40; bad=1; continue; fi
    for jit in "" -nojit; do
        ( cd "$WORK" && timeout 120 "$VM" $jit "$source" "$WORK" all > "$WORK/$source.txt" 2>&1 )
        if ! grep -q ": PASS" "$WORK/$source.txt"; then echo "FAIL  $source $jit:"; tail -30 "$WORK/$source.txt" | cut -c1-240; bad=1; else echo "$(tail -1 "$WORK/$source.txt")   [$source${jit:+ $jit}]"; fi
    done
done
# a crash inside C is reported as C's, with the function's name - not as a fault of the VM
if [ -f "$WORK/mistakes" ]; then
    for which in crash crash_array; do
        ( cd "$WORK" && timeout 120 "$VM" mistakes "$WORK" $which > "$WORK/$which.txt" 2>&1 )
        if grep -q "crashed inside the C function" "$WORK/$which.txt" && grep -q "= here: argument 1" "$WORK/$which.txt" && ! grep -q "contact the language developer" "$WORK/$which.txt"; then
            echo "the crash report names the C function   [$which]"
        else echo "FAIL  $which: the crash was not reported as a crash inside C:"; tail -12 "$WORK/$which.txt" | cut -c1-200; bad=1; fi
    done
fi
[ $bad = 0 ] && echo "CFFI: PASS" || { echo "CFFI: FAIL"; exit 1; }
