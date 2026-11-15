#!/bin/bash
# Runs the whole benchmark set once and writes one line per test to a results
# file:   name | input | checked output | seconds | peak memory (MB)
#
#   run.sh <label> [compiler options...]
#
#   run.sh plain            compiled as is
#   run.sh O -O             compiled with the bytecode optimizer
#
# Environment:
#   SHARPC, SHARP    the compiler and the runtime          (default: sharpc, sharp)
#   LIB              the standard library folder for -L    (default: ../../lib/support/0.4.0)
#   OUT              where executables and results go      (default: ./out)
#   RUNTIME_FLAGS    extra runtime options, e.g. -nojit
#   JSON_ENTRIES     entries in the json input             (default 131072, a quarter of the original
#                    524288: a Sharp string takes 16 bytes per character, and the full 115 MB file
#                    needs more memory than the 3 GB this was measured with)
#   ONLY             run just one test (bench.b, mandel.b, base64, json, matmul, primes)
#
# "seconds" is the time the program measures around the work itself, as in the
# original benchmark: reading input and the self test are not counted.
HERE=$(cd "$(dirname "$0")" && pwd)
LABEL=${1:-plain}; shift
FLAGS="$*"
SHARPC=${SHARPC:-sharpc}
SHARP=${SHARP:-sharp}
LIB=${LIB:-$HERE/../../lib/support/0.4.0}
OUT=${OUT:-$HERE/out}
JSON_ENTRIES=${JSON_ENTRIES:-131072}
JSON_FILE=${JSON_FILE:-/tmp/sharp_bench_$JSON_ENTRIES.json}
RESULTS=$OUT/results_$LABEL.txt
mkdir -p "$OUT/$LABEL"
: > "$RESULTS"

build() {   # $1 = source, $2 = name
    rm -f "$OUT/$LABEL/$2"
    $SHARPC "$HERE/$1" -j 1 -o "$OUT/$LABEL/$2" -L "$LIB" $FLAGS > "$OUT/$LABEL/$2.compile.txt" 2>&1
    [ -f "$OUT/$LABEL/$2" ] || { echo "$2 did not compile, see $OUT/$LABEL/$2.compile.txt"; return 1; }
}

run() {     # $1 = test name, $2 = input description, $3 = executable, rest = arguments
    local name=$1 input=$2 exe=$3; shift 3
    [ -n "$ONLY" ] && [ "$ONLY" != "$name" ] && return
    local log="$OUT/$LABEL/$name.out.txt"
    /usr/bin/time -f "peak_kb %M" $SHARP -mem 2800mb $RUNTIME_FLAGS "$OUT/$LABEL/$exe" "$@" > "$log" 2>&1
    local seconds=$(grep '^time: ' "$log" | tail -1 | cut -d' ' -f2)
    local peak=$(grep '^peak_kb ' "$log" | tail -1 | cut -d' ' -f2)
    local output=$(grep -v '^time: \|^peak_kb \|^$\|^Executed in\|^vm exiting' "$log" | head -2 | tr '\n' ' ' | cut -c1-110)
    echo "$name | $input | $output| ${seconds:-FAILED} | $(( ${peak:-0} / 1024 ))" | tee -a "$RESULTS"
}

build brainfuck/bf.sharp bf
build base64/base64.sharp base64
build json/json.sharp json
build json/generate_json.sharp generate_json
build matmul/matmul.sharp matmul
build primes/primes.sharp primes

if [ ! -f "$JSON_FILE" ] && { [ -z "$ONLY" ] || [ "$ONLY" = "json" ]; }; then
    $SHARP -mem 1gb "$OUT/$LABEL/generate_json" "$JSON_FILE" "$JSON_ENTRIES" > /dev/null 2>&1
fi

run bench.b  "bench.b"                       bf "$HERE/brainfuck/bench.b"
run mandel.b "mandel.b"                      bf "$HERE/brainfuck/mandel.b"
run base64   "131072 bytes x 8192"           base64 131072 8192
run json     "$JSON_ENTRIES entries"         json "$JSON_FILE"
run matmul   "1500 x 1500"                   matmul 1500
run primes   "up to 5000000, prefix 32338"   primes 5000000 32338
echo "done" >> "$RESULTS"
