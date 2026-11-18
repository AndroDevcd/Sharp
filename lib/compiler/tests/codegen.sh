#!/bin/bash
# The tests of code generation (doc/codegen.md 5.8, 13.1): programs compiled by
# the bootstrap compiler and RUN on the virtual machine.
#
#   tests/codegen.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  SHARPC=<the C++ compiler>
#       LIBRARY=<the standard library folder>  OUT=<a folder for the executables>
#       (defaults: ~/sharp_work/sharp_dev/sharp, ~/sharp_work/out/boot/sharpc_boot,
#        ~/sharp_work/sharpc_cl/sharpc, ~/sharp_work/stdlib, ~/sharp_work/out/boot/codegen)
#
#   codegen/cases/*.sharp   a program and the output it must print, in a comment block:
#                               // output:
#                               // 42
#                           Each is compiled together with codegen/print.sharp and the
#                           library as a normal program (the VM starts it at
#                           platform.srt_init, which sets it up and calls its main), run
#                           with the JIT and with -nojit, and both outputs must be the
#                           block. The same program is ALSO compiled by the C++ compiler
#                           and run: its output must be the block too - the oracle
#                           (codegen.md 2). (Until stage 4 the cases were started at their
#                           main with the test option -entry, which is gone now.)
#
#   codegen/own/*.sharp     the same, WITHOUT the C++ compiler: programs it cannot build or builds
#                           wrongly (its bugs, and the deliberate differences of codegen.md section
#                           10). The output block was worked out by hand from the rule of the
#                           language, and each file says which rule; JIT and -nojit must print it.
#
#   codegen/refused/*.sharp a program analysis accepts and code generation must refuse (a name
#                           the VM needs that an `obfuscate` block renames ...): the codes of the
#                           `// expect:` line must be printed, in that order, and no file written.
#
#   codegen/obfuscation/*.sharp   programs compiled with -obf (codegen.md 6.17), JIT and -nojit:
#                           the output block must be printed as without it (only names in the
#                           file change, never what the program does), and the map of new names
#                           (<output>.names) must list every `// renamed: <full name>` of the
#                           file and none of its `// kept: <full name>` lines.
#
#   -dbg                    cases/hello.sharp once more with -dbg: the file must carry the
#                           program's source text (the VM prints its lines in stack traces)
#                           and still run with the same output.
#
#   -O                      the optimizer (doc/codegen.md 7, 13.6): every program of cases/ and
#                           own/ once more, compiled with -O, run with the JIT and with -nojit: it
#                           must print its block - the optimizer may make a program faster, never
#                           different. (The plain build of each cases/ program is compared with the
#                           C++-built one above, so the block is already the oracle's.) Then the
#                           options: -n<name> leaves a pass out (and -Oreport says so), an unknown
#                           name is refused, -O0 turns -O off again. The library's checks below
#                           are made once more with -O.
#
#   the suite               every program of tests/suite/cases, compiled as a normal program (the
#                           VM starts it at platform.srt_init, which calls its main): it must run
#                           and print RESULT PASS, with the JIT and with -nojit (since stage 4
#                           every case; codegen/suite_running.txt lists them, below).
#
#   the library             the whole standard library (every function and class analysis
#                           has: -eager), with a `main` of nothing, through layout, the
#                           verifier and the file writer, and the VM must load the file and run
#                           it to its end - the platform's start-up, the empty main, the
#                           shutdown - printing nothing, exit code 0. The same once more with
#                           -obf: every name the library's obfuscate blocks do not keep renamed.
#
# Only the program's own output is compared: the VM prints "Executed in ..." and
# "vm exiting..." after it, which are cut off.
#
# Prints one line per failing test and a summary; exits 1 when anything failed.

# THE C++ COMPILER IS GONE (17 Nov 2026). Sharp's first compiler was written in C++ and this
# script used it as a second opinion. It cannot read today's standard library, so it is no longer
# built or kept. SHARPC=<a second Sharp compiler, as one command> still turns the comparison on;
# without it (the default) the parts that say "C++" below are skipped.
HERE="$(cd "$(dirname "$0")" && pwd)"
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
SHARPC=${SHARPC:-}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
OUT=${OUT:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/codegen}
LIB=$(find "$LIBRARY" -name "*.sharp" | sort)
mkdir -p "$OUT"

