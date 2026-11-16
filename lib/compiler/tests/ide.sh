#!/bin/bash
# The tests of the files the compiler writes for the editor (`-ide <folder>`, backend/ide).
#
#   tests/ide.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  LIBRARY=<the standard library folder>
#       (defaults: ~/sharp_work/sharp_cl/sharp, ~/sharp_work/out/boot_ide/sharpc_boot, ~/sharp_work/stdlib)
#
# Every sample of tests/ide/samples is compiled TOGETHER WITH the standard library and `-ide <folder>`;
# then tests/ide/check.py looks at what was written: valid JSON, one run id in every file, the expected
# entries, and - the point of it - that every position slices the right text out of the source
# (UTF-16 units, a line end \r\n as one unit, a tab as one, an emoji as two; doc/ide.md 4.1).
#
#   shapes        a clean program: modules, classes, inheritance, a generic class and its instances, a lambda,
#                 an extension function, a property with get/set, an enum, an alias, a global
#   errors        a declaration error, a body error and a warning: the index is still written (level `full`)
#   syntax        a file that does not parse: the syntax errors only, no index
#   weird         a file with tabs, CRLF line ends, non-ASCII text in strings and comments, and an emoji
#   corners       two files: a generic function, `mutate`, a nested class, an operator, static members, a one-line
#                 function, a lambda in a field's value, a base call; the text each kind of local can be seen in
#   odd           a lone carriage return; bytes that are not UTF-8; a class declared in two files (an error
#                 with a related place in the other file)
#   bodies_*      which files get a bodies file: bodies_for.txt missing / listing two files / empty
#   no -ide       what the compiler prints is the same with and without the option (a clean program, a program with
#                 analysis errors and a warning, a program with a syntax error)
#   failing       the output folder cannot be written into: the build is not disturbed, failed.txt says why
#   suite_*       four programs of the language's own test suite: every position cuts the right text out
#
# Prints one line per failure and a summary; exits 1 when anything failed.

HERE="$(cd "$(dirname "$0")" && pwd)"
SHARP=${SHARP:-$HOME/sharp_work/sharp_cl/sharp}
COMPILER=${COMPILER:-$HOME/sharp_work/out/boot_ide/sharpc_boot}
LIBRARY=${LIBRARY:-$HOME/sharp_work/stdlib}
LIB=$(find "$LIBRARY" -name "*.sharp" | sort)
SAMPLES="$HERE/ide/samples"
WORK="${TMPDIR:-/tmp}/ide_tests_$$"
rm -rf "$WORK"; mkdir -p "$WORK"
trap 'rm -rf "$WORK"' EXIT

passed=0
failed=0

# compile `files...` with the library and -ide into $WORK/<name>; the compiler's output goes to $WORK/<name>.txt
compile() {
    local name=$1; shift
    mkdir -p "$WORK/$name"
    timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB "$@" -ide "$WORK/$name" 2>&1 | grep -v "^Executed in\|^vm exiting\|^$" > "$WORK/$name.txt"
}

# run check.py for a case
verify() {
    local case_name=$1 name=$2; shift 2
    if [ ! -f "$WORK/$name/ide.json" ]; then
        failed=$((failed+1)); echo "FAIL $case_name: no ide.json was written"; tail -3 "$WORK/$name.txt" | sed 's/^/        /'
        return
    fi
    if out=$(python3 "$HERE/ide/check.py" "$case_name" "$WORK/$name" "$WORK/$name.txt" "$@" 2>&1); then
        passed=$((passed+1))
    else
        failed=$((failed+1)); echo "$out"
    fi
}

# ---- the weird file: tabs, CRLF, non-ASCII, an emoji (made here: git may change the line ends of a checked-in file)
python3 - "$WORK/weird_text.sharp" <<'EOF'
import sys
lines = [
    "mod weird;",
    "",
    "import ( std.io )",
    "",
    "// café ☕ 日本語 comment",
    "def greet(name: string) : string {",
    "\ttext := \"héllo \U0001F600 wörld\";\t// a tab, then a comment ünïcode",
    "\tcount := 1 + 2;",
    "\tbad := text + undefined_one;",
    "\tprintln(\"\U0001F600 \U0001F600\" + undefined_two + count);",
    "\treturn text;",
    "}",
    "",
    "def main(args: string[]) : int {",
    "\tprintln(greet(\"x\"));",
    "\treturn 0;",
    "}",
    "",
]
with open(sys.argv[1], "wb") as handle:
    handle.write("\r\n".join(lines).encode("utf-8"))
EOF

# ---- the odd files (made here for the same reason, and because an editor would "repair" them when saving)
python3 - "$WORK" <<'ODD'
import os, sys
work = sys.argv[1]
def write(name, data):
    with open(os.path.join(work, name), "wb") as handle:
        handle.write(data)
# a lone carriage return (the old Mac line end) between the first lines, then ordinary line ends
write("lone_cr.sharp", b"mod lone_cr;\r\rdef f() : _int64 {\r    return missing_cr;\n}\n")
# bytes that are not UTF-8: two stray bytes, a two-byte character cut short, a four-byte character cut short
write("bad_bytes.sharp", b"mod bad_bytes;\n\n// bad bytes: \xff\xfe \xc3( \xf0\x9f end\ndef f() : _int64 {\n"
                         b"    text := \"caf\xc3\xa9 \xff\";\n    return missing_bad;\n}\n")
# one class declared in two files of a module
write("twin_a.sharp", b"mod twins;\n\nclass twin {\n    size : _int64 = 1;\n}\n")
write("twin_b.sharp", b"mod twins;\n\nclass twin {\n    size : _int64 = 2;\n}\n")
ODD

echo "ide tests: compiling the samples with the standard library (about 5 seconds each)"

