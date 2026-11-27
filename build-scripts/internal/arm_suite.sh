#!/bin/bash
# THE ARM JIT, WITHOUT AN ARM MACHINE: the VM is built for ARM (64 and 32 bit), and the suite's program -
# compiled on this machine by test 8 - is run on it under qemu, with its JIT.
# A PART OF build-scripts/test.sh (test 17; needs test 8).   (Linux / WSL)
#
#     internal/arm_suite.sh                 both, the program compiled with -O
#     internal/arm_suite.sh 64              one of them
#     internal/arm_suite.sh 32 suite.jit suite.library.math        only these groups
#
#     WAY=plain     the program compiled without -O      (default O)
#     VM_OPTIONS=-nojit    options for the VM
#
# What is needed, and what happens without it: the cross compilers (aarch64-linux-gnu-g++,
# arm-linux-gnueabihf-g++) and qemu-user (qemu-aarch64, qemu-arm). A machine that has not got them
# skips the test and says so - it is not a failure.
#     sudo apt install g++-aarch64-linux-gnu g++-arm-linux-gnueabihf qemu-user
#
# Left out: the groups whose file starts the program once more as a process of its own (qemu runs
# one program; the child would be an ARM program started without it). They are found in the sources
# (compiler/tests/suite_program.sh: run_isolated, program_path(), runtime_path()) and named below the
# result; every other group runs (-test-only for each).
#
# The program must pass: exit code 0 and `all N tests passed`; a run that dies without its summary
# is run again one top-level group at a time, which names the group. A run is killed after 3600 s.
source "$(dirname "$0")/common.sh"
source "$REPO/compiler/tests/suite_program.sh"
WAY=${WAY:-O}
PROGRAM=$WORK/out/suite_runner/$WAY/suite

which=""
case "$1" in 64|32) which=$1; shift ;; esac
wanted="$*"
[ -f "$PROGRAM" ] || fail "ARM: no compiled suite at $PROGRAM - run test 8 first (bash build-scripts/test.sh only 8 17)"

copy_sources || exit 1          # (the VM is compiled from the copy on Linux's own disk: common.sh)
# the groups to run: the ones named, else everything but the groups that start the program again
if [ -n "$wanted" ]; then pick=$wanted; left_out=""
else
    pick=$(suite_without_self_starters "$WORK/suite" 2> "$WORK/out/arm_left_out.txt")
    left_out=$(sed 's/^left out: //' "$WORK/out/arm_left_out.txt" | tr '\n' ' ')
fi
ok=1
for bits in ${which:-64 32}; do
    if [ "$bits" = 32 ]; then triple=arm-linux-gnueabihf; qemu=qemu-arm; else triple=aarch64-linux-gnu; qemu=qemu-aarch64; fi
    if ! command -v $triple-g++ > /dev/null || ! command -v $qemu > /dev/null; then
        echo "ARM$bits: not run ($triple-g++ or $qemu is not installed)"
        continue
    fi
    vm_folder=$WORK/arm$bits-vm
    CXX=$triple-g++ OUT="$vm_folder" SOURCES="$WORK/src" bash "$REPO/build-scripts/linux/build_vm.sh" -Wno-psabi > "$WORK/out/arm$bits-build.txt" 2>&1
    if [ ! -x "$vm_folder/sharp" ]; then tail -15 "$WORK/out/arm$bits-build.txt"; echo "ARM$bits: the VM does not build"; ok=0; continue; fi

    out=$WORK/out/arm_suite/$bits-$WAY; rm -rf "$out"; mkdir -p "$out"
    SUITE_PICK="$pick" suite_way "ARM$bits ($WAY, under $qemu)" "$WORK/suite" "$out/run.txt" 3600 \
        $qemu -L /usr/$triple "$vm_folder/sharp" $VM_OPTIONS "$PROGRAM" || ok=0
    echo "   (what it printed: $out)"
done
[ -n "$left_out" ] && echo "left out on ARM (they start the program once more): $left_out"
[ $ok = 1 ]
