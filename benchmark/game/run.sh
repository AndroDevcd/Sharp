#!/bin/bash
# THE BENCHMARKS GAME, Sharp against the other languages on this machine.   (Linux / WSL)
#
# Six programs of https://benchmarksgame-team.pages.debian.net/benchmarksgame, each written the
# same plain way in every language (one thread, the same algorithm as the C file next to it):
#
#     nbody  spectralnorm  fannkuchredux  binarytrees  mandelbrot  fasta
#
#   run.sh                      every test in every language that is installed
#   run.sh sharp c node         only these languages   (C is always run: it is the reference)
#   run.sh list                 the languages, and which of them are installed
#
# What it does for each program: builds it; runs it on a small input and compares what it prints
# with the folder `expected` (a program that prints something else is not timed); then runs it on
# the real input and measures the whole run, from start to exit, as the site does.
#
# It writes, in the folder `results` next to this script:
#     results.json      every measurement                (chart.py draws chart.html from it)
#     results.md        the same as tables
#
# Environment:
#   SIZE        small | medium | full          how big the inputs are (default medium;
#               full is the site's own sizes: slow languages then take many minutes a test)
#   TESTS       only these tests               (default: all six)
#   RUNS        how often each program is timed; the best run counts   (default 1)
#   TIMEOUT     seconds one run may take                               (default 600)
#   OUT         where programs are built and output goes               (default ~/sharp_game)
#   RESULTS     where results.json, results.md and chart.html go       (default: the folder `results` here)
#   UPDATE      1: only the languages named are run again; every other language keeps the rows it
#               has in results.json (they must be of the same SIZE):  UPDATE=1 run.sh sharp sharp_O
#   SHARP       the Sharp runtime              (default: the one build-scripts/build.sh made,
#   SHARPC      the Sharp compiler              ~/sharp_work; else `sharp` and `sharpc` on the PATH)
#   LIB         the standard library for -L
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-$HOME/sharp_game}
BIN=$OUT/bin; LOGS=$OUT/logs
RESULTS=${RESULTS:-$HERE/results}
SIZE=${SIZE:-medium}
RUNS=${RUNS:-1}
TIMEOUT=${TIMEOUT:-600}
ALL_TESTS="nbody spectralnorm fannkuchredux binarytrees mandelbrot fasta"
TESTS=${TESTS:-$ALL_TESTS}
ALL_LANGS="c rust go java csharp node pypy luajit sharp_O sharp php python ruby lua perl tcl"

# ---- Sharp: the build of this repository when there is one
WORK=${SHARP_WORK:-$HOME/sharp_work}
if [ -z "$SHARP" ] && [ -x "$WORK/sharp_dev/sharp" ] && [ -f "$WORK/out/boot/sharpc_boot" ]; then
    SHARP="$WORK/sharp_dev/sharp"; SHARPC="$SHARP $WORK/out/boot/sharpc_boot"; LIB=${LIB:-$WORK/stdlib}
fi
SHARP=${SHARP:-sharp}; SHARPC=${SHARPC:-sharpc}

# ---- the inputs
size_of() {     # $1 = test: its small input, and the input that is timed
    case "$1:$SIZE" in
        nbody:small) echo 500000 ;;             nbody:medium) echo 5000000 ;;           nbody:full) echo 50000000 ;;
        spectralnorm:small) echo 500 ;;         spectralnorm:medium) echo 2000 ;;       spectralnorm:full) echo 5500 ;;
        fannkuchredux:small) echo 9 ;;          fannkuchredux:medium) echo 10 ;;        fannkuchredux:full) echo 12 ;;
        binarytrees:small) echo 14 ;;           binarytrees:medium) echo 17 ;;          binarytrees:full) echo 21 ;;
        mandelbrot:small) echo 1000 ;;          mandelbrot:medium) echo 4000 ;;         mandelbrot:full) echo 16000 ;;
        fasta:small) echo 250000 ;;             fasta:medium) echo 2500000 ;;           fasta:full) echo 25000000 ;;
        *) echo "SIZE is small, medium or full" >&2; exit 2 ;;
    esac
}
check_size_of() {
    case "$1" in nbody) echo 1000 ;; spectralnorm) echo 100 ;; fannkuchredux) echo 7 ;; binarytrees) echo 10 ;; mandelbrot) echo 200 ;; fasta) echo 1000 ;; esac
}

