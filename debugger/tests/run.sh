#!/bin/bash
# The tests of sdb, the Sharp debugger (debugger). Run on Linux / WSL:
#
#     bash debugger/tests/run.sh
#
# It needs the VM and the compiler that build-scripts/build.sh makes, then
#   1. builds sdb (build-scripts/internal/build_sdb.sh, from the work folder's copy of the sources)
#   2. builds the small programs of tests/programs - with -dbg, and `values` twice more:
#      without -dbg (values_plain) and with -obf (values_obf)
#   3. builds tests/sdb_tests.sharp and runs it with `sharp`: the tests are a Sharp program
#      that starts sdb on those programs and reads what it says
#
# Its last line is  sdb tests: PASS  or  sdb tests: FAIL.
#     SDB=<an sdb that is already built>      skips step 1
source "$(cd "$(dirname "$0")/../.." && pwd)/build-scripts/internal/common.sh"
[ -x "$VM" ] && [ -f "$COMPILER" ] || fail "no VM or compiler yet: run  bash build-scripts/build.sh  first"
HERE="$REPO/debugger/tests"
OUT="$WORK/out/sdb_tests"; rm -rf "$OUT"; mkdir -p "$OUT"

# ---- 1. sdb
if [ -z "$SDB" ]; then
    bash "$REPO/build-scripts/internal/build_sdb.sh" > "$OUT/build.txt" 2>&1 || { tail -20 "$OUT/build.txt"; fail "sdb did not build"; }
    SDB="$WORK/sdb_dev/sdb"
fi
[ -x "$SDB" ] || fail "no sdb at $SDB"
echo "sdb: $("$SDB" -V)"

# ---- 2. the programs
compile() {     # compile <output> <options...> -- <files...>
    local output=$1; shift
    "$VM" "$COMPILER" "$@" -j 1 -o "$OUT/$output" -L "$STDLIB" > "$OUT/$output.compile.txt" 2>&1
    [ -f "$OUT/$output" ] || { grep -E "error" -A4 "$OUT/$output.compile.txt" | head -20; fail "tests/programs: $output did not compile"; }
}
P="$HERE/programs"
for name in notebook values lambdas workers two_threads locks long_run fiber_values main_ends_first; do compile $name.dbg "$P/$name.sharp" -dbg; done
compile twins.dbg "$P/twins/twins.sharp" "$P/twins/shop/notes.sharp" "$P/twins/school/notes.sharp" -dbg
compile values_plain "$P/values.sharp"
compile values_obf "$P/values.sharp" -dbg -obf

# ---- 3. the tests
compile sdb_tests "$HERE/sdb_tests.sharp"
"$VM" "$OUT/sdb_tests" "$SDB" "$OUT" 2>&1 | tee "$OUT/result.txt" | tail -40
if grep -q "RESULT PASS" "$OUT/result.txt"; then echo "sdb tests: PASS"; else echo "sdb tests: FAIL   (everything it printed: $OUT/result.txt)"; exit 1; fi
