#!/bin/bash
# The tests of the self-hosted compiler's backend (everything after parsing).
#
#   tests/backend.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  LIBRARY=<the standard library folder>
#       (defaults: ~/sharp_work/sharp_dev/sharp, ~/sharp_work/out/boot/sharpc_boot, ~/sharp_work/stdlib)
#
# Every test file is compiled TOGETHER WITH the standard library, as a real
# program would be.
#
#   backend/pass/*.sharp    must be accepted: no error at all, and the compiler
#                           must reach its last line ("analysis complete").
#                           Each file gathers one area: every correct form of it.
#
#   backend/fail/*.sharp    must be REJECTED, with exactly the errors its first
#                           line names, in the order they are printed:
#                               // expect: E0341 E0342
#                           A mistake that goes unnoticed and a mistake that
#                           sets off extra messages are both wrong.
#                           Warnings (W....) are listed like errors, in the same
#                           order - but only the ones reported IN THE TEST FILE:
#                           the standard library has a few warnings of its own,
#                           which are not the test's business.
#
#   A second line `// options: -wextra` (in a pass or a fail file) adds those
#   options to the command line of that test.
#
# Prints one line per failing test and a summary; exits 1 when anything failed.

HERE="$(cd "$(dirname "$0")" && pwd)"
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
LIB=$(find "$LIBRARY" -name "*.sharp" | sort)

run() { timeout -s KILL 180 "$SHARP" "$COMPILER" -c $LIB "$@" 2>&1 | grep -v "^Executed in\|^vm exiting\|^$"; }

# the options a test asks for on its second line: `// options: -wextra`
options_of() { sed -n '2s|^// options: *||p' "$1"; }

passed=0
failed=0
fail() { failed=$((failed+1)); echo "FAIL  $1"; shift; for line in "$@"; do echo "        $line"; done; }

for file in "$HERE"/backend/pass/*.sharp; do
    [ -f "$file" ] || continue
    name="pass/$(basename "$file")"
    out=$(run "$file" $(options_of "$file"))
    if echo "$out" | grep -q "^analysis complete" && ! echo "$out" | grep -q "^error"; then
        passed=$((passed+1))
    else
        fail "$name" "expected no errors, got:" "$(echo "$out" | grep -A3 "^error" | head -12)" "$(echo "$out" | tail -3)"
    fi
done

for file in "$HERE"/backend/fail/*.sharp; do
    [ -f "$file" ] || continue
    name="fail/$(basename "$file")"
    expected=$(head -1 "$file" | sed -n 's|^// expect: *||p' | tr -s ' ' | sed 's/ *$//')
    actual=$(run "$file" $(options_of "$file") | awk -v name="$(basename "$file")" '
        /^error\[E[0-9]+\]/   { match($0, /E[0-9]+/); printf "%s ", substr($0, RSTART, RLENGTH); next }
        /^warning\[W[0-9]+\]/ { match($0, /W[0-9]+/); warning = substr($0, RSTART, RLENGTH); next }
        warning != "" && /-->/  { if(index($0, name) > 0) { printf "%s ", warning }; warning = "" }
    ' | sed 's/ *$//')
    if [ -z "$expected" ]; then
        fail "$name" "the first line must be:  // expect: E.... E...."
    elif [ "$expected" = "$actual" ]; then
        passed=$((passed+1))
    else
        fail "$name" "expected: $expected" "got:      $actual"
    fi
done

echo "backend tests: $passed passed, $failed failed"
[ $failed = 0 ]
