#!/bin/bash
# TEST SHARP: builds the compiler, then runs the tests that say whether everything still works.   (Linux / WSL)
#
#     bash build-scripts/test.sh              the SHORT set - about 5 minutes. After any ordinary change.
#     bash build-scripts/test.sh full         every test - about an hour. Before a release that changed
#                                             code generation, the optimizer or the VM.
#     bash build-scripts/test.sh only 8       the build, then exactly the tests named (numbers below)
#     bash build-scripts/test.sh from 9       the build, then test 9 and every one after it
#     bash build-scripts/test.sh list         the tests and their numbers
#
# Run build.sh first when the VM (C++) changed: this script builds the compiler, not the VM.
#
# THE LAST LINE says how it went, and the script exits 1 on FAIL:
#     TESTS: PASS                       every test ran (full) and passed
#     TESTS: PASS (tests 1 2 4 ...)     the ones named ran and passed - the others were not run
#     TESTS: FAIL                       the test above it that reports failures names the cases
#
# THE TESTS
#      1  the compiler builds                                   (build.sh compiler)              10 s
#      2  the standard library compiles with no error                                            20 s
#      3  every suite case compiles (not run)                                                    8 min
#      4  frontend: reading and parsing source                  (lib/compiler/tests/run.sh)       1 min
#      5  backend: types, checks, the error messages            (tests/backend.sh)                1 min
#      6  helps: the fix every error message proposes compiles  (tests/helps.sh)                  5 min
#      7  code generation: small programs compiled, run, output compared   (tests/codegen.sh)     8 min
#      8  THE SUITE (tests/suite): every case compiled and RUN, four ways                        25 min
#      9  kind check: the programs of 8 on a VM that checks every value's kind       (needs 8)    5 min
#     10  kind check of the -O builds                                                (needs 8)    5 min
#     11  the repository's programs: benchmark/kostya and examples/   (tests/programs.sh)         3 min
#     12  driver: the command line, projects, the build cache   (tests/driver.sh)                 2 min
#     13  self-hosting: the compiler compiles itself, twice, to the same bytes  (tests/selfhost.sh) 20 min
#     14  the editor's files: what the CLion plugin reads       (tests/ide.sh)                    1 min
#     15  the native interface: Sharp calling C++               (tests/sni.sh)                    1 min
#
# The SHORT set is 1 2 4 5 12 14 15. WHICH TESTS A CHANGE NEEDS beyond it:
#     the standard library ............ 8
#     the compiler's analysis ......... 3 6
#     code generation, the optimizer .. 7 8 11 13
#     the VM (lib/runtime, lib/core) .. full
# When a full run fails at one test, fix the cause and rerun just that one (`only 9`) until it
# passes - then run `full` once more.
#
# TEST 8, THE SUITE, compiles and runs every case of tests/suite four ways:
#     plain         compiled normally,   run with the machine-code compiler (the JIT)
#     plain-nojit   compiled normally,   run by the interpreter alone
#     O             compiled with -O,    run with the JIT
#     O-nojit       compiled with -O,    run by the interpreter alone
# A bug in the optimizer shows as a case that passes plain and fails with O; a bug in the JIT as one
# that passes with -nojit only.   WAYS="plain" bash build-scripts/test.sh only 8   runs one way (6 min).
# What each case printed: ~/sharp_work/out/suite_runner/<way>/     (tests/suite/README.md: writing a case)
source "$(dirname "$0")/internal/common.sh"
SHORT=" 1 2 4 5 12 14 15 "
MODE=short; FROM=1; ONLY="$SHORT"; RAN=""
case "$1" in
  "")    ;;
  full)  MODE=full ;;
  from)  MODE=from; FROM=${2:?test.sh from <test number>} ;;
  only)  MODE=only; shift; ONLY=" $* " ;;
  list)  sed -n '/^# THE TESTS/,/^# The SHORT set/p' "$0" | sed 's/^# \{0,1\}//' | grep '^ *[0-9]'; exit 0 ;;
  *)     echo "test.sh: unknown argument '$1' (the top of the script lists them)"; exit 2 ;;
