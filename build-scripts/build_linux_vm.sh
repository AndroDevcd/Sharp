#!/bin/bash
# Builds the Sharp VM (`sharp`) on the machine it is run on: a PC, or a Raspberry Pi.
#
#     bash build-scripts/build_linux_vm.sh            -> build/linux-vm/sharp
#
#     OUT=<folder>      where the object files and `sharp` go        (default: build/linux-vm)
#     JOBS=<n>          how many files are compiled at once          (default: by cores AND memory, see below)
#     CXX=<compiler>    the C++ compiler                              (default: g++)
#     SHARP_NO_JIT=1    build without the machine-code compiler even where there is one
#     any arguments     more flags for the C++ compiler
#
# Needs: g++ with C++17 (g++ 8 or newer). On a Raspberry Pi:  sudo apt install g++
#
# WHAT DEPENDS ON THE CPU
#   The machine-code compiler (the JIT, lib/runtime/jit/x64 and the AsmJit library it uses) exists for
#   64-bit x86 only. On every other CPU - 32-bit ARM, 64-bit ARM, 32-bit x86 - those files are left out
#   and the interpreter runs everything. Nothing else in the VM is tied to a CPU.
#
# The build is incremental: a file is compiled again when it, or any header, is newer than its object.
# Change the flags and everything is compiled again.
HERE="$(cd "$(dirname "$0")/.." && pwd)"
OUT=${OUT:-$HERE/build/linux-vm}; mkdir -p "$OUT"
CXX=${CXX:-g++}
cd "$HERE" || exit 1
command -v "$CXX" > /dev/null || { echo "no C++ compiler \`$CXX\`:  sudo apt install g++"; exit 1; }

MACHINE=$("$CXX" -dumpmachine 2>/dev/null)          # what the compiler builds FOR: x86_64-linux-gnu, arm-linux-gnueabihf ...
case "$MACHINE" in
  x86_64*) JIT=1 ;;
  *)       JIT=0 ;;
esac
[ -n "$SHARP_NO_JIT" ] && JIT=0

FLAGS="-std=c++17 -O2 -w -pthread $*"
LINK="-pthread -ldl -lrt -lm"
if [ $JIT = 1 ]; then
  FLAGS="$FLAGS -DASMJIT_STATIC"
else
  FLAGS="$FLAGS -DSHARP_NO_JIT"
fi
# 64-bit numbers shared between threads need a helper library on 32-bit CPUs
case "$MACHINE" in x86_64*|aarch64*) ;; *) LINK="$LINK -latomic" ;; esac

# how many compiles at once: one per core, but not more than the memory allows (a compile takes
# up to ~400 MB; a Raspberry Pi 2 has 4 cores and 1 GB - it gets 2)
if [ -z "$JOBS" ]; then
  CORES=$(nproc 2>/dev/null || echo 2)
  MEM_MB=$(awk '/MemTotal/ {print int($2/1024)}' /proc/meminfo 2>/dev/null || echo 2048)
  BY_MEM=$(( MEM_MB / 450 )); [ $BY_MEM -lt 1 ] && BY_MEM=1
  JOBS=$CORES; [ $BY_MEM -lt $JOBS ] && JOBS=$BY_MEM
fi

echo "building the Sharp VM for $MACHINE   (machine-code compiler: $([ $JIT = 1 ] && echo yes || echo "no - interpreter only"); $JOBS at a time)"
echo "$CXX $FLAGS" > "$OUT/flags.new"
if ! cmp -s "$OUT/flags.new" "$OUT/flags.txt"; then rm -f "$OUT"/*.o; cp "$OUT/flags.new" "$OUT/flags.txt"; fi

{
  ls main.cpp lib/runtime/*.cpp lib/runtime/error/*.cpp lib/runtime/exe/*.cpp lib/runtime/memory/*.cpp lib/runtime/reflect/*.cpp \
     lib/runtime/sni/*.cpp lib/runtime/multitasking/*/*.cpp lib/runtime/types/*.cpp lib/runtime/jit/*.cpp \
     lib/runtime/telemetry/*.cpp lib/core/*.cpp lib/core/opcode/*.cpp lib/util/*.cpp lib/util/zip/*.cpp \
     lib/Modules/math/*.cpp lib/Modules/std/*.cpp lib/Modules/std.io/*.cpp
  if [ $JIT = 1 ]; then
    ls lib/runtime/jit/x64/*.cpp lib/runtime/jit/x64/opcode/*.cpp lib/runtime/jit/x64/sections/*.cpp \
       lib/util/jit/asmjit/src/asmjit/core/*.cpp lib/util/jit/asmjit/src/asmjit/arm/*.cpp lib/util/jit/asmjit/src/asmjit/x86/*.cpp
  fi
} 2>/dev/null > "$OUT/files.txt"

# an object whose source is no longer in the list (a removed file, or the JIT's after a switch) must not be linked
for o in "$OUT"/*.o; do
  [ -e "$o" ] || continue
  n=$(basename "$o" .o); grep -qxF "$n" <(tr "/ " "__" < "$OUT/files.txt") || rm -f "$o"
done

NEWEST_H=$(find lib/runtime lib/core stdimports.h -name '*.h' -printf '%T@ %p\n' | sort -n | tail -1 | cut -d' ' -f2-)
export OUT FLAGS NEWEST_H CXX
TOTAL=$(wc -l < "$OUT/files.txt")
cat "$OUT/files.txt" | xargs -P "$JOBS" -I{} bash -c 'f="{}"; o="$OUT/$(echo "$f" | tr "/ " "__").o"
  if [ ! -f "$o" ] || [ "$f" -nt "$o" ] || { [[ "$f" != lib/util/* ]] && [ "$NEWEST_H" -nt "$o" ]; }; then
    "$CXX" $FLAGS -c "$f" -o "$o" 2>"$o.err" || { echo "FAILED: $f"; grep -m8 "error" "$o.err"; rm -f "$o"; }
  fi'
BUILT=$(ls "$OUT"/*.o 2>/dev/null | wc -l)
echo "compiled: $BUILT of $TOTAL files"
[ "$BUILT" = "$TOTAL" ] || { echo "the VM was not linked: fix the files marked FAILED above (the whole message of each is in $OUT/<file>.o.err)"; exit 1; }

rm -f "$OUT/sharp"
"$CXX" -o "$OUT/sharp" "$OUT"/*.o $LINK 2>&1 | grep -v warning | head -20
[ -x "$OUT/sharp" ] || { echo "the VM did not link"; exit 1; }
echo "built: $OUT/sharp   ($("$OUT/sharp" -V 2>&1 | head -1))"