compile() { timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB "$@" 2>&1 | grep -v "^Executed in\|^vm exiting\|^$"; }

# the program's own output: everything before the VM's "Executed in ..." line (and the blank line before it)
run_program() {
    timeout -s KILL 60 "$SHARP" "$@" 2>&1 | sed '/^Executed in [0-9]*us/,$d' | sed -e :a -e '/^\n*$/{$d;N;ba' -e '}'
}

# the lines of the `// output:` block of a case
expected_of() { awk '/^\/\/ output:/ {on=1; next} on && /^\/\// {sub(/^\/\/ ?/, ""); print; next} on {exit}' "$1"; }

passed=0
failed=0
fail() { failed=$((failed+1)); echo "FAIL  $1"; shift; for line in "$@"; do echo "        $line"; done; }

for file in "$HERE"/codegen/cases/*.sharp; do
    [ -f "$file" ] || continue
    base=$(basename "$file" .sharp)
    name="cases/$base.sharp"
    expected=$(expected_of "$file")
    rm -f "$OUT/$base" "$OUT/$base.cpp"
    out=$(compile "$HERE/codegen/print.sharp" "$file" -o "$OUT/$base")
    if [ ! -f "$OUT/$base" ]; then
        fail "$name" "the bootstrap compiler wrote no file:" "$(echo "$out" | grep -A4 "^error" | head -10)" "$(echo "$out" | tail -2)"
        continue
    fi
    with_jit=$(run_program "$OUT/$base")
    without_jit=$(run_program -nojit "$OUT/$base")
    # the C++ compiler now and then dies on a signal under load (its own bug, not the test's):
    # compiled once more then, and said so; a refusal is never retried
    if [ ! -x "$SHARPC" ]; then cpp_built="$expected"; else        # no second compiler: nothing to compare with
    "$SHARPC" "$HERE/codegen/print.sharp" "$file" -o "$OUT/$base.cpp" -L "$LIBRARY" > "$OUT/$base.cpp.txt" 2>&1
    if [ $? -ge 128 ]; then
        echo "note: the C++ compiler died on a signal compiling $name; compiled again"
        "$SHARPC" "$HERE/codegen/print.sharp" "$file" -o "$OUT/$base.cpp" -L "$LIBRARY" > "$OUT/$base.cpp.txt" 2>&1
    fi
    if [ -f "$OUT/$base.cpp" ]; then cpp_built=$(run_program "$OUT/$base.cpp"); else cpp_built="(the C++ compiler wrote no file)"; fi
    # The C++ compiler does not always build the same program from the same source: in one gates
    # run its build of cases/enums_and_dictionaries.sharp printed "green" for an enum value that is
    # `blue` (it numbers the values without a fixed number from a randomly offset counter:
    # codegen.md 10, G7), and eight builds of the same file alone were right. So a C++-built
    # program that does not print the block is built and run ONCE more, and said so; the case
    # fails when the second build does not print the block either. The bootstrap's own two runs
    # above are never repeated.
    if [ "$cpp_built" != "$expected" ]; then
        echo "note: the C++-built $name did not print the expected output; built with the C++ compiler again"
        rm -f "$OUT/$base.cpp"
        "$SHARPC" "$HERE/codegen/print.sharp" "$file" -o "$OUT/$base.cpp" -L "$LIBRARY" > "$OUT/$base.cpp.txt" 2>&1
        if [ -f "$OUT/$base.cpp" ]; then cpp_built=$(run_program "$OUT/$base.cpp"); else cpp_built="(the C++ compiler wrote no file)"; fi
    fi
    fi
    problems=()
    [ "$with_jit" == "$expected" ] || problems+=("with the JIT it printed:" "$(echo "$with_jit" | head -12 | tr '\n' '|')")
    [ "$without_jit" == "$expected" ] || problems+=("with -nojit it printed:" "$(echo "$without_jit" | head -12 | tr '\n' '|')")
    [ "$cpp_built" == "$expected" ] || problems+=("the C++-built program printed:" "$(echo "$cpp_built" | head -12 | tr '\n' '|')")
    if [ ${#problems[@]} -eq 0 ]; then
        passed=$((passed+1))
    else
        fail "$name" "expected: $(echo "$expected" | tr '\n' '|')" "${problems[@]}"
    fi
done

for file in "$HERE"/codegen/own/*.sharp; do
    [ -f "$file" ] || continue
    base=$(basename "$file" .sharp)
    name="own/$base.sharp"
    expected=$(expected_of "$file")
    rm -f "$OUT/own_$base"
    out=$(compile "$HERE/codegen/print.sharp" "$file" -o "$OUT/own_$base")
    if [ ! -f "$OUT/own_$base" ]; then
        fail "$name" "the bootstrap compiler wrote no file:" "$(echo "$out" | grep -A4 "^error" | head -10)" "$(echo "$out" | tail -2)"
        continue
    fi
    with_jit=$(run_program "$OUT/own_$base")
    without_jit=$(run_program -nojit "$OUT/own_$base")
    problems=()
    [ "$with_jit" == "$expected" ] || problems+=("with the JIT it printed:" "$(echo "$with_jit" | head -24 | tr '
' '|')")
    [ "$without_jit" == "$expected" ] || problems+=("with -nojit it printed:" "$(echo "$without_jit" | head -24 | tr '
' '|')")
    if [ ${#problems[@]} -eq 0 ]; then
        passed=$((passed+1))
    else
        fail "$name" "expected: $(echo "$expected" | tr '
' '|')" "${problems[@]}"
    fi
done

# ---- refused by code generation: the expected codes in order, and no file
for file in "$HERE"/codegen/refused/*.sharp; do
    [ -f "$file" ] || continue
    base=$(basename "$file" .sharp)
    name="refused/$base.sharp"
    wanted=$(head -5 "$file" | sed -n 's|^// expect: *||p')
    rm -f "$OUT/refused_$base"
    out=$(compile "$file" -o "$OUT/refused_$base")
    got=$(echo "$out" | sed -n 's/^error\[\(E[0-9]*\)\].*/\1/p' | tr '\n' ' ' | sed 's/ *$//')
    if [ -f "$OUT/refused_$base" ]; then
        fail "$name" "a file was written, although code generation must refuse the program"
    elif [ "$got" != "$wanted" ]; then
        fail "$name" "expected $wanted, got: ${got:-nothing}" "$(echo "$out" | grep -A2 "^error" | head -6)"
    elif ! echo "$out" | grep -q "^code generation failed"; then
        fail "$name" "the compile did not say it failed:" "$(echo "$out" | tail -3)"
    else
        passed=$((passed+1))
    fi
