#!/bin/bash
# EVERY FIX A HELP PROPOSES MUST COMPILE.
#
#   tests/helps.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  LIBRARY=<the standard library folder>
#       CPP=<the C++ compiler, sharpc>   (defaults as in backend.sh; CPP: ~/sharp_work/sharpc_cl/sharpc,
#                                          the check against it is left out when it is not there)
#
# A message that says "write: `x == 1 && x == 2`" is only worth having if
# what it says to write is right. Each file in helps/ is a small program with
# one mistake, and says which help the mistake must get and where its text
# goes:
#
#       // expect: E0100                    the code of the message (one of the messages printed)
#       // help: x == 1 && x == 2           text that must appear, exactly, in a `= help:` line
#       // replace: x == 1 and x == 2       ... and the text of the program it is pasted over
#       // help: ...                        more pairs, applied in order; instead of `replace:`
#       // before: class c                  a pair may say `before:` - the help's text is put in
#                                           front of that text (a declaration the help adds)
#       // dots: return self;               what to write where the help says `...` (the
#                                           programmer's own code); nothing when not given
#       // remove: def f() { }              text the help says to take away ("move it out
#                                           of ..."), removed after the pastes
#
# The test then:
#   1. compiles the program: a message with the code must be printed, and every
#      help text must be printed in a help line;
#   2. pastes each help's text where the file says, `...` filled in;
#   3. compiles the result with this compiler: it must reach "analysis
#      complete" with no error at all;
#   4. compiles it with the C++ compiler (the toolchain that builds programs
#      today): it must build. A fix that only this compiler takes is no fix
#      the user can use yet. (When the C++ compiler CRASHES - dies on a signal
#      without reporting an error - it is run once more; see below.)
#
# Prints one line per failing file and a summary; exits 1 when anything failed.

