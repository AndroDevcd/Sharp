#!/bin/bash
# SELF-HOSTING: the compiler compiles itself (doc/codegen.md 8 and 13.7).
#
#       stage 1   the C++ compiler compiles the bootstrap sources           -> sharpc_boot   (bootc.sh)
#       stage 2   sharpc_boot (stage 1) compiles the same sources           -> stage2/sharpc
#       stage 3   stage2/sharpc compiles the same sources                   -> stage3/sharpc
#
# Stage 1 and stage 2 are the SAME program (the bootstrap's source) translated by two different
# compilers. If the bootstrap translates correctly they behave alike, so what they write for the
# same input must be the same bytes: stage 2 and stage 3 must be identical files (the FIXED POINT).
# A difference means the bootstrap mistranslated itself, or that its output depends on something
# it must not depend on (the order in which threads finish, the order of a hash table, an address).
#
#   tests/selfhost.sh
#       SHARP=<the runtime>  COMPILER=<stage 1: the bootstrap built by the C++ compiler>
#       SOURCES=<the folder of the bootstrap's sources>   LIBRARY=<the standard library folder>
#       SUITE=<the suite's project folder (tests/suite)>  OUT=<a folder to work in>
#       QUICK=1   leave out the suite (part C and check 11; it was about 12 minutes as 219 programs)
#       (defaults: ~/sharp_work/sharp_dev/sharp, ~/sharp_work/out/boot/sharpc_boot,
#        ~/sharp_work/src/compiler - the copy bootc.sh built stage 1 from -,
#        ~/sharp_work/stdlib, ~/sharp_work/suite, ~/sharp_work/out/boot/selfhost)
#
# What is checked:
#   A. THE FIXED POINT
#      1. stage 2 is written, with no error; stage 3 is written, with no error
#      2. stage 2 and stage 3 are the same bytes
#      3. the parallel parse does not reach the output: stage 2 compiling itself on 4 threads
#         and on the default number of threads writes those same bytes again
#      4. the same with the optimizer: stage 1 with -O and stage 2 with -O write the same
#         file (the optimized compiler), and THAT compiler, compiling itself with -O, writes it again
#   B. STAGE 2 AS THE COMPILER: the same messages. Both compilers run here on a runtime started with
#      a 1 MB stack (`sharp -istack 1mb`; the default is 64 KB). Stage 2 needs about twice the stack
#      per level of nesting that stage 1 needs (its frames are larger, doc/codegen.md 11.8), so with
#      the default stack it runs out of stack on shallower code than stage 1 does. With room to
#      spare, the limits the compiler COUNTS (256 levels in the parser, 150 statements in the
#      analysis) decide in both, and both must then say exactly the same.
#      5. the standard library, every warning shown (-wlib -wextra): stage 2 prints what stage 1 prints
#      6. the frontend tests and the backend tests pass with stage 2 as the compiler
#      7. every program that must be refused (tests/backend/fail, tests/parse/fail): stage 2 prints,
#         character for character, the messages stage 1 prints
#   C. STAGE 2 AS THE COMPILER: the same results
#      8. the suite (one project), built by stage 1 and by stage 2: the same executable, byte for
#         byte, and the same output of the compiler - once plain and once with -O. (Test 8 of
#         build-scripts/test.sh runs what stage 1 builds and judges its results; identical bytes
#         behave identically, so that judgement holds for stage 2's.) Every compiler builds it in
#         the same folder, a copy of the suite's (OUT/suite_project), one after the other.
#   D. THE OPTIMIZED COMPILER AS THE COMPILER. The compiler built by itself with -O is the one a
#      user keeps (README.md), and it is the only one of the three every optimizer pass has been
#      through. Part A proves it compiles ITSELF as stage 1 does; here it compiles other programs:
#      9. the standard library, every warning shown: it prints what stage 1 prints
#     10. every program that must be refused: the messages of stage 1, character for character
#     11. the suite: the executable stage 1 builds, byte for byte
#
# The time each compiler takes for the self-compile is printed (13.7 keeps the numbers).
# Prints one line per failure and a summary; exits 1 when anything failed.
HERE="$(cd "$(dirname "$0")" && pwd)"
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
SOURCES=${SOURCES:-${SHARP_WORK:-$HOME/sharp_work}/src/compiler}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
SUITE=${SUITE:-${SHARP_WORK:-$HOME/sharp_work}/suite}
OUT=${OUT:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/selfhost}
rm -rf "${OUT:?}"
mkdir -p "$OUT"
cd "$OUT" || exit 1