done

# ---- -obf (codegen.md 6.17): the same output, and the map of the new names
for file in "$HERE"/codegen/obfuscation/*.sharp; do
    [ -f "$file" ] || continue
    base=$(basename "$file" .sharp)
    name="obfuscation/$base.sharp"
    expected=$(expected_of "$file")
    rm -f "$OUT/obf_$base" "$OUT/obf_$base.names"
    out=$(compile "$HERE/codegen/print.sharp" "$file" -o "$OUT/obf_$base" -obf)
    if [ ! -f "$OUT/obf_$base" ]; then
        fail "$name" "the bootstrap compiler wrote no file:" "$(echo "$out" | grep -A4 "^error" | head -10)" "$(echo "$out" | tail -2)"
        continue
    fi
    problems=()
    with_jit=$(run_program "$OUT/obf_$base")
    without_jit=$(run_program -nojit "$OUT/obf_$base")
    [ "$with_jit" == "$expected" ] || problems+=("with the JIT it printed:" "$(echo "$with_jit" | head -12 | tr '\n' '|')")
    [ "$without_jit" == "$expected" ] || problems+=("with -nojit it printed:" "$(echo "$without_jit" | head -12 | tr '\n' '|')")
    if [ ! -f "$OUT/obf_$base.names" ]; then
        problems+=("no map of new names ($OUT/obf_$base.names) was written")
    else
        while read -r wanted; do
            [ -n "$wanted" ] || continue
            awk -v n="$wanted" '$3 == n { found = 1 } END { exit !found }' "$OUT/obf_$base.names" || problems+=("not renamed: $wanted")
        done < <(sed -n 's|^// renamed: *||p' "$file")
        while read -r wanted; do
            [ -n "$wanted" ] || continue
            awk -v n="$wanted" '$3 == n { found = 1 } END { exit !found }' "$OUT/obf_$base.names" && problems+=("renamed, but it must keep its name: $wanted")
        done < <(sed -n 's|^// kept: *||p' "$file")
    fi
    if [ ${#problems[@]} -eq 0 ]; then
        passed=$((passed+1))
    else
        fail "$name" "expected: $(echo "$expected" | tr '\n' '|')" "${problems[@]}"
    fi
done

# ---- -dbg: the debug flag of the manifest and the source text in the meta section (codegen.md 3.9)
file="$HERE/codegen/cases/hello.sharp"
rm -f "$OUT/hello_dbg"
out=$(compile "$HERE/codegen/print.sharp" "$file" -o "$OUT/hello_dbg" -dbg)
if [ ! -f "$OUT/hello_dbg" ]; then
    fail "-dbg" "the bootstrap compiler wrote no file:" "$(echo "$out" | grep -A4 "^error" | head -10)" "$(echo "$out" | tail -2)"
elif ! grep -a -q 'println("hello, world");' "$OUT/hello_dbg"; then
    fail "-dbg" "the file does not carry the source text of cases/hello.sharp"
elif [ "$(run_program "$OUT/hello_dbg")" != "$(expected_of "$file")" ] || [ "$(run_program -nojit "$OUT/hello_dbg")" != "$(expected_of "$file")" ]; then
    fail "-dbg" "the program built with -dbg does not print what it must:" "$(run_program "$OUT/hello_dbg" | head -6 | tr '
' '|')"
else
    passed=$((passed+1))
fi

# ---- the optimizer (-O): the same programs, the same output
for file in "$HERE"/codegen/cases/*.sharp "$HERE"/codegen/own/*.sharp; do
    [ -f "$file" ] || continue
    base=$(basename "$file" .sharp)
    kind=$(basename "$(dirname "$file")")
    name="-O $kind/$base.sharp"
    expected=$(expected_of "$file")
    rm -f "$OUT/O_${kind}_$base"
    out=$(compile "$HERE/codegen/print.sharp" "$file" -o "$OUT/O_${kind}_$base" -O)
    if [ ! -f "$OUT/O_${kind}_$base" ]; then
        fail "$name" "the bootstrap compiler wrote no file:" "$(echo "$out" | grep -A4 "^error" | head -10)" "$(echo "$out" | tail -2)"
        continue
    fi
    with_jit=$(run_program "$OUT/O_${kind}_$base")
    without_jit=$(run_program -nojit "$OUT/O_${kind}_$base")
    problems=()
    [ "$with_jit" == "$expected" ] || problems+=("with the JIT it printed:" "$(echo "$with_jit" | head -12 | tr '\n' '|')")
    [ "$without_jit" == "$expected" ] || problems+=("with -nojit it printed:" "$(echo "$without_jit" | head -12 | tr '\n' '|')")
    if [ ${#problems[@]} -eq 0 ]; then
        passed=$((passed+1))
    else
        fail "$name" "expected: $(echo "$expected" | tr '\n' '|')" "${problems[@]}"
    fi
done
# the options: a pass left out by name, -Oreport, an unknown name, -O0
file="$HERE/codegen/cases/optimizer.sharp"
rm -f "$OUT/O_left_out"
out=$(compile "$HERE/codegen/print.sharp" "$file" -o "$OUT/O_left_out" -O -nloop-rotation -ninline-calls -Oreport)
if [ ! -f "$OUT/O_left_out" ]; then
    fail "-O -nloop-rotation -ninline-calls" "no file was written:" "$(echo "$out" | grep -A3 "^error" | head -6)"
elif ! echo "$out" | grep -q "^left out *: loop-rotation inline-calls"; then
    fail "-Oreport" "the report does not say which passes were left out:" "$(echo "$out" | grep -A3 "optimizer (-O)" | head -4)"
elif echo "$out" | grep -q "pass loop-rotation:\|pass inline-calls:"; then
    fail "-nloop-rotation -ninline-calls" "a pass that was left out ran:" "$(echo "$out" | grep "pass loop-rotation:\|pass inline-calls:")"
elif [ "$(run_program "$OUT/O_left_out")" != "$(expected_of "$file")" ]; then
    fail "-O -nloop-rotation -ninline-calls" "it does not print the block of cases/optimizer.sharp"
else
    passed=$((passed+1))
fi
out=$(compile "$file" -o "$OUT/O_unknown" -O -nno-such-pass)
if ! echo "$out" | grep -q "unknown optimization \`no-such-pass\`"; then
    fail "-nno-such-pass" "an unknown optimization name was not refused:" "$(echo "$out" | head -3)"
else
    passed=$((passed+1))
fi
out=$(compile "$HERE/codegen/print.sharp" "$file" -o "$OUT/O_zero" -O -O0 -timings)
if ! echo "$out" | grep -q "optimizer 0 ms"; then
    fail "-O -O0" "-O0 did not turn the optimizer off:" "$(echo "$out" | grep "^time: layout")"
else
    passed=$((passed+1))
fi

# ---- the whole library through layout and the file writer; the VM must load and run it
#   1. -eager: every function and class analysis has (11000 functions). The VM must run the file
#      to its end with and without the JIT: platform.srt_init (the platform's start-up), the
#      `main` of nothing, the shutdown. It prints nothing and the exit code is 0.
#   2. the program a `main` of nothing really is (what srt_init and main reach): the same.
#   3. that program again with -obf: the same.
# runs_quietly <name> <file>: the file runs to its end, JIT and -nojit, printing nothing, exit code 0
runs_quietly() {
    for mode in "" "-nojit"; do
        timeout -s KILL 60 "$SHARP" $mode "$2" > "$2.run" 2>&1; loaded=$?
        printed=$(sed '/^Executed in [0-9]*us/,$d' "$2.run" | sed -e :a -e '/^\n*$/{$d;N;ba' -e '}')
        if [ "$loaded" != 0 ] || [ -n "$printed" ]; then
            fail "$1" "run ${mode:-with the JIT}: exit code $loaded; it printed:" "$(echo "$printed" | head -3)"
            return 1
        fi
    done
    return 0
}
rm -f "$OUT/library" "$OUT/library_reached"
out=$(compile "$HERE/codegen/library_main.sharp" -o "$OUT/library" -eager -timings)
summary=$(echo "$out" | grep "^code generation")
if [ ! -f "$OUT/library" ]; then
    fail "the library (-eager)" "no file was written:" "$(echo "$out" | grep -A3 "^error" | head -8)" "$(echo "$out" | tail -2)"
elif runs_quietly "the library (-eager)" "$OUT/library"; then
    passed=$((passed+1))
    echo "library, -eager: $summary"
    echo "$out" | grep "^time: layout"
fi
out=$(compile "$HERE/codegen/library_main.sharp" -o "$OUT/library_reached")
if [ ! -f "$OUT/library_reached" ]; then
    fail "the library (reached from main)" "no file was written:" "$(echo "$out" | grep -A3 "^error" | head -8)"
elif runs_quietly "the library (reached from main)" "$OUT/library_reached"; then
    passed=$((passed+1))
    echo "library, reached from main: $(echo "$out" | grep "^code generation")"
fi
rm -f "$OUT/library_obf"
out=$(compile "$HERE/codegen/library_main.sharp" -o "$OUT/library_obf" -obf)
if [ ! -f "$OUT/library_obf" ]; then
    fail "the library (reached from main, -obf)" "no file was written:" "$(echo "$out" | grep -A3 "^error" | head -8)"
elif runs_quietly "the library (reached from main, -obf)" "$OUT/library_obf"; then
    passed=$((passed+1))
    echo "library, reached from main, -obf: $(echo "$out" | grep "^code generation")"
fi
#   4. -eager with -O: every function of the library through the optimizer and the verifier, and the
#      VM runs the file to its end as in 1.
rm -f "$OUT/library_O"
out=$(compile "$HERE/codegen/library_main.sharp" -o "$OUT/library_O" -eager -O -timings)
if [ ! -f "$OUT/library_O" ]; then
    fail "the library (-eager -O)" "no file was written:" "$(echo "$out" | grep -A3 "^error" | head -8)" "$(echo "$out" | tail -2)"
elif runs_quietly "the library (-eager -O)" "$OUT/library_O"; then
    passed=$((passed+1))
    echo "library, -eager -O: $(echo "$out" | grep "^code generation")"
    echo "$out" | grep "^time: layout"
fi

# ---- the suite's programs (doc/codegen.md 13.2 - 13.4)
#   Every suite case is compiled as a normal program - the VM starts it at platform.srt_init,
#   which sets the platform up and calls its main, as for the C++-built program - and run with
#   the JIT and with -nojit. It must RUN and print its `RESULT PASS` line both ways: a compile
#   that writes no file, a file the VM cannot run, or a RESULT that is not PASS fails the step.
#   (Until stage 4 a case could also WAIT: stop with E0974 "not generated yet" at a construct of a
#   later stage. Every construct has code now, so that is a failure like any other.)
#   codegen/suite_running.txt lists the cases that run (since stage 4: all of them); each of them
#   MUST run, and a case that runs and is not on it is reported, to be added.
SUITE=${SUITE:-${SHARP_WORK:-$HOME/sharp_work}/suite}
RUNNING="$HERE/codegen/suite_running.txt"
if [ -d "$SUITE/cases" ]; then
    mkdir -p "$OUT/suite"
    export SHARP COMPILER SUITE OUT
    export LIB_LIST="$LIB"
    suite_case() {
        f=$1; b=$(basename "$f" .sharp)
        rm -f "$OUT/suite/$b"
        timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB_LIST "$SUITE/testing.sharp" "$f" -o "$OUT/suite/$b" > "$OUT/suite/$b.txt" 2>&1
        if [ ! -f "$OUT/suite/$b" ]; then
            echo "$b BROKEN no file was written: $(grep -m1 "^error" "$OUT/suite/$b.txt" | cut -c1-160)"
            return
        fi
        with_jit=$(timeout -s KILL 120 "$SHARP" "$OUT/suite/$b" 2>&1 | grep -a "^RESULT" | head -1)
        without_jit=$(timeout -s KILL 120 "$SHARP" -nojit "$OUT/suite/$b" 2>&1 | grep -a "^RESULT" | head -1)
        if [[ "$with_jit" == "RESULT PASS"* ]] && [[ "$without_jit" == "RESULT PASS"* ]]; then echo "$b RUNS"
        else echo "$b BROKEN with the JIT: [${with_jit:-no RESULT line}], with -nojit: [${without_jit:-no RESULT line}]"; fi
    }
    export -f suite_case
    results=$(ls "$SUITE"/cases/*.sharp | xargs -P 4 -I{} bash -c 'suite_case {}' | sort)
    runs=$(echo "$results" | grep -c " RUNS$")
    while read -r line; do
        [ -n "$line" ] || continue
        fail "suite case ${line%% *}" "${line#* BROKEN }"
    done < <(echo "$results" | grep " BROKEN ")
    while read -r name; do
        [ -n "$name" ] || continue
        state=$(echo "$results" | grep "^$name ")
        case "$state" in
            *" RUNS") ;;
            "") fail "suite case $name" "listed in codegen/suite_running.txt but not in $SUITE/cases" ;;
        esac
    done < <(grep -v "^#" "$RUNNING" | sed 's/ *$//')
    new_ones=$(echo "$results" | grep " RUNS$" | cut -d' ' -f1 | while read -r name; do grep -qx "$name" "$RUNNING" || echo "$name"; done | tr '\n' ' ')
    [ -n "$new_ones" ] && echo "note: these suite cases run now and are not in codegen/suite_running.txt yet: $new_ones"
    if [ "$(echo "$results" | grep -c " BROKEN ")" -eq 0 ]; then passed=$((passed+1)); fi
    echo "suite cases: $runs of $(echo "$results" | grep -c .) run and pass (JIT and -nojit)"
fi

echo "codegen tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
