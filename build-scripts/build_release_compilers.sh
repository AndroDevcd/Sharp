#!/bin/bash
# Builds the two compilers a release ships, by letting the compiler compile itself (WSL / Linux):
#
#     <OUT>/plain/sharpc     the compiler compiled by itself WITHOUT the optimizer   (sharpc-unoptimized-v<version>)
#     <OUT>/O/sharpc         the compiler compiled by itself WITH -O                 (sharpc-v<version>)
#     <OUT>/debug/sharpc     the DEBUG compiler, with -O: prints how the compile went and has the options
#                            that look inside the compiler                           (sharpc-debug-v<version>)
#
# and then checks, in a few minutes, that the two behave the same: every program of tests/backend/fail
# gets the same messages from both, and a handful of suite cases are written as the same bytes by both,
# plain and with -O. (The full proof is the gates' self-hosting step; this is the quick form.)
#
#     STAGE1   the compiler to start from   (default ~/sharp_work/out/boot/sharpc_boot, built by bootc.sh)
#     SOURCES  the bootstrap's sources      (default ~/sharp_work/src/lib/compiler)
#     LIBRARY  the standard library         (default ~/sharp_work/stdlib)
#     SHARP    the VM                       (default ~/sharp_work/sharp_dev/sharp)
#     OUT      (default ~/sharp_work/out/release_build)
W=$HOME/sharp_work
STAGE1=${STAGE1:-$W/out/boot/sharpc_boot}
SOURCES=${SOURCES:-$W/src/lib/compiler}
LIBRARY=${LIBRARY:-$W/stdlib}
SHARP=${SHARP:-$W/sharp_dev/sharp}
OUT=${OUT:-$W/out/release_build}
HERE=$(cd "$(dirname "$0")/.." && pwd)
rm -rf "$OUT"; mkdir -p "$OUT/plain" "$OUT/O" "$OUT/debug" "$OUT/s1" "$OUT/s2"
# THE STANDARD COMPILER is the sources with one line changed: `debug_compiler` false
# (lib/compiler/configuration/build_kind.sharp says what that leaves out). The change is made in a copy.
STANDARD="$OUT/standard_sources"
mkdir -p "$STANDARD"; rsync -a --delete --exclude tests "$SOURCES/" "$STANDARD/"
sed -i 's/^debug_compiler : decimal = true;/debug_compiler : decimal = false;/' "$STANDARD/configuration/build_kind.sharp"
grep -q '^debug_compiler : decimal = false;' "$STANDARD/configuration/build_kind.sharp" || { echo "build_kind.sharp: the debug_compiler line was not found"; exit 1; }
DEBUG_SRC=$(find "$SOURCES" -name "*.sharp" -not -path "*/tests/*" | sort)
SRC=$(find "$STANDARD" -name "*.sharp" | sort)
LIB=$(find "$LIBRARY" -name "*.sharp" | sort)
VM="$SHARP -mem 4gb"
# every stage writes a file of the same name from inside its own folder: the name given to -o is written into the file
(cd "$OUT/plain" && timeout -s KILL 1500 $VM $STAGE1 $SRC -o sharpc -L "$LIBRARY" -j 1 -timings > compile.txt 2>&1) &
(cd "$OUT/O" && timeout -s KILL 1500 $VM $STAGE1 $SRC -o sharpc -L "$LIBRARY" -j 1 -O -timings > compile.txt 2>&1) &
wait
# the debug compiler (sharpc-debug-v<version>): the sources as they are, with -O. After the other two:
# three of these at once need more memory than WSL has.
(cd "$OUT/debug" && timeout -s KILL 1500 $VM $STAGE1 $DEBUG_SRC -o sharpc -L "$LIBRARY" -j 1 -O -timings > compile.txt 2>&1)
for k in plain O debug; do
  echo "$k: $(grep -c '^error' $OUT/$k/compile.txt) error(s), $(stat -c %s $OUT/$k/sharpc 2>/dev/null || echo NOT WRITTEN) bytes"
  [ -f $OUT/$k/sharpc ] || { grep -a "^error\|internal" -A5 $OUT/$k/compile.txt | head -12; exit 1; }