# ---- the languages:  name | the program it needs | how its version is asked for
describe() {
    case "$1" in
        c)       echo "C (gcc)|gcc|gcc --version" ;;
        rust)    echo "Rust|rustc|rustc --version" ;;
        go)      echo "Go|go|go version" ;;
        java)    echo "Java|javac|java -version" ;;
        csharp)  echo "C# (Mono)|mcs|mono --version" ;;
        node)    echo "JavaScript (Node)|node|node --version" ;;
        pypy)    echo "Python (PyPy)|pypy3|pypy3 --version" ;;
        luajit)  echo "Lua (LuaJIT)|luajit|luajit -v" ;;
        sharp_O) echo "Sharp -O|${SHARP%% *}|$SHARP -V" ;;
        sharp)   echo "Sharp|${SHARP%% *}|$SHARP -V" ;;
        php)     echo "PHP|php|php --version" ;;
        python)  echo "Python|python3|python3 --version" ;;
        ruby)    echo "Ruby|ruby|ruby --version" ;;
        lua)     echo "Lua|lua5.4|lua5.4 -v" ;;
        perl)    echo "Perl|perl|perl -e print($^V)" ;;
        tcl)     echo "Tcl|tclsh|echo puts [info patchlevel]" ;;
    esac
}
name_of()      { describe "$1" | cut -d'|' -f1; }
installed()    { command -v "$(describe "$1" | cut -d'|' -f2)" > /dev/null 2>&1; }
version_of() {
    local ask; ask=$(describe "$1" | cut -d'|' -f3)
    case "$1" in
        tcl) echo 'puts [info patchlevel]' | tclsh 2>&1 | head -1 ;;
        perl) perl -e 'print $^V' ;;
        *) $ask 2>&1 | grep -v "^$" | head -1 ;;
    esac | tr -d '"\\' | cut -c1-80
}
source_of() {   # $1 = language: the ending of its source files
    case "$1" in c) echo c ;; rust) echo rs ;; go) echo go ;; java) echo java ;; csharp) echo cs ;; node) echo js ;;
        pypy|python) echo py ;; luajit|lua) echo lua ;; sharp|sharp_O) echo sharp ;; php) echo php ;; ruby) echo rb ;; perl) echo pl ;; tcl) echo tcl ;; esac
}

# build <language> <test>: makes what `command_of` then starts. Nothing to do for a language that runs its source.
build() {
    local lang=$1 t=$2 src="$HERE/$2/$2.$(source_of "$1")" log="$LOGS/$2.$1.build.txt"
    [ -f "$src" ] || return 1
    case "$lang" in
        c)       gcc -O3 -march=native -ffp-contract=off "$src" -o "$BIN/$t.c" -lm > "$log" 2>&1 ;;
        rust)    rustc -C opt-level=3 -C target-cpu=native "$src" -o "$BIN/$t.rust" > "$log" 2>&1 ;;
        go)      ( cd "$BIN" && go build -o "$BIN/$t.go.bin" "$src" ) > "$log" 2>&1 ;;
        java)    mkdir -p "$BIN/java" && javac -d "$BIN/java" "$src" > "$log" 2>&1 ;;
        csharp)  mcs -optimize+ -out:"$BIN/$t.exe" "$src" > "$log" 2>&1 ;;
        sharp)   rm -f "$BIN/$t.sharp";   $SHARPC "$src" -j 1 -o "$BIN/$t.sharp" ${LIB:+-L "$LIB"} > "$log" 2>&1;      [ -f "$BIN/$t.sharp" ] ;;
        sharp_O) rm -f "$BIN/$t.sharp_O"; $SHARPC "$src" -j 1 -o "$BIN/$t.sharp_O" ${LIB:+-L "$LIB"} -O > "$log" 2>&1; [ -f "$BIN/$t.sharp_O" ] ;;
        *)       true ;;
    esac
}

# command_of <language> <test>: sets COMMAND to what starts the program (the input is added by the caller)
command_of() {
    local lang=$1 t=$2 src="$HERE/$2/$2.$(source_of "$1")"
    case "$lang" in
        c)       COMMAND=("$BIN/$t.c") ;;
        rust)    COMMAND=("$BIN/$t.rust") ;;
        go)      COMMAND=("$BIN/$t.go.bin") ;;
        java)    COMMAND=(java -cp "$BIN/java" "$t") ;;
        csharp)  COMMAND=(mono "$BIN/$t.exe") ;;
        node)    COMMAND=(node "$src") ;;
        pypy)    COMMAND=(pypy3 "$src") ;;
        python)  COMMAND=(python3 "$src") ;;
        luajit)  COMMAND=(luajit "$src") ;;
        lua)     COMMAND=(lua5.4 "$src") ;;
        sharp)   COMMAND=($SHARP $SHARP_FLAGS "$BIN/$t.sharp") ;;
        sharp_O) COMMAND=($SHARP $SHARP_FLAGS "$BIN/$t.sharp_O") ;;
        php)     COMMAND=(php "$src") ;;
        ruby)    COMMAND=(ruby "$src") ;;
        perl)    COMMAND=(perl "$src") ;;
        tcl)     COMMAND=(tclsh "$src") ;;
    esac
}

if [ "$1" = list ]; then
    for lang in $ALL_LANGS; do
        if installed "$lang"; then echo "$lang    $(name_of "$lang"): $(version_of "$lang")"; else echo "$lang    $(name_of "$lang"): not installed"; fi
    done
    exit 0
fi