# the bootstrap's own sources: every .sharp file but its tests, as absolute paths, sorted
SRC=$(find "$SOURCES" -name "*.sharp" -not -path "*/tests/*" | sort)
LIB=$(find "$LIBRARY" -name "*.sharp" | sort)

passed=0
failed=0
pass() { passed=$((passed+1)); }
fail() { failed=$((failed+1)); echo "FAIL  $1"; shift; for line in "$@"; do echo "        $line"; done; }
# the lines that differ from run to run for reasons that are not the compiler's: times
steady() { grep -v "^Executed in\|^vm exiting\|^time:\|^$" "$1"; }

# self_compile <folder> <compiler> <options...>: that compiler compiles the bootstrap into
# <folder>/sharpc. Every stage writes a file of the SAME name from inside its own folder: the
# name given to -o is written into the file.
self_compile() {
    local folder=$1 compiler=$2; shift 2
    mkdir -p "$folder"
    local started=$(date +%s%N)
    (cd "$folder" && timeout -s KILL 900 "$SHARP" -mem 4gb "$compiler" $SRC -o sharpc -L "$LIBRARY" -timings "$@" > compile.txt 2>&1)
    local ms=$(( ($(date +%s%N) - started) / 1000000 ))
    echo "        $folder: $ms ms   $(grep -c "^error" "$folder/compile.txt") error(s), $(stat -c %s "$folder/sharpc" 2>/dev/null || echo no) bytes"
}
written() { # written <folder>: the self-compile wrote its file and reported no error
    if [ -f "$1/sharpc" ] && grep -q "^code generation: " "$1/compile.txt" && ! grep -q "^error" "$1/compile.txt"; then pass
    else fail "$1" "the compiler was not written:" "$(grep -m1 -A4 "^error\|Unhandled\|SIGSEGV" "$1/compile.txt" | head -6)" "$(steady "$1/compile.txt" | tail -2)"; fi
}
same() { # same <name> <file a> <file b>
    if [ -f "$2" ] && cmp -s "$2" "$3"; then pass
    else fail "$1" "$2 and $3 are not the same bytes: $(cmp "$2" "$3" 2>&1 | head -1)" "(add -listing to both compiles and diff the two .listing files to see what differs)"; fi
}

# ---- A. the fixed point
echo "self-compiles (the bootstrap's $(echo "$SRC" | wc -l) files + the library):"
self_compile stage2 "$COMPILER" -j 1
written stage2
self_compile stage3 "$OUT/stage2/sharpc" -j 1
written stage3
same "fixed point" stage2/sharpc stage3/sharpc

self_compile stage3_j4 "$OUT/stage2/sharpc" -j 4
same "4 threads" stage2/sharpc stage3_j4/sharpc
self_compile stage3_threads "$OUT/stage2/sharpc"
same "default threads" stage2/sharpc stage3_threads/sharpc

self_compile stage2_O "$COMPILER" -j 1 -O
written stage2_O
self_compile stage3_O "$OUT/stage2/sharpc" -j 1 -O
same "-O, stage 1 and stage 2" stage2_O/sharpc stage3_O/sharpc
self_compile stage4_O "$OUT/stage2_O/sharpc" -j 1 -O
same "-O fixed point" stage2_O/sharpc stage4_O/sharpc

