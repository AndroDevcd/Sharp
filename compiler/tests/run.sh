#!/bin/bash
# The tests of the self-hosted compiler's frontend (tokenizer and parser).
#
#   tests/run.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>
#       (defaults: ~/sharp_work/sharp_dev/sharp and ~/sharp_work/out/boot/sharpc_boot)
#
# Three kinds of test, one file each:
#
#   parse/pass/*.sharp      must parse with NO syntax error. Each file gathers
#                           one area of the language: every form it can take.
#
#   parse/fail/*.sharp      must be REJECTED, with exactly the errors its first
#                           line names, in order:
#                               // expect: E0101 E0100 E0102
#                           (the code in the brackets of "error[E0101]: ...").
#                           A file that reports a different number of errors,
#                           or other ones, fails: a mistake that goes unnoticed
#                           and a mistake that sets off extra messages are both
#                           wrong.
#
#   parse/trees/*.sharp     must produce exactly the syntax tree in the file
#                           with the same name ending in .tree (the output of
#                           `sharpc -parsetree`). For what "parses" cannot
#                           show: how an expression is grouped.
#                           To accept a new tree after looking at it:
#                               tests/run.sh --accept-trees
#
# Prints one line per failing test and a summary; exits 1 when anything failed.

HERE="$(cd "$(dirname "$0")" && pwd)"
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
ACCEPT=0
[ "$1" = "--accept-trees" ] && ACCEPT=1

run() { timeout -s KILL 60 "$SHARP" "$COMPILER" "$@" 2>&1 | grep -v "^Executed in\|^vm exiting\|^$"; }

passed=0
failed=0
fail() { failed=$((failed+1)); echo "FAIL  $1"; shift; for line in "$@"; do echo "        $line"; done; }

# ---- must parse
for file in "$HERE"/parse/pass/*.sharp; do
    name="pass/$(basename "$file")"
    out=$(run -parsestats "$file")
    if echo "$out" | grep -q "syntax errors 0 in 0 files"; then
        passed=$((passed+1))
    else
        fail "$name" "expected no syntax errors, got:" "$(echo "$out" | grep "^error" | head -5)"
    fi
done

# ---- must be rejected, with these errors
for file in "$HERE"/parse/fail/*.sharp; do
    name="fail/$(basename "$file")"
    expected=$(head -1 "$file" | sed -n 's|^// expect: *||p' | tr -s ' ' | sed 's/ *$//')
    actual=$(run "$file" | grep -o "^error\[E[0-9]*\]" | sed 's/error\[\(.*\)\]/\1/' | tr '\n' ' ' | sed 's/ *$//')
    if [ -z "$expected" ]; then
        fail "$name" "the first line must be:  // expect: E.... E...."
    elif [ "$expected" = "$actual" ]; then
        passed=$((passed+1))
    else
        fail "$name" "expected: $expected" "got:      $actual"
    fi
done

# ---- must give this tree
for file in "$HERE"/parse/trees/*.sharp; do
    name="trees/$(basename "$file")"
    want="${file%.sharp}.tree"
    got=$(run -parsetree "$file")
    if [ $ACCEPT = 1 ]; then
        echo "$got" > "$want"
        echo "accepted $name"
        passed=$((passed+1))
    elif [ ! -f "$want" ]; then
        fail "$name" "no expected tree yet: look at the output of -parsetree, then run tests/run.sh --accept-trees"
    elif [ "$got" = "$(cat "$want")" ]; then
        passed=$((passed+1))
    else
        fail "$name" "the tree changed:" "$(diff <(echo "$got") "$want" | head -8)"
    fi
done

echo "frontend tests: $passed passed, $failed failed"
[ $failed = 0 ]