HERE="$(cd "$(dirname "$0")" && pwd)"
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
CPP=${CPP:-}
WORK=${WORK:-${SHARP_WORK:-$HOME/sharp_work}/out/helps}
mkdir -p "$WORK"; rm -rf "$WORK"/*
export HERE SHARP COMPILER LIBRARY CPP WORK

one() {
    file=$1
    name=$(basename "$file" .sharp)
    LIB=$(find "$LIBRARY" -name "*.sharp" | sort)
    out="$WORK/$name.txt"
    timeout -s KILL 180 "$SHARP" "$COMPILER" -c $LIB "$file" > "$out.wrong" 2>&1

    # the program with the helps pasted in (and the checks of step 1)
    python3 - "$file" "$out.wrong" "$WORK/$name.sharp" > "$out" 2>&1 <<'PYTHON'
import sys, re
source_path, output_path, fixed_path = sys.argv[1], sys.argv[2], sys.argv[3]
lines = open(source_path, encoding="utf-8").read().split("\n")
output = open(output_path, encoding="utf-8", errors="replace").read()
expect, pairs, dots, removals = None, [], "", []
for line in lines:
    if not line.startswith("// "):
        break
    key, _, value = line[3:].partition(": ")
    if key == "expect": expect = value.strip()
    elif key == "help": pairs.append([value, None, None])
    elif key in ("replace", "before") and pairs: pairs[-1][1], pairs[-1][2] = key, value
    elif key == "dots": dots = value
    elif key == "remove": removals.append(value)
problems = []
codes = re.findall(r"^(?:error|warning)\[([EW][0-9]+)\]", output, re.M)
mine = [c for c in codes]
if expect is None: problems.append("no `// expect:` line")
elif not mine: problems.append("expected %s, but nothing was reported" % expect)
elif mine[0] != expect and expect not in mine: problems.append("expected %s, got %s" % (expect, " ".join(mine)))
helps = [l.split("= help:", 1)[1] for l in output.split("\n") if "= help:" in l]
text = "\n".join(lines)
for help_text, how, where in pairs:
    if not any(help_text in h for h in helps):
        problems.append("no help says: %s" % help_text)
        continue
    if how is None:
        problems.append("the help `%s` has no `// replace:` or `// before:` line" % help_text); continue
    body_start = text.index("\nmod ") if "\nmod " in text else 0
    at = text.find(where, body_start)
    if at < 0:
        problems.append("the text to replace is not in the program: %s" % where); continue
    pasted = help_text.replace("...", dots)
    if how == "replace": text = text[:at] + pasted + text[at + len(where):]
    else: text = text[:at] + pasted + "\n" + text[at:]
for where in removals:
    body_start = text.index("\nmod ") if "\nmod " in text else 0
    at = text.find(where, body_start)
    if at < 0: problems.append("the text to remove is not in the program: %s" % where)
    else: text = text[:at] + text[at + len(where):]
open(fixed_path, "w", encoding="utf-8").write(text)
for p in problems: print("PROBLEM " + p)
PYTHON

    if ! grep -q "^PROBLEM" "$out"; then
        timeout -s KILL 180 "$SHARP" "$COMPILER" -c $LIB "$WORK/$name.sharp" > "$out.fixed" 2>&1
        if ! grep -q "^analysis complete" "$out.fixed" || grep -q "^error" "$out.fixed"; then
            echo "PROBLEM with the help pasted in, this compiler still refuses the program:" >> "$out"
            grep -A6 "^error" "$out.fixed" | head -14 >> "$out"
        fi
        if [ -x "$CPP" ]; then
            # (each in a folder of its own: two C++ compiles in one folder spoil each other's build)
            mkdir -p "$WORK/$name.d"
            ( cd "$WORK/$name.d" && timeout -s KILL 180 "$CPP" "$WORK/$name.sharp" -o "$WORK/$name.out" -L "$LIBRARY" > "$out.cpp" 2>&1 )
            status=$?
            # The C++ compiler itself now and then dies with a segmentation fault when several
            # of it run at once (seen on field_called.sharp, which it compiles six times out of
            # six when run alone). A CRASH says nothing about the help, a REFUSAL does: when it
            # wrote no file and reported no error, it is run once more, and the file is listed in
            # the summary as retried. (Until the verifier of codegen stage 3 the retry also asked
            # for an exit status above 128; a gates run then failed on group_import.sharp - no file,
            # no message, and the retry did not run - and the same file built when run again alone.
            # A refusal always comes with an error message, so "no file and no message" is the
            # crash, whatever the status.)
            # The second thing it does now and then under load, found by the verifier of codegen
            # stage 4: it refuses the STANDARD LIBRARY - `loopable.sharp:4:29: could not resolve
            # symbol t` (the generic parameter of loopable<t>) and six errors that follow from it in
            # string.sharp, string_builder.sharp and char_array.sharp - on a program it builds when
            # run again (cast_of_operation.sharp; the 57 other programs of the same run built with
            # the same library). Errors that are ALL in library files say nothing about the pasted
            # help either: a help can only break the program's own file. So the compile is also run
            # once more when it wrote no file and none of its errors is outside the library; a
            # refusal that names the program is never retried, and a second failure fails the test.
            if [ ! -f "$WORK/$name.out" ] && ! grep -i "error" "$out.cpp" | grep -qv "^$LIBRARY/"; then
                echo "$name" > "$WORK/$name.retried"
                ( cd "$WORK/$name.d" && timeout -s KILL 180 "$CPP" "$WORK/$name.sharp" -o "$WORK/$name.out" -L "$LIBRARY" > "$out.cpp" 2>&1 )
            fi
            if [ ! -f "$WORK/$name.out" ]; then
                echo "PROBLEM with the help pasted in, the C++ compiler does not build the program:" >> "$out"
                grep -i "error" "$out.cpp" | grep -v "stdlib" | head -6 >> "$out"
            fi
        fi
    fi
}
export -f one

# (the folder has spaces in its name: each path goes to `one` as one argument)
find "$HERE/helps" -name "*.sharp" -print0 | xargs -0 -P 4 -I{} bash -c 'one "$1"' _ {}

passed=0
failed=0
for file in "$HERE"/helps/*.sharp; do
    name=$(basename "$file" .sharp)
    if [ ! -f "$WORK/$name.txt" ]; then
        failed=$((failed+1))
        echo "FAIL  helps/$name.sharp"
        echo "        the test did not run (no $WORK/$name.txt)"
    elif grep -q "^PROBLEM" "$WORK/$name.txt"; then
        failed=$((failed+1))
        echo "FAIL  helps/$name.sharp"
        sed 's/^/        /' "$WORK/$name.txt" | head -12
    else
        passed=$((passed+1))
    fi
done
retried=$(cat "$WORK"/*.retried 2>/dev/null | tr '
' ' ')
[ -n "$retried" ] && echo "note: the C++ compiler crashed (a signal, no error message) or refused the standard library itself, and was run a second time on: $retried"
echo "help tests: $passed passed, $failed failed   (the programs with the helps pasted in: $WORK/<name>.sharp)"
[ $failed = 0 ]