STAGE2="$OUT/stage2/sharpc"
if [ ! -f "$STAGE2" ]; then
    echo "self-hosting: $passed passed, $failed failed   (stage 2 was not written: parts B and C were not run; work folder: $OUT)"
    exit 1
fi

# ---- B. the same messages (on a runtime with a 1 MB stack: see the top of this script)
ROOMY="$OUT/sharp_roomy.sh"
printf '#!/bin/bash
exec "%s" -istack 1mb -mem 4gb "$@"
' "$SHARP" > "$ROOMY"
chmod +x "$ROOMY"
"$ROOMY" "$COMPILER" -c $LIB -wlib -wextra > library_stage1.txt 2>&1
"$ROOMY" "$STAGE2" -c $LIB -wlib -wextra > library_stage2.txt 2>&1
if grep -q "^analysis complete" library_stage2.txt && [ "$(steady library_stage1.txt)" = "$(steady library_stage2.txt)" ]; then pass
else fail "library" "stage 2 does not print what stage 1 prints for the standard library:" "$(diff <(steady library_stage1.txt) <(steady library_stage2.txt) | head -6)"; fi

out=$(SHARP=$ROOMY COMPILER=$STAGE2 bash "$HERE/run.sh" 2>&1) && pass || fail "frontend tests with stage 2" "$(echo "$out" | tail -6)"
echo "        stage 2, frontend tests: $(echo "$out" | tail -1)"
out=$(SHARP=$ROOMY COMPILER=$STAGE2 LIBRARY=$LIBRARY bash "$HERE/backend.sh" 2>&1) && pass || fail "backend tests with stage 2" "$(echo "$out" | tail -6)"
echo "        stage 2, backend tests: $(echo "$out" | tail -1)"

