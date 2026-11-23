#!/bin/bash
# The tests of sdb, the Sharp debugger, on WINDOWS.   (Git Bash)
#
#     bash build-scripts/windows/sdb_tests.sh
#
# test.sh runs them in WSL, on Linux (its test 16: lib/debugger/tests/run.sh). This is the same
# check for the Windows sdb: the small programs of lib/debugger/tests/programs are built, then
# lib/debugger/tests/sdb_tests.sharp - a Sharp program - starts sdb.exe on them and reads what it says.
#
# It needs  sdb.exe  (windows/build_sdb.sh), the Windows VM (windows/build_vm.sh) and a compiler:
#     COMPILER=<file>   the compiler to build the programs with      (default: the installed one)
#                       After a change to what the compiler writes for the debugger, that is the one
#                       just built in WSL:  COMPILER=<a copy of ~/sharp_work/out/boot/sharpc_boot>
#
# The last line is  WINDOWS SDB TESTS: PASS  or  WINDOWS SDB TESTS: FAIL.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
VM=${VM:-$ROOT/cmake-build-windows-vm/sharp.exe}
SDB=${SDB:-$ROOT/cmake-build-windows-sdb/sdb.exe}
INSTALLED="$LOCALAPPDATA/Sharp"
COMPILER=${COMPILER:-$(ls "$INSTALLED"/compiler/sharpc-v* 2>/dev/null | grep -v debug | sort -V | tail -1)}
LIBRARY="$ROOT/lib/support/$(ls "$ROOT/lib/support" | sort -V | tail -1)"
OUT=${OUT:-$ROOT/build/windows-sdb-tests}
[ -f "$VM" ] || { echo "no VM at $VM: run  bash build-scripts/windows/build_vm.sh  first"; exit 1; }
[ -f "$SDB" ] || { echo "no sdb at $SDB: run  bash build-scripts/windows/build_sdb.sh  first"; exit 1; }
[ -f "$COMPILER" ] || { echo "no compiler: install a release, or give one with COMPILER=<file>"; exit 1; }
rm -rf "$OUT"; mkdir -p "$OUT"
echo "sdb: $("$SDB" -V), the programs compiled by $(basename "$COMPILER")"

compile() {     # compile <output> <files and options...>
    local output=$1; shift
    "$VM" "$COMPILER" "$@" -j 1 -o "$OUT/$output" -L "$LIBRARY" > "$OUT/$output.compile.txt" 2>&1
    [ -f "$OUT/$output" ] || { grep -E "error" -A4 "$OUT/$output.compile.txt" | head -20; echo "$output did not compile"; echo "WINDOWS SDB TESTS: FAIL"; exit 1; }
}
P="$ROOT/lib/debugger/tests/programs"
for name in notebook values lambdas workers two_threads locks long_run fiber_values main_ends_first; do compile $name.dbg "$P/$name.sharp" -dbg; done
compile twins.dbg "$P/twins/twins.sharp" "$P/twins/shop/notes.sharp" "$P/twins/school/notes.sharp" -dbg
compile values_plain "$P/values.sharp"
compile values_obf "$P/values.sharp" -dbg -obf
compile sdb_tests "$ROOT/lib/debugger/tests/sdb_tests.sharp"

"$VM" "$OUT/sdb_tests" "$(cygpath -m "$SDB")" "$(cygpath -m "$OUT")" 2>&1 | tee "$OUT/result.txt" | tail -40
if grep -q "RESULT PASS" "$OUT/result.txt"; then echo "WINDOWS SDB TESTS: PASS"; else echo "WINDOWS SDB TESTS: FAIL   (everything it printed: $OUT/result.txt)"; exit 1; fi
