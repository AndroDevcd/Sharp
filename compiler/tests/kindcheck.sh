#!/bin/bash
# Programs built by the bootstrap compiler, run on the KIND-CHECK runtime (doc/codegen.md 5.7,
# 13.5): a build of runtime with -DSHARP_TYPE_CHECK (runtime/memory/type_check.h), in
# which the interpreter checks at every instruction that a register or stack slot read as a
# whole number holds a whole number, one read as a decimal a decimal, one read as an object an
# object. It prints `[type check] ...` for every mismatch and goes on. A correct code generator
# gives it nothing to print.
#
#   tests/kindcheck.sh <the folder test 8 built the suite into: ~/sharp_work/out/suite_runner/plain or O>
#       TC=<the kind-check runtime>   (default ~/sharp_work/sharp_tc_dev/sharp)
#       SOURCES=<the copy of the repository the runtime is built from>  (default ~/sharp_work/src)
#       SUITE=<the suite's project folder, where its program runs>   (default ~/sharp_work/suite)
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  LIBRARY=<the standard library>
#       (for the probe; defaults ~/sharp_work/sharp_dev/sharp, ~/sharp_work/out/boot/sharpc_boot,
#        ~/sharp_work/stdlib)
#
# Three steps:
#   1. THE RUNTIME IS UP TO DATE. When a source file of the runtime is newer than the kind-check
#      runtime (or there is none yet), it is built again - an incremental build, a few files
#      after a small change - with
#           SOURCES=$SOURCES OUT=<folder of $TC> bash build-scripts/linux/build_vm.sh -O3 -DSHARP_TYPE_CHECK
#      A check on a runtime older than runtime would be a check of something else.
#   2. THE CHECKER IS AWAKE. tests/kindcheck_probe.sharp, a program that adds a decimal as a
#      whole number in an asm block, is built by the bootstrap and run: it MUST give a report.
#      Without this, a runtime built without the flag would pass everything.
#   3. The suite's program in the folder (<folder>/suite, built by tests/suite_runner.sh) is run
#      with -nojit (the checks are in the interpreter; machine code is not checked), in the suite's
#      folder. It must print no `[type check]` line and still pass: exit code 0 and `all N tests
#      passed` (suite_program.sh; a run that dies without its summary is run again one top-level
#      group at a time). The checks make the interpreter slower still: a run is killed after 3600 s.
#      What it printed: <folder>/kindcheck.txt.
#
# Prints the reports (at most 20) and a summary; exits 1 when there was any report or failure.
FOLDER=${1:?usage: kindcheck.sh <the folder test 8 built the suite into>}
HERE="$(cd "$(dirname "$0")" && pwd)"
TC=${TC:-${SHARP_WORK:-$HOME/sharp_work}/sharp_tc_dev/sharp}
SOURCES=${SOURCES:-${SHARP_WORK:-$HOME/sharp_work}/src}
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
SUITE=${SUITE:-${SHARP_WORK:-$HOME/sharp_work}/suite}
source "$HERE/suite_program.sh"

# ---- 1: the kind-check runtime is not older than the runtime's sources
newer_sources() {
    [ -x "$TC" ] || { echo "(no runtime at $TC)"; return; }
    find "$SOURCES/runtime" -type f \( -name '*.cpp' -o -name '*.h' \) -newer "$TC" 2>/dev/null | head -3
}
if [ -n "$(newer_sources)" ]; then
    # never build into a folder that holds a runtime of another kind (build_vm.sh keeps the
    # flags of a folder's last build in flags.txt)
    if [ -f "$(dirname "$TC")/flags.txt" ] && ! grep -q "SHARP_TYPE_CHECK" "$(dirname "$TC")/flags.txt"; then
        echo "kind check: $TC is older than the runtime's sources and was not built with -DSHARP_TYPE_CHECK; not rebuilding it"; exit 1
    fi
    echo "kind check: the kind-check runtime is older than the runtime's sources ($(newer_sources | head -1 | sed "s|$SOURCES/||") ...); building it again"
    SOURCES="$SOURCES" OUT="$(dirname "$TC")" bash "$HERE/../../build-scripts/linux/build_vm.sh" -O3 -DSHARP_TYPE_CHECK 2>&1 | tail -3
    if [ -n "$(newer_sources)" ]; then echo "kind check: the kind-check runtime could not be built from $SOURCES"; exit 1; fi
fi

# ---- 2: the checker is awake
PROBE="$(dirname "$TC")/kindcheck_probe"
rm -f "$PROBE"
timeout -s KILL 300 "$SHARP" "$COMPILER" "$HERE/kindcheck_probe.sharp" -j 1 -o "$PROBE" -L "$LIBRARY" > "$PROBE.compile.txt" 2>&1
if [ ! -f "$PROBE" ]; then
    echo "kind check: the probe (tests/kindcheck_probe.sharp) did not compile:"; grep -m1 -A3 "error" "$PROBE.compile.txt" | head -4; exit 1
fi
if ! timeout -s KILL 60 prlimit --core=0 "$TC" -nojit "$PROBE" 2>&1 | grep -a -q '^\[type check\].*holds a decimal, an integer is needed'; then
    echo "kind check: the probe gave NO report on $TC - this runtime does not check kinds (was it built with -DSHARP_TYPE_CHECK?)"; exit 1
fi

# ---- 3: the program
[ -f "$FOLDER/suite" ] || { echo "kind check: no program at $FOLDER/suite - run test 8 first (bash build-scripts/test.sh only 8 9)"; exit 1; }
rm -f "$FOLDER"/kindcheck*.txt
verdict=$(suite_way "kind check, $(basename "$FOLDER")" "$SUITE" "$FOLDER/kindcheck.txt" 3600 prlimit --core=0 "$TC" -nojit "$FOLDER/suite"); passed=$?
# (the reports of every run: the whole one, and the runs of single groups after a crash)
reports=$(cat "$FOLDER"/kindcheck*.txt 2>/dev/null | grep -a '^\[type check\]')
echo "$reports" | grep -v '^$' | head -20 | cut -c1-200
echo "$verdict"
with_reports=$(echo "$reports" | grep -c '^\[type check\]')
echo "kind check: $with_reports report(s); the suite $([ $passed = 0 ] && echo "passed" || echo "did NOT pass") on the kind-check runtime"
[ "$with_reports" = 0 ] && [ $passed = 0 ]