mkdir -p refused
export SHARP COMPILER STAGE2 LIBRARY OUT ROOMY
ls "$HERE"/backend/fail/*.sharp "$HERE"/parse/fail/*.sharp | xargs -P ${TEST_JOBS:-4} -I{} bash -c '
    f="{}"; b=$(basename "$(dirname "$(dirname "$f")")")_$(basename "$f" .sharp)
    options=$(sed -n "2s|^// options: *||p" "$f")
    timeout -s KILL 300 "$ROOMY" "$COMPILER" -c "$f" $options -L "$LIBRARY" > "$OUT/refused/$b.1.txt" 2>&1
    timeout -s KILL 300 "$ROOMY" "$STAGE2" -c "$f" $options -L "$LIBRARY" > "$OUT/refused/$b.2.txt" 2>&1'
refused=0; refused_differ=0
for one in refused/*.1.txt; do
    two=${one%.1.txt}.2.txt; refused=$((refused+1))
    if ! grep -q "error" "$one" || [ "$(steady "$one")" != "$(steady "$two")" ]; then
        refused_differ=$((refused_differ+1))
        [ $refused_differ -le 3 ] && echo "        $(basename "$one" .1.txt): $(diff <(steady "$one") <(steady "$two") | head -3 | cut -c1-160)"
    fi
done
if [ $refused_differ = 0 ] && [ $refused -gt 0 ]; then pass; else fail "refused programs" "$refused_differ of $refused programs that must be refused: stage 2's messages are not stage 1's (or stage 1 reported no error)"; fi
echo "        refused programs: $refused, the messages of stage 2 differ in $refused_differ"

# ---- D. the optimized compiler as the compiler (it exists: part A wrote and checked stage2_O)
STAGE2_O="$OUT/stage2_O/sharpc"
export STAGE2_O
if [ ! -f "$STAGE2_O" ]; then
    fail "the optimized compiler" "stage2_O/sharpc was not written: part D was not run"
else
    "$ROOMY" "$STAGE2_O" -c $LIB -wlib -wextra > library_stage2_O.txt 2>&1
    if grep -q "^analysis complete" library_stage2_O.txt && [ "$(steady library_stage1.txt)" = "$(steady library_stage2_O.txt)" ]; then pass
    else fail "library (the optimized compiler)" "it does not print what stage 1 prints for the standard library:" "$(diff <(steady library_stage1.txt) <(steady library_stage2_O.txt) | head -6)"; fi

    ls "$HERE"/backend/fail/*.sharp "$HERE"/parse/fail/*.sharp | xargs -P ${TEST_JOBS:-4} -I{} bash -c '
        f="{}"; b=$(basename "$(dirname "$(dirname "$f")")")_$(basename "$f" .sharp)
        options=$(sed -n "2s|^// options: *||p" "$f")
        timeout -s KILL 300 "$ROOMY" "$STAGE2_O" -c "$f" $options -L "$LIBRARY" > "$OUT/refused/$b.3.txt" 2>&1'
    refused_differ=0
    for one in refused/*.1.txt; do
        three=${one%.1.txt}.3.txt
        if [ "$(steady "$one")" != "$(steady "$three")" ]; then
            refused_differ=$((refused_differ+1))
            [ $refused_differ -le 3 ] && echo "        $(basename "$one" .1.txt): $(diff <(steady "$one") <(steady "$three") | head -3 | cut -c1-160)"
        fi
    done
    if [ $refused_differ = 0 ] && [ $refused -gt 0 ]; then pass; else fail "refused programs (the optimized compiler)" "$refused_differ of $refused programs that must be refused: its messages are not stage 1's"; fi
    echo "        refused programs: $refused, the messages of the optimized compiler differ in $refused_differ"
fi

# ---- C. the same results: the suite
if [ "$QUICK" = 1 ]; then
    echo "        (QUICK=1: the suite was left out)"
else
    source "$HERE/suite_program.sh"
    PROJECT="$OUT/suite_project"; mkdir -p "$PROJECT"
    rsync -a --delete --exclude /build "$SUITE/" "$PROJECT/"
    # build_with <the file to keep it as> <compiler> <option>: the suite, built from nothing in the one folder
    build_with() { suite_build "$PROJECT" "$1" "$1.txt" "$SHARP" "$2" -P "$PROJECT" -j 1 -L "$LIBRARY" $3 > /dev/null; }
    # same <first> <second>: the same executable and the same output of the compiler (else what differs)
    same() {
        [ -f "$1" ] && cmp -s "$1" "$2" && [ "$(steady "$1.txt")" = "$(steady "$2.txt")" ] && return 0
        echo "        $(cmp "$1" "$2" 2>&1 | head -1 | cut -c1-160) $(diff <(steady "$1.txt") <(steady "$2.txt") | head -2 | cut -c1-160)"
        return 1
    }
    for configuration in plain O; do
        option=""; [ $configuration = O ] && option="-O"
        mkdir -p "suite_$configuration"
        build_with "$OUT/suite_$configuration/1" "$COMPILER" "$option"
        build_with "$OUT/suite_$configuration/2" "$STAGE2" "$option"
        if same "suite_$configuration/1" "suite_$configuration/2"; then pass; echo "        the suite ($configuration): stage 1 and stage 2 build the same bytes"
        else fail "suite ($configuration)" "stage 2 does not write the bytes (or print the lines) stage 1 does$([ -f "suite_$configuration/1" ] || echo "; stage 1 wrote no program: suite_$configuration/1.txt")"; fi
        # (check 11: the optimized compiler, on the plain configuration)
        if [ $configuration = plain ] && [ -f "$STAGE2_O" ]; then
            build_with "$OUT/suite_plain/3" "$STAGE2_O" ""
            if same "suite_plain/1" "suite_plain/3"; then pass; echo "        the suite (plain): the optimized compiler builds the bytes stage 1 does"
            else fail "suite (the optimized compiler)" "it does not write the bytes (or print the lines) stage 1 does"; fi
        fi
    done
fi

echo "self-hosting: $passed passed, $failed failed   (work folder: $OUT)"
[ "$failed" -eq 0 ]