esac
# does this run include test $1?
want() {
  case $MODE in
    full)       ;;
    from)       [ $1 -ge $FROM ] || return 1 ;;
    short|only) [[ "$ONLY" == *" $1 "* ]] || return 1 ;;
  esac
  RAN="$RAN $1"; return 0
}
T="$REPO/lib/compiler/tests"
W="$WORK"
SHARP="$VM"
ok=1
[ -x "$VM" ] || fail "no Linux VM at $VM: run  bash build-scripts/build.sh  first"

echo "== [1] the compiler builds   (VM: $VM, $("$VM" -V 2>&1 | head -1))"
bash "$REPO/build-scripts/build.sh" compiler || { echo "TESTS: FAIL (the compiler does not build)"; exit 1; }
LIB=$(find "$STDLIB" -name "*.sharp" | sort)
RUN="$VM $COMPILER"
RAN="$RAN 1"
# every test script reads these
export SHARP COMPILER LIBRARY="$STDLIB"

if want 2; then echo "== [2] the standard library (0 errors, and the compile reaches its end)"
timeout -s KILL 600 $RUN -c $LIB -timings > $W/out/boot/lib.txt 2>&1
n=$(grep -c "^error" $W/out/boot/lib.txt)
grep -q "^analysis complete" $W/out/boot/lib.txt && done_=yes || done_=no
echo "library: $n error(s), reached the end: $done_   (whole output: $W/out/boot/lib.txt)"
# the compiler's own self-check: no expression of the library may be left without a type and without a message
grep -q "^unknown: 0 " $W/out/boot/lib.txt || { echo "library: expressions without a type and without a message (the 'unknown:' line must say 0)"; ok=0; }
if [ "$n" != 0 ] || [ $done_ = no ]; then ok=0; grep "^error" $W/out/boot/lib.txt | sed 's/`[^`]*`/X/g' | sort | uniq -c | sort -rn | head -15; tail -4 $W/out/boot/lib.txt; fi
fi

