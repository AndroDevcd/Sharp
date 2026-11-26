#!/bin/bash
# Runs the whole benchmark set once and writes one line per test to a results
# file:   name | input | checked output | seconds | peak memory (MB)
#
#   run.sh <label> [compiler options...]
#
#   run.sh plain            Sharp, compiled as is
#   run.sh O -O             Sharp, compiled with the bytecode optimizer
#   run.sh c                the C programs next to the Sharp ones (gcc -O3): what the same work
#                           costs without a runtime. The json input must be there already
#                           (a Sharp run makes it).
#
# Environment:
#   SHARPC, SHARP    the compiler and the runtime          (default: sharpc, sharp)
#   LIB              the standard library folder for -L    (default: ../../stdlib)
#   OUT              where executables and results go      (default: ./out)
#   RUNTIME_FLAGS    extra runtime options, e.g. -nojit
#   JSON_ENTRIES     entries in the json input             (default 131072, a quarter of the original
#                    524288: a Sharp string took 16 bytes per character when this was written, and
#                    the full 115 MB file needed more memory than the 3 GB it was measured with)
#   ONLY             run just one test (bench.b, mandel.b, base64, json, matmul, primes)
#
# "seconds" is the time the program measures around the work itself, as in the
# original benchmark: reading input and the self test are not counted.
HERE=$(cd "$(dirname "$0")" && pwd)
LABEL=${1:-plain}; shift
FLAGS="$*"
SHARPC=${SHARPC:-sharpc}
SHARP=${SHARP:-sharp}
LIB=${LIB:-$HERE/../../stdlib}
OUT=${OUT:-$HERE/out}
JSON_ENTRIES=${JSON_ENTRIES:-131072}
JSON_FILE=${JSON_FILE:-/tmp/sharp_bench_$JSON_ENTRIES.json}
RESULTS=$OUT/results_$LABEL.txt
mkdir -p "$OUT/$LABEL"
: > "$RESULTS"

build() {   # $1 = source (without its ending), $2 = name
    rm -f "$OUT/$LABEL/$2"
    if [ "$LABEL" = c ]; then
        [ -f "$HERE/$1.c" ] || return 0         # (a helper that only Sharp has: generate_json)
        gcc -O3 -march=native "$HERE/$1.c" -o "$OUT/$LABEL/$2" -lm > "$OUT/$LABEL/$2.compile.txt" 2>&1
    else
        $SHARPC "$HERE/$1.sharp" -j 1 -o "$OUT/$LABEL/$2" -L "$LIB" $FLAGS > "$OUT/$LABEL/$2.compile.txt" 2>&1
    fi
    [ -f "$OUT/$LABEL/$2" ] || { echo "$2 did not compile, see $OUT/$LABEL/$2.compile.txt"; return 1; }
}

run() {     # $1 = test name, $2 = input description, $3 = executable, rest = arguments
    local name=$1 input=$2 exe=$3; shift 3
    [ -n "$ONLY" ] && [ "$ONLY" != "$name" ] && return
    local log="$OUT/$LABEL/$name.out.txt"
    if [ "$LABEL" = c ]; then
        /usr/bin/time -f "peak_kb %M" "$OUT/$LABEL/$exe" "$@" > "$log" 2>&1
    else
        /usr/bin/time -f "peak_kb %M" $SHARP -mem 2800mb $RUNTIME_FLAGS "$OUT/$LABEL/$exe" "$@" > "$log" 2>&1
    fi
    local seconds=$(grep '^time: ' "$log" | tail -1 | cut -d' ' -f2)
    local peak=$(grep '^peak_kb ' "$log" | tail -1 | cut -d' ' -f2)
    local output=$(grep -v '^time: \|^peak_kb \|^$\|^Executed in\|^vm exiting' "$log" | head -2 | tr '\n' ' ' | cut -c1-110)
    echo "$name | $input | $output| ${seconds:-FAILED} | $(( ${peak:-0} / 1024 ))" | tee -a "$RESULTS"
}

build brainfuck/bf bf
build base64/base64 base64
build json/json json
build json/generate_json generate_json
build matmul/matmul matmul
build primes/primes primes

if [ ! -f "$JSON_FILE" ] && { [ -z "$ONLY" ] || [ "$ONLY" = "json" ]; }; then
    if [ "$LABEL" = c ]; then echo "the json input $JSON_FILE is not there: run the Sharp set first (it makes it)"
    else $SHARP -mem 1gb "$OUT/$LABEL/generate_json" "$JSON_FILE" "$JSON_ENTRIES" > /dev/null 2>&1; fi
fi

run bench.b  "bench.b"                       bf "$HERE/brainfuck/bench.b"
run mandel.b "mandel.b"                      bf "$HERE/brainfuck/mandel.b"
run base64   "131072 bytes x 8192"           base64 131072 8192
run json     "$JSON_ENTRIES entries"         json "$JSON_FILE"
run matmul   "1500 x 1500"                   matmul 1500
run primes   "up to 5000000, prefix 32338"   primes 5000000 32338
echo "done" >> "$RESULTS"
