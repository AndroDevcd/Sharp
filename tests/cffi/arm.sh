#!/bin/bash
# Calls to C on ARM, on a PC: the engine test and the whole VM, built for ARM by a cross compiler and
# run under qemu.   (Linux / WSL)
#
#     bash tests/cffi/arm.sh              both: 32-bit (hard float, a Raspberry Pi's) and 64-bit
#     bash tests/cffi/arm.sh 32           one of them
#
# Needs:  sudo apt install qemu-user g++-arm-linux-gnueabihf g++-aarch64-linux-gnu
#
# For each: the engine test (tests/cffi/engine_test.sh), then the VM is built for that processor
# (build-scripts/linux/build_vm.sh with the cross compiler - with the ARM machine-code compiler, lib/runtime/jit/arm;
# SHARP_NO_JIT=1 leaves it out) and tests/cffi/run.sh runs every Sharp-side test
# with it. The compiler that compiles the tests is a Sharp program too: it runs on the ARM VM.
# The last line is ARM: PASS or ARM: FAIL.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK=${ARM_WORK:-$HOME/sharp_work/arm}
SOURCES=${SOURCES:-$HOME/sharp_work/src}
[ -d "$SOURCES/lib/runtime" ] || SOURCES="$ROOT"
bad=0
for bits in ${1:-32 64}; do
    if [ "$bits" = 32 ]; then triple=arm-linux-gnueabihf; qemu=qemu-arm; else triple=aarch64-linux-gnu; qemu=qemu-aarch64; fi
    command -v $qemu > /dev/null && command -v $triple-g++ > /dev/null || { echo "ARM$bits: $qemu or $triple-g++ is not installed (see the top of this file)"; bad=1; continue; }
    mkdir -p "$WORK/$bits"
    echo "=== ARM$bits: the engine"
    CC=$triple-gcc CXX=$triple-g++ FLAGS=-Wno-psabi RUN="$qemu -L /usr/$triple" OUT="$WORK/$bits/engine" bash "$ROOT/tests/cffi/engine_test.sh" > "$WORK/$bits/engine.txt" 2>&1
    grep -E "ENGINE|FAIL|error" "$WORK/$bits/engine.txt"
    [ "$(grep -c "ENGINE: PASS" "$WORK/$bits/engine.txt")" = 2 ] || bad=1
    echo "=== ARM$bits: the VM"
    CXX=$triple-g++ OUT="$WORK/$bits/vm" SOURCES="$SOURCES" ${SHARP_NO_JIT:+SHARP_NO_JIT=1} bash "$ROOT/build-scripts/linux/build_vm.sh" -Wno-psabi 2>&1 | grep -E "FAILED|error|building|compiled|built" | cut -c1-200
    [ -f "$WORK/$bits/vm/sharp" ] || { echo "ARM$bits: the VM did not build"; bad=1; continue; }
    # `sharp` for the tests: the ARM VM under qemu
    printf '#!/bin/bash\nexec %s -L /usr/%s "%s" "$@"\n' "$qemu" "$triple" "$WORK/$bits/vm/sharp" > "$WORK/$bits/sharp"
    chmod +x "$WORK/$bits/sharp"
    echo "=== ARM$bits: from Sharp"
    VM="$WORK/$bits/sharp" CC=$triple-gcc WORK="$WORK/$bits/tests" TIME=${TIME:-900} bash "$ROOT/tests/cffi/run.sh" > "$WORK/$bits/tests.txt" 2>&1
    cut -c1-200 "$WORK/$bits/tests.txt"
    grep -q "^CFFI: PASS" "$WORK/$bits/tests.txt" || bad=1
done
[ $bad = 0 ] && echo "ARM: PASS" || { echo "ARM: FAIL"; exit 1; }