if want 3; then echo "== [3] the $(ls $W/suite/cases/*.sharp | wc -l) suite cases compile (each with the library and testing.sharp; not run)"
mkdir -p $W/out/boot/cases; rm -f $W/out/boot/cases/*
export W LIB
ls $W/suite/cases/*.sharp | xargs -P 4 -I{} bash -c 'f={}; b=$(basename $f .sharp); timeout -s KILL 300 $SHARP $COMPILER -c $LIB $W/suite/testing.sharp $f > $W/out/boot/cases/$b.txt 2>&1'
# A VM crash (SIGSEGV) while compiling is a FAILURE. The case is compiled once more, alone, so the
# report says whether the crash comes back - but it counts as failing either way.
crashes=0
for f in $W/out/boot/cases/*.txt; do
  if ! grep -q "^analysis complete" $f && ! grep -q "^error" $f && grep -q "SIGSEGV\|Unhandled exception" $f; then
    b=$(basename $f .txt); crashes=$((crashes+1))
    cp $f $W/out/boot/cases/$b.crash
    timeout -s KILL 300 $RUN -c $LIB $W/suite/testing.sharp $W/suite/cases/$b.sharp > $W/out/boot/cases/$b.again 2>&1
    grep -q "^analysis complete" $W/out/boot/cases/$b.again && again="passed" || again="FAILED again"
    echo "VM CRASH on $b (output kept: $W/out/boot/cases/$b.crash); compiled again alone: $again"
  fi
done
bad=0; total=0
for f in $W/out/boot/cases/*.txt; do total=$((total+1)); if ! grep -q "^analysis complete" $f || grep -q "^error" $f; then bad=$((bad+1)); [ $bad -le 8 ] && { echo "FAIL $(basename $f .txt):"; grep -A6 "^error" $f | head -9 | cut -c1-220; tail -2 $f | cut -c1-200; }; fi; done
echo "suite cases: $total compiled, $bad failing, vm crashes: $crashes   (outputs: $W/out/boot/cases/<name>.txt)"
[ $bad = 0 ] || ok=0
fi

if want 4; then echo "== [4] frontend"
out=$(bash "$T/run.sh" 2>&1) || ok=0; echo "$out" | tail -12
fi

if want 5; then echo "== [5] backend"
out=$(bash "$T/backend.sh" 2>&1) || ok=0; echo "$out" | tail -30
fi

if want 6; then echo "== [6] helps (the text every tested help proposes, pasted in, must compile)"
out=$(WORK="$W/out/helps" bash "$T/helps.sh" 2>&1) || ok=0; echo "$out" | tail -20
fi

if want 7; then echo "== [7] code generation (programs compiled and RUN, with the JIT and with -nojit, compared with the output they"
echo "   must print; a program that must be refused gives its error; every program once more with -O)"
out=$(bash "$T/codegen.sh" 2>&1) || ok=0; echo "$out" | tail -20
fi

if want 8; then echo "== [8] the suite, compiled and run: ${WAYS:-plain plain-nojit O O-nojit}"
out=$(OUT=$W/out/suite_runner SUITE=$W/suite CONFIGURATIONS="${WAYS:-plain plain-nojit O O-nojit}" bash "$T/suite_runner.sh" 2>&1) || ok=0; echo "$out" | tail -30
fi

if want 9; then echo "== [9] kind check (the programs test 8 built, run on a VM built with -DSHARP_TYPE_CHECK: no report allowed)"
out=$(TC=$W/sharp_tc_dev/sharp bash "$T/kindcheck.sh" $W/out/suite_runner/plain 2>&1) || ok=0; echo "$out" | tail -25
fi

if want 10; then echo "== [10] kind check of the -O builds (the optimizer must not make a value read as the wrong kind either)"
out=$(TC=$W/sharp_tc_dev/sharp bash "$T/kindcheck.sh" $W/out/suite_runner/O 2>&1) || ok=0; echo "$out" | tail -25
fi

if want 11; then echo "== [11] the repository's programs (benchmark/kostya, examples/): the same output plain and with -O, JIT and -nojit"
out=$(bash "$T/programs.sh" 2>&1) || ok=0; echo "$out" | tail -20
fi

if want 12; then echo "== [12] driver (the command line, the output file and exit code, projects and settings.json, the build cache,"
echo "   compilers started side by side)"
out=$(bash "$T/driver.sh" 2>&1) || ok=0; echo "$out" | tail -20
fi

if want 13; then echo "== [13] self-hosting (the compiler compiles itself: the second and third generation are the same bytes, also on"
echo "   4 threads and with -O; the second generation then passes the library, the frontend and backend tests, and"
echo "   writes every suite case as the same bytes as the first)"
out=$(SOURCES=$W/src/lib/compiler SUITE=$W/suite bash "$T/selfhost.sh" 2>&1) || ok=0; echo "$out" | tail -40
fi

if want 14; then echo "== [14] the editor's files (what -ide writes for the CLion plugin: diagnostics, declarations, types, bodies)"
out=$(bash "$T/ide.sh" 2>&1) || ok=0; echo "$out" | tail -20
fi

if want 15; then echo "== [15] the native interface (Sharp calls C++ built from the files -sni writes)"
out=$(bash "$T/sni.sh" 2>&1) || ok=0; echo "$out" | tail -20
fi

if [ $ok != 1 ]; then echo "TESTS: FAIL"; exit 1
elif [ $MODE = full ]; then echo "TESTS: PASS"
else echo "TESTS: PASS (tests$RAN) - the others were not run"
fi
