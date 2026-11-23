#!/bin/bash
# THE ARM JIT, WITHOUT AN ARM MACHINE: the VM is built for ARM (64 and 32 bit), and the suite's cases -
# compiled on this machine by test 8 - are run on it under qemu, with its JIT.
# A PART OF build-scripts/test.sh (test 17; needs test 8).   (Linux / WSL)
#
#     internal/arm_suite.sh                 both, the cases compiled with -O
#     internal/arm_suite.sh 64              one of them
#     internal/arm_suite.sh 32 jit_numbers lib_math_sqrt_instruction        only these cases
#
#     WAY=plain     the cases compiled without -O      (default O)
#     VM_OPTIONS=-nojit    options for the VM
#
# What is needed, and what happens without it: the cross compilers (aarch64-linux-gnu-g++,
# arm-linux-gnueabihf-g++) and qemu-user (qemu-aarch64, qemu-arm). A machine that has not got them
# skips the test and says so - it is not a failure.
#     sudo apt install g++-aarch64-linux-gnu g++-arm-linux-gnueabihf qemu-user
#
# Left out: the cases that start the VM once more as a program of its own (qemu runs one program;
# the child would be an ARM program started without it).
source "$(dirname "$0")/common.sh"
WAY=${WAY:-O}
CASES=$WORK/out/suite_runner/$WAY
STARTS_ITSELF=" lib_system_subprocess lib_system_signals lib_testing_ide lib_io_print_is_seen "

which=""
case "$1" in 64|32) which=$1; shift ;; esac
wanted="$*"
[ -d "$CASES" ] || fail "ARM: no compiled cases in $CASES - run test 8 first (bash build-scripts/test.sh only 8 17)"

copy_sources || exit 1          # (the VM is compiled from the copy on Linux's own disk: common.sh)
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
    names=$wanted
    [ -z "$names" ] && names=$(cd "$CASES" && ls | grep -v '\.')
    export vm_folder CASES out qemu triple VM_OPTIONS
    run=""
    for n in $names; do case "$STARTS_ITSELF" in *" $n "*) ;; *) run="$run $n" ;; esac; done
    ( cd "$WORK" && echo $run | tr ' ' '\n' | xargs -P "${TEST_JOBS:-8}" -I{} bash -c 'n={}
        timeout -s KILL 600 $qemu -L /usr/$triple "$vm_folder/sharp" $VM_OPTIONS "$CASES/$n" > "$out/$n.txt" 2>&1 < /dev/null' )
    total=0; bad=0
    for n in $run; do
        total=$((total + 1))
        if ! grep -q "^RESULT PASS" "$out/$n.txt"; then
            bad=$((bad + 1))
            echo "FAIL  $n on ARM$bits: $(grep -v '^RESULT PASS' "$out/$n.txt" | tail -3 | cut -c1-160 | tr '\n' '|')"
        fi
    done
    echo "ARM$bits ($WAY, under $qemu): $((total - bad)) of $total cases passed   (what each printed: $out)"
    [ $bad = 0 ] || ok=0
done
[ $ok = 1 ]
