#!/bin/bash
# Programs built by the bootstrap compiler, run on the KIND-CHECK runtime (doc/codegen.md 5.7,
# 13.5): a build of runtime with -DSHARP_TYPE_CHECK (runtime/memory/type_check.h), in
# which the interpreter checks at every instruction that a register or stack slot read as a
# whole number holds a whole number, one read as a decimal a decimal, one read as an object an
# object. It prints `[type check] ...` for every mismatch and goes on. A correct code generator
# gives it nothing to print.
#
#   tests/kindcheck.sh <folder of executables>
#       TC=<the kind-check runtime>   (default ~/sharp_work/sharp_tc_dev/sharp)
#       SOURCES=<the copy of the repository the runtime is built from>  (default ~/sharp_work/src)
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
#   3. Every executable in the folder (a file without a dot in its name - the suite runner's
#      work folder has exactly those: <case>, <case>.compile.txt, <case>.out.txt) is run with
#      -nojit (the checks are in the interpreter; machine code is not checked), four at a time.
#      Each must print no `[type check]` line and still print RESULT PASS.
#
# Prints the reports (at most 20) and a summary; exits 1 when there was any report or failure.
FOLDER=${1:?usage: kindcheck.sh <folder of executables>}
HERE="$(cd "$(dirname "$0")" && pwd)"
TC=${TC:-${SHARP_WORK:-$HOME/sharp_work}/sharp_tc_dev/sharp}
SOURCES=${SOURCES:-${SHARP_WORK:-$HOME/sharp_work}/src}
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}

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

# ---- 3: the programs
export TC
check_one() {
    exe=$1
    out=$(timeout -s KILL 300 prlimit --core=0 "$TC" -mem 1gb -nojit "$exe" 2>&1)
    reports=$(echo "$out" | grep -a -c '^\[type check\]')
    echo "$out" | grep -a '^\[type check\]' | head -3 | sed "s|\$|   <- $(basename "$exe")|"
    if [ "$reports" != 0 ]; then echo "REPORTS $(basename "$exe") $reports"
    elif ! echo "$out" | grep -a -q "RESULT PASS"; then echo "NOT-PASSED $(basename "$exe")"
    else echo "CLEAN $(basename "$exe")"; fi
}
export -f check_one
results=$(find "$FOLDER" -maxdepth 1 -type f ! -name "*.*" | sort | xargs -P ${TEST_JOBS:-4} -I{} bash -c 'check_one {}')
echo "$results" | grep '^\[type check\]' | head -20 | cut -c1-200
echo "$results" | grep '^NOT-PASSED' | head -10
clean=$(echo "$results" | grep -c '^CLEAN')
with_reports=$(echo "$results" | grep -c '^REPORTS')
not_passed=$(echo "$results" | grep -c '^NOT-PASSED')
echo "kind check: $clean clean, $with_reports with reports, $not_passed did not pass on the kind-check runtime"
[ "$with_reports" = 0 ] && [ "$not_passed" = 0 ] && [ "$clean" -gt 0 ]
