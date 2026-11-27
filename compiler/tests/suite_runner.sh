#!/bin/bash
# THE SUITE (tests/suite), built by the compiler under test and run four ways
# (doc/codegen.md 5.8 and 13.5).
#
#   tests/suite_runner.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>
#       LIBRARY=<the standard library folder>  SUITE=<the suite's project folder>  OUT=<a folder to work in>
#       (defaults: ~/sharp_work/sharp_dev/sharp, ~/sharp_work/out/boot/sharpc_boot,
#        ~/sharp_work/stdlib, ~/sharp_work/suite, ~/sharp_work/out/boot/suite_runner)
#
# The suite is ONE program (tests/suite/README.md): a project, built as
#
#       <sharp> <compiler> -P <suite> -j 1 -L <library> [-O]
#
# and run in the project's folder. A way PASSES when the program's exit code is 0 AND its last
# line is `all N tests passed`: a compile that writes no file, a crash, a hang or a failed test is
# a FAIL. suite_program.sh does the building and the judging.
#
# Four configurations: `plain` (the JIT), `plain-nojit`, and the same two with the program built
# with -O (the optimizer, doc/codegen.md 13.6: `O`, `O-nojit`). CONFIGURATIONS="plain O" runs only
# those. The program is built once without -O and once with it, and kept: OUT/plain/suite and
# OUT/O/suite (build-scripts/test.sh's kind checks, collector stress and ARM test run those).
# What each configuration printed: OUT/<configuration>/run.txt.
#
# ONE RUN AT A TIME: several tests work in fixed folders of their own under /tmp
# (/tmp/sharp_suite_io_json, /tmp/sharp_suite_std_files ...), and two runs of the program at once
# delete each other's files. For the same reason no other test that RUNS the suite (test.sh: 7, 9,
# 10, 17, 18) may run beside this one.
#
# A RUN THAT DIES WITHOUT ITS SUMMARY - a VM crash loses everything after it - is run again once
# for each top-level group (suite_program.sh), so the report names the group that crashes and still
# gives the others. A crash is a FAIL like any other and is never re-run until it passes. (An
# uncaught exception of a TEST is not a crash: it is a failed test, and the summary says so.)
#
# TIMEOUTS: a whole run takes seconds with the JIT; without it, much longer. A run is killed after
# 900 s with the JIT and 1800 s with -nojit (the old suite gave each of its 219 programs 120 s).
#
# Prints one line per configuration, the failed tests of a failing one, and exits 1 when any failed.
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
SUITE=${SUITE:-${SHARP_WORK:-$HOME/sharp_work}/suite}
OUT=${OUT:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/suite_runner}
source "$(dirname "$0")/suite_program.sh"
mkdir -p "$OUT"

# the compiler under test as one command (not used here any more; scripts written by hand call it)
WRAPPER="$OUT/sharpc_boot.sh"
printf '#!/bin/bash\nexec "%s" "%s" "$@"\n' "$SHARP" "$COMPILER" > "$WRAPPER"
chmod +x "$WRAPPER"

CONFIGURATIONS=${CONFIGURATIONS:-plain plain-nojit O O-nojit}
for name in $CONFIGURATIONS; do
    case "$name" in
        plain|plain-nojit|O|O-nojit) rm -rf "$OUT/$name"; mkdir -p "$OUT/$name" ;;
        *) echo "unknown configuration $name"; exit 1 ;;
    esac
done

ok=1
# the builds the configurations need: plain for plain and plain-nojit, -O for O and O-nojit
for build in plain O; do
    case " $CONFIGURATIONS " in *" $build "*|*" $build-nojit "*) ;; *) continue ;; esac
    options=""; [ $build = O ] && options="-O"
    mkdir -p "$OUT/$build"
    suite_build "$SUITE" "$OUT/$build/suite" "$OUT/$build/compile.txt" "$SHARP" "$COMPILER" -P "$SUITE" -j 1 -stdlib "$LIBRARY" $options \
        || { echo "the suite, built ${options:-plain}: DID NOT COMPILE"; ok=0; }
done

for name in $CONFIGURATIONS; do
    build=${name%-nojit}; runtime="$SHARP"; seconds=900
    case "$name" in *nojit) runtime="$SHARP -nojit"; seconds=1800 ;; esac
    if [ ! -f "$OUT/$build/suite" ]; then echo "the suite, $name: FAIL   not run (it did not compile)"; ok=0; continue; fi
    suite_way "the suite, $name" "$SUITE" "$OUT/$name/run.txt" $seconds $runtime "$OUT/$build/suite" || ok=0
done
[ $ok = 1 ]
