#!/bin/bash
# The repository's own programs - the kostya benchmarks (benchmark/kostya) and the examples
# (examples/) - built by the bootstrap compiler AND by the C++ compiler, and run: each must
# print the same as the C++-built program, with the JIT and with -nojit (doc/codegen.md 13.5).
# Each is ALSO built by the bootstrap with -O (the optimizer, codegen.md 7, 13.6): the optimized
# program must print the same again, with the JIT and with -nojit - these are the programs with
# the hot loops the optimizer is for.
#
#   tests/programs.sh
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  SHARPC=<the C++ compiler>
#       LIBRARY=<the standard library folder>  OUT=<a folder to work in>
#       (defaults: ~/sharp_work/sharp_cl/sharp, ~/sharp_work/out/boot/sharpc_boot,
#        ~/sharp_work/sharpc_cl/sharpc, ~/sharp_work/stdlib, ~/sharp_work/out/boot/programs)
#
# Both compilers are called as the benchmark script calls one: <program> -j 1 -o <exe> -L <library>.
# The inputs are SMALL (a gates run must stay short); the full-size run and its times are in
# codegen.md 13.5. What is compared is the output with the times the programs measure taken out:
# a line `time: <seconds>`, and the `, <seconds>` base64 puts at the end of its two lines - and
# the exit code of the run.
# generate_json writes a file: the two files must be the same bytes.
#
# Examples that neither compiler builds (written for an older library or an older grammar:
# atomic, factorial, file_copy, function_timer, helloworld, list, new_class, random,
# string_to_int, task, thread, xor_neural_net) are checked for exactly that: the bootstrap
# must refuse each one the C++ compiler refuses, and build each one it builds. console_input
# waits for a password typed at a terminal (read_passwd), so it is built and not run.
#
# Prints one line per failure and a summary; exits 1 when anything failed.
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../../.." && pwd)"
SHARP=${SHARP:-$HOME/sharp_work/sharp_cl/sharp}
COMPILER=${COMPILER:-$HOME/sharp_work/out/boot/sharpc_boot}
SHARPC=${SHARPC:-$HOME/sharp_work/sharpc_cl/sharpc}
LIBRARY=${LIBRARY:-$HOME/sharp_work/stdlib}
OUT=${OUT:-$HOME/sharp_work/out/boot/programs}
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
    "$SHARPC" "$@" -j 1 -o "$OUT/$name.cpp" -L "$LIBRARY" > "$OUT/$name.cpp.txt" 2>&1
    [ -f "$OUT/$name.cpp" ] || "$SHARPC" "$@" -j 1 -o "$OUT/$name.cpp" -L "$LIBRARY" > "$OUT/$name.cpp.txt" 2>&1
}

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
    local reference; reference=$(printed "$OUT/$name.cpp" "$@")
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
    "$SHARP" "$OUT/generate_json.cpp" "$OUT/entries_cpp.json" 1000 > /dev/null 2>&1
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

# ---- the examples
for file in "$REPO"/examples/*.sharp; do
    name=example_$(basename "$file" .sharp)
    build "$name" "$file"
    if [ -f "$OUT/$name.cpp" ] && [ ! -f "$OUT/$name.boot" ]; then
        fail "$name" "the C++ compiler builds it, the bootstrap does not:" "$(grep -m1 -A3 "^error" "$OUT/$name.boot.txt" | head -4)"
    elif [ ! -f "$OUT/$name.cpp" ] && [ -f "$OUT/$name.boot" ]; then
        fail "$name" "the C++ compiler refuses it, the bootstrap builds it:" "$(grep -m1 -A2 "error" "$OUT/$name.cpp.txt" | head -3)"
    elif [ ! -f "$OUT/$name.cpp" ]; then
        passed=$((passed+1))     # refused by both: an example of an older Sharp
    elif [ "$name" = example_console_input ]; then
        passed=$((passed+1))     # built by both; it waits at a terminal, so it is not run
    else
        same_output "$name" one "two words" 3
    fi
done

echo "programs: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