# ---- shapes, with and without -ide
compile shapes "$SAMPLES/shapes.sharp"
verify shapes shapes "$SAMPLES/shapes.sharp"

# (-c: since the two streams were joined - doc/ide.md 3.3 - the compiler goes on to write an executable after
#  analysis, and -ide stops before that as -c does. So "the same without the option" is "the same as -c".)
timeout -s KILL 300 "$SHARP" "$COMPILER" -c $LIB "$SAMPLES/shapes.sharp" 2>&1 | grep -v "^Executed in\|^vm exiting\|^$" > "$WORK/plain.txt"
if diff -q "$WORK/plain.txt" "$WORK/shapes.txt" > /dev/null; then
    passed=$((passed+1))
else
    failed=$((failed+1)); echo "FAIL no -ide: the compiler prints something else with -ide:"; diff "$WORK/plain.txt" "$WORK/shapes.txt" | head -8 | sed 's/^/        /'
fi
if ! grep -q "^analysis complete" "$WORK/plain.txt"; then
    failed=$((failed+1)); echo "FAIL no -ide: the clean sample does not reach 'analysis complete'"; tail -5 "$WORK/plain.txt" | sed 's/^/        /'
else
    passed=$((passed+1))
fi

# ---- errors, syntax, weird
compile errors "$SAMPLES/analysis_errors.sharp"
verify errors errors "$SAMPLES/analysis_errors.sharp"

compile syntax "$SAMPLES/syntax_error.sharp"
verify syntax syntax "$SAMPLES/syntax_error.sharp"

# ---- the same promise for programs WITH messages: what the compiler prints (every error, warning, note and help,
# in the same order) is the same with and without -ide, for analysis errors and for syntax errors
same_output_without_ide() {
    local case_name=$1 name=$2; shift 2
    timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB "$@" 2>&1 | grep -v "^Executed in\|^vm exiting\|^$" > "$WORK/$name.plain.txt"
    if [ -s "$WORK/$name.plain.txt" ] && diff -q "$WORK/$name.plain.txt" "$WORK/$name.txt" > /dev/null; then
        passed=$((passed+1))
    else
        failed=$((failed+1)); echo "FAIL $case_name: the compiler prints something else with -ide:"; diff "$WORK/$name.plain.txt" "$WORK/$name.txt" | head -8 | sed 's/^/        /'
    fi
}
same_output_without_ide "no -ide (errors)" errors "$SAMPLES/analysis_errors.sharp"
same_output_without_ide "no -ide (syntax)" syntax "$SAMPLES/syntax_error.sharp"

compile weird "$WORK/weird_text.sharp"
verify weird weird "$WORK/weird_text.sharp"

compile corners "$SAMPLES/corners.sharp" "$SAMPLES/corners_more.sharp"
verify corners corners "$SAMPLES/corners.sharp" "$SAMPLES/corners_more.sharp"

compile odd "$WORK/lone_cr.sharp" "$WORK/bad_bytes.sharp" "$WORK/twin_a.sharp" "$WORK/twin_b.sharp"
verify odd odd "$WORK/lone_cr.sharp" "$WORK/bad_bytes.sharp" "$WORK/twin_a.sharp" "$WORK/twin_b.sharp"

# ---- bodies_for.txt: missing (every non-library file), two files, empty
compile bodies_auto "$SAMPLES/shapes.sharp"
verify bodies_auto bodies_auto "$SAMPLES/shapes.sharp"

LIBFILE=$(echo "$LIB" | head -1)
mkdir -p "$WORK/bodies_both"; printf '%s\r\n%s\n' "$SAMPLES/shapes.sharp" "$LIBFILE" > "$WORK/bodies_both/bodies_for.txt"
compile bodies_both "$SAMPLES/shapes.sharp"
verify bodies_both bodies_both "$SAMPLES/shapes.sharp" "$LIBFILE"

mkdir -p "$WORK/bodies_none"; : > "$WORK/bodies_none/bodies_for.txt"
compile bodies_none "$SAMPLES/shapes.sharp"
verify bodies_none bodies_none "$SAMPLES/shapes.sharp"

# ---- the writing itself fails (a FILE stands where the folder bodies/ must be made): the build goes on and prints
# what it always prints, no manifest is written (so the plugin does not read half a run), no temporary file is
# left, and the reason is in failed.txt
mkdir -p "$WORK/failing"; : > "$WORK/failing/bodies"
compile failing "$SAMPLES/shapes.sharp"
if [ ! -f "$WORK/failing/ide.json" ] && [ -s "$WORK/failing/failed.txt" ] \
   && [ -z "$(find "$WORK/failing" -name '*.tmp')" ] && diff -q "$WORK/plain.txt" "$WORK/failing.txt" > /dev/null; then
    passed=$((passed+1))
else
    failed=$((failed+1)); echo "FAIL failing: a write that fails must leave failed.txt, no ide.json, no .tmp file, and the same output:"
    ls "$WORK/failing" | sed 's/^/        /'; diff "$WORK/plain.txt" "$WORK/failing.txt" | head -5 | sed 's/^/        /'
fi

# ---- programs of the language's own test suite: operators, function values, exceptions, generics, properties.
# (The gates compile all of them with -ide and check that the compiler still passes; here their positions are checked.)
SUITE="$HERE/../../../tests/suite"
if [ -f "$SUITE/testing.sharp" ]; then
    for name in operators function_pointers exceptions list_index_assignment; do
        if [ -f "$SUITE/cases/$name.sharp" ]; then
            compile "suite_$name" "$SUITE/testing.sharp" "$SUITE/cases/$name.sharp"
            verify "suite_$name" "suite_$name" "$SUITE/cases/$name.sharp"
        fi
    done
fi

echo "ide tests: $passed passed, $failed failed"
[ $failed = 0 ]
