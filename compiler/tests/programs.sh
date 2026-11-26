#!/bin/bash
# The repository's own programs - the benchmarks (benchmark/) and the examples (examples/).
# EVERY ONE MUST BUILD: an example or a benchmark that does not compile is a failure, with the
# compiler's first error shown. What is run, and what it must do, is said where it is run below.
#
# The kostya benchmarks and the small examples were first built by the bootstrap compiler AND by
# the C++ compiler, and run: each must
# print the same as the C++-built program, with the JIT and with -nojit (doc/codegen.md 13.5).
# Each is ALSO built by the bootstrap with -O (the optimizer, codegen.md 7, 13.6): the optimized
# program must print the same again, with the JIT and with -nojit - these are the programs with
# the hot loops the optimizer is for.
#
#   tests/programs.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  SHARPC=<the C++ compiler>
#       LIBRARY=<the standard library folder>  OUT=<a folder to work in>
#       (defaults: ~/sharp_work/sharp_dev/sharp, ~/sharp_work/out/boot/sharpc_boot,
#        ~/sharp_work/sharpc_cl/sharpc, ~/sharp_work/stdlib, ~/sharp_work/out/boot/programs)
#
# Both compilers are called as the benchmark script calls one: <program> -j 1 -o <exe> -L <library>.
# The inputs are SMALL (a gates run must stay short); the full-size run and its times are in
# codegen.md 13.5. What is compared is the output with the times the programs measure taken out:
# a line `time: <seconds>`, and the `, <seconds>` base64 puts at the end of its two lines - and
# the exit code of the run.
# generate_json writes a file: the two files must be the same bytes.
#
# console_input waits for a password typed at a terminal (read_passwd), so it is built and not
# run. random and function_timer print something different on every run: they are built, run,
# and must end with exit code 0.
#
# Prints one line per failure and a summary; exits 1 when anything failed.
# THE C++ COMPILER IS GONE (17 Nov 2026). Sharp's first compiler was written in C++ and this
# script used it as a second opinion. It cannot read today's standard library, so it is no longer
# built or kept. SHARPC=<a second Sharp compiler, as one command> still turns the comparison on;
# without it (the default) the parts that say "C++" below are skipped.
# Then every program is compared with ITSELF: the plain build run by the interpreter (-nojit) is the
# reference, and the JIT, the -O build and the -O build under the JIT must print the same.
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
SHARPC=${SHARPC:-}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
OUT=${OUT:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/programs}
KOSTYA="$REPO/benchmark/kostya"
mkdir -p "$OUT"

passed=0
failed=0
fail() { failed=$((failed+1)); echo "FAIL  $1"; shift; for line in "$@"; do echo "        $line"; done; }

# build <name> <sources...>: <name>.boot by the bootstrap, <name>.O by the bootstrap with -O,
# <name>.cpp by the C++ compiler (once more when the C++ compiler wrote nothing: it dies on a
# signal now and then under load)
build() {
    local name=$1; shift
    rm -f "$OUT/$name.boot" "$OUT/$name.O" "$OUT/$name.cpp"
    timeout -s KILL 300 "$SHARP" "$COMPILER" "$@" -j 1 -o "$OUT/$name.boot" -L "$LIBRARY" > "$OUT/$name.boot.txt" 2>&1
    timeout -s KILL 300 "$SHARP" "$COMPILER" "$@" -j 1 -o "$OUT/$name.O" -L "$LIBRARY" -O > "$OUT/$name.O.txt" 2>&1
    if [ -x "$SHARPC" ]; then
        "$SHARPC" "$@" -j 1 -o "$OUT/$name.cpp" -L "$LIBRARY" > "$OUT/$name.cpp.txt" 2>&1
        [ -f "$OUT/$name.cpp" ] || "$SHARPC" "$@" -j 1 -o "$OUT/$name.cpp" -L "$LIBRARY" > "$OUT/$name.cpp.txt" 2>&1
    elif [ -f "$OUT/$name.boot" ]; then
        # no second compiler: the plain build stands in as the reference, and it is run by the
        # interpreter (REFERENCE_MODE) so that the JIT is still compared with something else
        cp "$OUT/$name.boot" "$OUT/$name.cpp"
    fi
}
REFERENCE_MODE=""; [ -x "$SHARPC" ] || REFERENCE_MODE="-nojit"

# what a program printed, without the VM's own last lines and the times it measured, and after
# it the exit code the run ended with (so a build that prints the same and then ends differently
# is a difference too)
printed() {
    local raw code
    raw=$(timeout -s KILL 300 "$SHARP" "$@" 2>&1); code=$?
    echo "$raw" | sed '/^Executed in [0-9]*us/,$d' | grep -av '^time: ' | sed 's/, [0-9.]*$//'
    echo "exit code $code"
}