# ---- which languages
LANGS=${*:-$ALL_LANGS}
case " $LANGS " in *" c "*) ;; *) LANGS="c $LANGS" ;; esac
chosen=""
for lang in $LANGS; do
    describe "$lang" | grep -q . || { echo "no language $lang (run.sh list)"; exit 2; }
    if installed "$lang"; then chosen="$chosen $lang"; else echo "$(name_of "$lang") is not installed: left out"; fi
done
LANGS=$chosen
for t in $TESTS; do size_of "$t" > /dev/null || exit 2; done

mkdir -p "$BIN" "$LOGS" "$RESULTS"
ROWS=$OUT/rows.txt; : > "$ROWS"         # test|language|n|status|seconds|cpu seconds|memory MB

echo "sizes: $SIZE    runs: $RUNS    time limit: ${TIMEOUT}s    work folder: $OUT"
for t in $TESTS; do
    n=$(size_of "$t"); small=$(check_size_of "$t")
    echo; echo "== $t $n"
    reference=""
    for lang in $LANGS; do
        status=ok; seconds=""; cpu=""; memory=""
        out="$LOGS/$t.$lang.out"
        if ! build "$lang" "$t"; then
            status="did not build"
        else
            command_of "$lang" "$t"
            # the small input first: the program must print what C prints
            timeout -s KILL 120 "${COMMAND[@]}" "$small" > "$out" 2> "$LOGS/$t.$lang.err.txt"
            if ! cmp -s "$out" "$HERE/expected/${t}_$small.txt"; then
                status="wrong output"
            else
                for run in $(seq "$RUNS"); do
                    /usr/bin/time -f "%e %U %S %M" -o "$LOGS/time.txt" timeout -s KILL "$TIMEOUT" "${COMMAND[@]}" "$n" > "$out" 2> "$LOGS/$t.$lang.err.txt"
                    code=$?
                    if [ $code -eq 137 ]; then status="timed out"; break; fi
                    if [ $code -ne 0 ]; then status="failed"; break; fi
                    read -r elapsed user system kb < <(tail -1 "$LOGS/time.txt")
                    if [ -z "$seconds" ] || awk "BEGIN { exit !($elapsed < $seconds) }"; then
                        seconds=$elapsed; cpu=$(awk "BEGIN { printf \"%.2f\", $user + $system }"); memory=$(( kb / 1024 ))
                    fi
                done
                if [ "$status" = ok ]; then
                    sum=$(md5sum < "$out" | cut -d' ' -f1)
                    [ "$lang" = c ] && reference=$sum
                    [ "$sum" = "$reference" ] || status="wrong output"
                fi
            fi
        fi
        rm -f "$out"
        [ "$status" = ok ] || { seconds=""; cpu=""; memory=""; }
        echo "$t|$lang|$n|$status|$seconds|$cpu|$memory" >> "$ROWS"
        if [ "$status" = ok ]; then printf "  %-20s %8.2f s  %6d MB\n" "$(name_of "$lang")" "$seconds" "$memory"
        else printf "  %-20s %s\n" "$(name_of "$lang")" "$status"; fi
    done
done

# ---- results.json
JSON=$RESULTS/results.json
KEPT=""
if [ "$UPDATE" = 1 ] && [ -f "$JSON" ]; then KEPT=$JSON; JSON=$RESULTS/results.new.json; fi
{
    echo "{"
    echo "  \"date\": \"$(date +%Y-%m-%d)\","
    echo "  \"machine\": \"$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ *//'), $(nproc) threads, $(awk '/MemTotal/ {printf "%.0f", $2/1048576}' /proc/meminfo) GB, $(uname -sr)\","
    echo "  \"size\": \"$SIZE\","
    echo "  \"runs\": $RUNS,"
    echo "  \"languages\": ["
    first=1
    for lang in $LANGS; do
        [ $first = 1 ] || echo ","; first=0
        printf '    { "id": "%s", "name": "%s", "version": "%s" }' "$lang" "$(name_of "$lang")" "$(version_of "$lang")"
    done
    echo; echo "  ],"
    echo "  \"results\": ["
    first=1
    while IFS='|' read -r t lang n status seconds cpu memory; do
        [ $first = 1 ] || echo ","; first=0
        printf '    { "test": "%s", "language": "%s", "n": %s, "status": "%s", "seconds": %s, "cpu_seconds": %s, "memory_mb": %s }' \
               "$t" "$lang" "$n" "$status" "${seconds:-null}" "${cpu:-null}" "${memory:-null}"
    done < "$ROWS"
    echo; echo "  ]"
    echo "}"
} > "$JSON"
# (UPDATE=1: the rows just measured go into the results that were there)
if [ -n "$KEPT" ]; then python3 "$HERE/chart.py" --merge "$KEPT" "$JSON" || exit 1; rm -f "$JSON"; JSON=$KEPT; fi

# ---- results.md, and the chart
if command -v python3 > /dev/null; then
    python3 "$HERE/chart.py" "$JSON" "$RESULTS"
fi
echo; echo "results: $RESULTS  (results.json, results.md, chart.html)"