done
steady() { grep -av "^Executed in\|^vm exiting\|^time:\|^$"; }
bad=0; n=0
for f in "$HERE"/lib/compiler/tests/backend/fail/*.sharp; do
  n=$((n+1))
  a=$($VM $OUT/plain/sharpc -c $LIB "$f" 2>&1 | steady | md5sum); b=$($VM $OUT/O/sharpc -c $LIB "$f" 2>&1 | steady | md5sum)
  [ "$a" = "$b" ] || { bad=$((bad+1)); echo "DIFFERENT messages: $(basename $f)"; }
done
echo "refused programs: $n compared, $bad different"
total_bad=$bad; bad=0; n=0
for c in lib_data_json lambdas_closures nullability lib_collections_functional lib_coroutines_core operators_unary_minus lib_math_linear; do
  f=$W/suite/cases/$c.sharp; [ -f $f ] || continue
  for flag in "" "-O"; do
    n=$((n+1))
    (cd $OUT/s1 && $VM $OUT/plain/sharpc $LIB $W/suite/testing.sharp $f -j 1 $flag -o case > out.txt 2>&1)
    (cd $OUT/s2 && $VM $OUT/O/sharpc $LIB $W/suite/testing.sharp $f -j 1 $flag -o case > out.txt 2>&1)
    if [ -f $OUT/s1/case ] && cmp -s $OUT/s1/case $OUT/s2/case && $SHARP $OUT/s1/case 2>&1 | grep -q "RESULT PASS"; then :
    else bad=$((bad+1)); echo "DIFFERENT or failing: $c $flag ($(tail -1 $OUT/s2/out.txt | cut -c1-120))"; fi
    rm -f $OUT/s1/case $OUT/s2/case
  done
done
echo "suite builds: $n compared (same bytes from both compilers, and the program passes), $bad different"
# THE STANDARD COMPILERS ARE QUIET, THE DEBUG ONE TALKS (lib/compiler/configuration/build_kind.sharp):
# a good compile prints nothing from the standard ones, and they refuse the options that look inside
# the compiler; the debug compiler prints how the compile went and takes them.
kind_bad=0
printf 'mod main;
def main(args: string[]) { println("hi"); }
' > $OUT/s1/quiet.sharp
for k in plain O; do
  said=$(cd $OUT/s1 && $VM $OUT/$k/sharpc $LIB quiet.sharp -j 1 -o quiet 2>&1 | steady)
  [ -z "$said" ] && [ -f $OUT/s1/quiet ] || { kind_bad=$((kind_bad+1)); echo "the standard compiler ($k) is not quiet on a good compile: $(echo "$said" | head -3)"; }
  rm -f $OUT/s1/quiet
  (cd $OUT/s1 && $VM $OUT/$k/sharpc $LIB quiet.sharp -j 1 -Oreport -o quiet 2>&1) | grep -q "only in the debug compiler" || { kind_bad=$((kind_bad+1)); echo "the standard compiler ($k) did not refuse -Oreport"; }
done
(cd $OUT/s1 && $VM $OUT/debug/sharpc $LIB quiet.sharp -j 1 -O -Oreport -o quiet 2>&1) > $OUT/s1/debug.txt
grep -q "analysis complete: no errors" $OUT/s1/debug.txt && grep -q "^code generation: " $OUT/s1/debug.txt || { kind_bad=$((kind_bad+1)); echo "the debug compiler did not print how the compile went: $(head -3 $OUT/s1/debug.txt)"; }
echo "standard / debug: $kind_bad problem(s)"
bad=$((bad + kind_bad))
[ $((total_bad + bad)) = 0 ] && echo "release compilers: OK" || { echo "release compilers: NOT OK"; exit 1; }