# same_output <name> <arguments...>: the bootstrap's two builds (plain and -O), each with the JIT
# and with -nojit, print what the C++-built program prints
same_output() {
    local name=$1; shift
    if [ ! -f "$OUT/$name.boot" ] || [ ! -f "$OUT/$name.cpp" ]; then
        fail "$name" "not built: bootstrap $([ -f "$OUT/$name.boot" ] && echo yes || echo no), C++ $([ -f "$OUT/$name.cpp" ] && echo yes || echo no)" \
             "$(grep -m1 -A3 "^error" "$OUT/$name.boot.txt" | head -4)"
        return
    fi
    local reference; reference=$(printed $REFERENCE_MODE "$OUT/$name.cpp" "$@")
    local problems=()
    [ "$(echo "$reference" | wc -l)" -gt 1 ] || problems+=("the C++-built program printed nothing")
    for mode in "" "-nojit"; do
        got=$(printed $mode "$OUT/$name.boot" "$@")
        [ "$got" == "$reference" ] || problems+=("${mode:-with the JIT} it printed:" "$(echo "$got" | head -4 | tr '\n' '|')" "the C++-built one:" "$(echo "$reference" | head -4 | tr '\n' '|')")
    done
    # the build with -O: only a program that is built plain is asked for (an example an older
    # Sharp wrote is refused either way)
    if [ ! -f "$OUT/$name.O" ]; then
        problems+=("with -O the bootstrap wrote no file:" "$(grep -m1 -A3 "^error" "$OUT/$name.O.txt" | head -4)")
    else
        for mode in "" "-nojit"; do
            got=$(printed $mode "$OUT/$name.O" "$@")
            [ "$got" == "$reference" ] || problems+=("built with -O, ${mode:-with the JIT} it printed:" "$(echo "$got" | head -4 | tr '\n' '|')" "the C++-built one:" "$(echo "$reference" | head -4 | tr '\n' '|')")
        done
    fi
    if [ ${#problems[@]} -eq 0 ]; then passed=$((passed+1)); else fail "$name $*" "${problems[@]}"; fi
}

# ---- the kostya benchmarks, small inputs
# (a brainfuck program of our own: bench.b takes 15 s with the JIT and 100 s without)
printf '%s\n' '++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.' > "$OUT/hello.b"
build bf "$KOSTYA/brainfuck/bf.sharp";          same_output bf "$OUT/hello.b"
build base64 "$KOSTYA/base64/base64.sharp";     same_output base64 4096 16
build matmul "$KOSTYA/matmul/matmul.sharp";     same_output matmul 60
build primes "$KOSTYA/primes/primes.sharp";     same_output primes 100000 32338
build generate_json "$KOSTYA/json/generate_json.sharp"
build json "$KOSTYA/json/json.sharp"
if [ -f "$OUT/generate_json.boot" ] && [ -f "$OUT/generate_json.cpp" ]; then
    rm -f "$OUT"/entries_*.json
    "$SHARP" $REFERENCE_MODE "$OUT/generate_json.cpp" "$OUT/entries_cpp.json" 1000 > /dev/null 2>&1
    "$SHARP" "$OUT/generate_json.boot" "$OUT/entries_jit.json" 1000 > /dev/null 2>&1
    "$SHARP" -nojit "$OUT/generate_json.boot" "$OUT/entries_nojit.json" 1000 > /dev/null 2>&1
    [ -f "$OUT/generate_json.O" ] && "$SHARP" "$OUT/generate_json.O" "$OUT/entries_O.json" 1000 > /dev/null 2>&1
    if [ -s "$OUT/entries_cpp.json" ] && cmp -s "$OUT/entries_cpp.json" "$OUT/entries_jit.json" && cmp -s "$OUT/entries_cpp.json" "$OUT/entries_nojit.json" \
       && cmp -s "$OUT/entries_cpp.json" "$OUT/entries_O.json"; then
        passed=$((passed+1))
    else
        fail "generate_json" "the file it writes differs from the C++-built program's (or is empty): plain, -nojit, or built with -O"
    fi
    same_output json "$OUT/entries_cpp.json"
else
    fail "generate_json" "not built"
fi

# ---- the small examples (examples/*.sharp): each is built, plain and with -O, and run
NOT_RUN=" console_input "                 # waits for typing at a terminal
NOT_COMPARED=" random function_timer "    # prints something else every run: run, must end with 0
for file in "$REPO"/examples/*.sharp; do
    short=$(basename "$file" .sharp)
    name=example_$short
    build "$name" "$file"
    if [ ! -f "$OUT/$name.boot" ] || [ ! -f "$OUT/$name.O" ]; then
        fail "$name" "does not compile$([ -f "$OUT/$name.boot" ] && echo " with -O"):" \
             "$(grep -m1 -A3 "^error" "$OUT/$name.boot.txt" "$OUT/$name.O.txt" | head -5)"
    elif [[ "$NOT_RUN" == *" $short "* ]]; then
        passed=$((passed+1))
    elif [[ "$NOT_COMPARED" == *" $short "* ]]; then
        problems=()
        for exe in "$OUT/$name.boot" "$OUT/$name.O"; do
            for mode in "" "-nojit"; do
                (cd "$OUT" && timeout -s KILL 120 "$SHARP" $mode "$exe" > "$OUT/$name.run.txt" 2>&1)
                code=$?
                [ $code -eq 0 ] || problems+=("$(basename "$exe") ${mode:-with the JIT} ended with exit code $code:" "$(tail -3 "$OUT/$name.run.txt" | tr '\n' '|')")
            done
        done
        if [ ${#problems[@]} -eq 0 ]; then passed=$((passed+1)); else fail "$name" "${problems[@]}"; fi
    else
        same_output "$name" one "two words" 3
    fi
done

# ---- the programs that are only COMPILED here, several at a time: the GUI examples (they open a
# window), the example with a C++ half (the C++ is tests/cffi's business), the serialization
# benchmark (it runs for minutes). compile_only <name> <sources...>
compile_only() {
    local name=$1; shift
    rm -f "$OUT/$name.boot"
    timeout -s KILL 600 "$SHARP" "$COMPILER" "$@" -j 1 -o "$OUT/$name.boot" -L "$LIBRARY" > "$OUT/$name.boot.txt" 2>&1 < /dev/null
}
compiled=()
running=0
start_compile() {
    compiled+=("$1")
    compile_only "$@" &
    running=$((running+1))
    if [ $running -ge ${TEST_JOBS:-4} ]; then wait -n; running=$((running-1)); fi
}
for folder in "$REPO"/examples/gui/*/ "$REPO"/examples/testing/*/; do
    kind=$(basename "$(dirname "$folder")")
    mapfile -t sources < <(find "$folder" -name "*.sharp" -not -path "*/build/*" | sort)     # (a path may hold a space)
    [ ${#sources[@]} -gt 0 ] && start_compile "example_${kind}_$(basename "$folder")" "${sources[@]}"
done
start_compile "example_cpp_hello" "$REPO/examples/cpp_hello/helloworld.sharp"
start_compile "benchmark_serialization" "$REPO/benchmark/serialization/serialization.sharp"
wait
for name in "${compiled[@]}"; do
    if [ -f "$OUT/$name.boot" ]; then passed=$((passed+1))
    else fail "$name" "does not compile:" "$(grep -m1 -A3 "^error" "$OUT/$name.boot.txt" | head -4)"; fi
done

# ---- the examples that are tests: run, and the exit code is the check. `failing` shows what
# failures look like, so it must end with 1; every other one must pass its own tests (0).
# (They are the folders of examples/testing, and the UI tests in examples/gui/ui_tests.)
for name in "${compiled[@]}"; do
    case $name in
        example_testing_failing) wanted=1 ;;
        example_testing_*|example_gui_ui_tests) wanted=0 ;;
        *) continue ;;
    esac
    [ -f "$OUT/$name.boot" ] || continue
    (cd "$OUT" && timeout -s KILL 300 "$SHARP" "$OUT/$name.boot" > "$OUT/$name.run.txt" 2>&1)
    code=$?
    if [ $code -eq $wanted ]; then passed=$((passed+1))
    else fail "$name" "ended with exit code $code, not $wanted:" "$(grep -a -m6 "FAIL\|fail\|Exception\|error" "$OUT/$name.run.txt" | cut -c1-160)"; fi
done

# ---- the Benchmarks Game programs (benchmark/game): built plain and with -O, run on the small
# input, and what they print must be the bytes of benchmark/game/expected (written by the C programs)
GAME="$REPO/benchmark/game"
for wanted in "$GAME"/expected/*.txt; do
    pair=$(basename "$wanted" .txt); test=${pair%_*}; n=${pair##*_}
    name=game_$test
    build "$name" "$GAME/$test/$test.sharp"
    if [ ! -f "$OUT/$name.boot" ] || [ ! -f "$OUT/$name.O" ]; then
        fail "$name" "does not compile:" "$(grep -m1 -A3 "^error" "$OUT/$name.boot.txt" "$OUT/$name.O.txt" | head -5)"
        continue
    fi
    problems=()
    for exe in "$OUT/$name.boot" "$OUT/$name.O"; do
        timeout -s KILL 300 "$SHARP" "$exe" "$n" 2> /dev/null | sed '/^Executed in [0-9]*us/,$d' > "$OUT/$name.out"
        cmp -s "$OUT/$name.out" "$wanted" || problems+=("$(basename "$exe") $n does not print what benchmark/game/expected/$pair.txt holds")
    done
    if [ ${#problems[@]} -eq 0 ]; then passed=$((passed+1)); else fail "$name" "${problems[@]}"; fi
done

echo "programs: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
