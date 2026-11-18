#!/bin/bash
# The suite, judged by its own runner, with the BOOTSTRAP compiler as the compiler under test
# (doc/codegen.md 5.8 and 13.5).
#
#   tests/suite_runner.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  SHARPC=<the C++ compiler>
#       LIBRARY=<the standard library folder>  SUITE=<the suite folder>  OUT=<a folder to work in>
#       (defaults: ~/sharp_work/sharp_dev/sharp, ~/sharp_work/out/boot/sharpc_boot,
#        ~/sharp_work/sharpc_cl/sharpc, ~/sharp_work/stdlib, ~/sharp_work/suite,
#        ~/sharp_work/out/boot/suite_runner)
#
# This is exactly what ~/sharp_work/tools/suite.sh does for the C++ build, with one thing
# changed: the <sharpc> the runner is given. The runner (tests/suite/run.sharp, built by the
# C++ compiler, so the judge is not the thing judged) compiles every case as
#
#       <sharpc> <case>.sharp testing.sharp -o <exe> -j 1 -L <library>
#
# runs it under `timeout 120`, and counts a PASS only when the program printed RESULT PASS:
# a compile that writes no file, a crash, a hang or a failed check is a FAIL. Here <sharpc> is
# a two-line wrapper that runs the bootstrap compiler on the VM. So the bootstrap must take its
# library from `-L` (every .sharp file below the folder) and accept `-j`, as the C++ compiler
# does.
#
# Four configurations, as suite.sh's four: `plain` (the JIT), `plain-nojit`, and the same two
# with every case compiled with -O (the optimizer, doc/codegen.md 13.6: `O`, `O-nojit`; the
# runner hands -O to every compile, after `-j 1 -L <library>`). One after the other, NOT side
# by side: several cases work in fixed folders of their own under /tmp (lib_io_tree in
# /tmp/sharp_suite_io_tree, lib_data_json in /tmp/sharp_suite_io_json ...), and two runs of one
# case at once delete each other's files - tried, and two cases failed that way.
# CONFIGURATIONS="plain O" runs only those.
#
# VM CRASHES are counted on their own: a case whose program died on a signal (SIGSEGV in its
# output), and a compile whose compiler - the bootstrap, itself a program on the VM - died
# before it finished (SIGSEGV or "Unhandled exception" in the compile log, no "analysis
# complete"). Each is a FAIL like any other and is never re-run until it passes; the count only
# says what kind of failure it was. The runtime stage fixed the one intermittent crash this
# machine had (lib/runtime/virtual_machine.cpp, continues_in_machine_code), so a new one is a
# runtime bug to report. (An uncaught exception of the PROGRAM is not a crash: it is a failed
# case, and the runner says so.)
#
# Prints one summary line per configuration and exits 1 when any case failed.
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
SHARPC=${SHARPC:-}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
SUITE=${SUITE:-${SHARP_WORK:-$HOME/sharp_work}/suite}
OUT=${OUT:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/suite_runner}
mkdir -p "$OUT"

# the compiler under test, as one command the runner can call
WRAPPER="$OUT/sharpc_boot.sh"
printf '#!/bin/bash\nexec "%s" "%s" "$@"\n' "$SHARP" "$COMPILER" > "$WRAPPER"
chmod +x "$WRAPPER"

# the judge: the runner, built by the C++ compiler (once more if that compiler dies on a signal,
# which it does now and then under load)
# No second compiler (the C++ one is gone since 17 Nov 2026; SHARPC=<another compiler> names one):
# the compiler under test builds the runner. The runner is a small program that the frontend,
# backend and driver tests already cover, so a compiler broken enough to build a lying runner
# fails long before this step.
[ -x "$SHARPC" ] || SHARPC="$WRAPPER"
rm -f "$OUT/run_tests"
(cd "$SUITE" && "$SHARPC" run.sharp -j 1 -o "$OUT/run_tests" -L "$LIBRARY" > "$OUT/runner_compile.txt" 2>&1)
[ -f "$OUT/run_tests" ] || (cd "$SUITE" && "$SHARPC" run.sharp -j 1 -o "$OUT/run_tests" -L "$LIBRARY" > "$OUT/runner_compile.txt" 2>&1)
[ -f "$OUT/run_tests" ] || { echo "the suite runner did not compile:"; grep -m5 -A3 error "$OUT/runner_compile.txt"; exit 1; }

CONFIGURATIONS=${CONFIGURATIONS:-plain plain-nojit O O-nojit}
run_configuration() {   # $1 = name, $2 = the runtime command the runner runs each case with, $3 = compiler options
    work="$OUT/$1"
    rm -rf "$work"; mkdir -p "$work"
    "$SHARP" "$OUT/run_tests" "$WRAPPER" "$2" "$SUITE" "$work" -j 1 -L "$LIBRARY" $3 > "$work/run.log" 2>&1
}
for name in $CONFIGURATIONS; do
    case "$name" in
        plain)       run_configuration plain "$SHARP -mem 1gb" "" ;;
        plain-nojit) run_configuration plain-nojit "$SHARP -mem 1gb -nojit" "" ;;
        O)           run_configuration O "$SHARP -mem 1gb" "-O" ;;
        O-nojit)     run_configuration O-nojit "$SHARP -mem 1gb -nojit" "-O" ;;
        *)           echo "unknown configuration $name"; exit 1 ;;
    esac
done

ok=1
for name in $CONFIGURATIONS; do
    log="$OUT/$name/run.log"
    verdict=$(grep -E 'TESTS PASSED|TESTS FAILED' "$log")
    program_crashes=$(grep -l -a "SIGSEGV" "$OUT/$name"/*.out.txt 2>/dev/null | wc -l)
    compiler_crashes=0
    for compile_log in "$OUT/$name"/*.compile.txt; do
        [ -f "$compile_log" ] || continue
        grep -q "^analysis complete" "$compile_log" && continue
        grep -q -a "SIGSEGV\|Unhandled exception" "$compile_log" && compiler_crashes=$((compiler_crashes+1))
    done
    echo "suite runner, $name: ${verdict:-the runner did not finish (see $log)}; vm crashes: $program_crashes in programs, $compiler_crashes in the compiler"
    grep '^FAIL' "$log" | cut -c1-160 | head -20
    grep -q "^ALL [0-9]* TESTS PASSED" "$log" || ok=0
done
[ $ok = 1 ]
