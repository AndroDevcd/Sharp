#!/bin/bash
# Builds the Sharp VM (sharp.exe) for Windows x64, natively, with a MinGW g++.
#
#   bash build-scripts/windows/build_vm.sh            from Git Bash
#
#   MINGW   the folder with g++.exe      (default: the MinGW that ships with CLion, see mingw.sh)
#   OUT     where the objects and sharp.exe go   (default: cmake-build-windows-vm, which git ignores)
#   JOBS    g++ processes at a time      (default 6)
#
# The same sources and flags as the Linux build (linux/build_vm.sh), compiled
# file by file so that a second run only compiles what changed. The .exe is linked statically: it needs
# no MinGW DLL beside it, so it can be copied to a machine that has no compiler.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${OUT:-$ROOT/cmake-build-windows-vm}
JOBS=${JOBS:-6}
source "$ROOT/build-scripts/windows/mingw.sh"
mkdir -p "$OUT"
cd "$ROOT" || exit 1
# __USE_MINGW_ANSI_STDIO: MinGW's own printf family instead of the old Windows C library's, which does not
# format every decimal the way Linux does (very large and very small numbers lose their last digits there:
# the library's json numbers then do not read back as what was written)
FLAGS="-std=c++17 -O3 -w -pthread -DASMJIT_STATIC -D__USE_MINGW_ANSI_STDIO=1 $CRT_FLAG $*"
echo "$FLAGS" > "$OUT/flags.new"
if ! cmp -s "$OUT/flags.new" "$OUT/flags.txt"; then rm -f "$OUT"/*.o; cp "$OUT/flags.new" "$OUT/flags.txt"; fi
ls main.cpp lib/runtime/error/*.cpp lib/runtime/interrupts/*.cpp lib/runtime/exe/*.cpp lib/runtime/memory/*.cpp lib/runtime/reflect/*.cpp \
   lib/runtime/sni/*.cpp lib/runtime/cffi/*.cpp lib/runtime/multitasking/*/*.cpp lib/runtime/types/*.cpp lib/runtime/jit/*.cpp \
   lib/runtime/jit/x64/*.cpp lib/runtime/jit/x64/opcode/*.cpp lib/runtime/jit/x64/sections/*.cpp \
   lib/runtime/*.cpp lib/runtime/telemetry/*.cpp lib/core/*.cpp lib/core/opcode/*.cpp lib/util/*.cpp lib/util/zip/*.cpp \
   lib/Modules/math/*.cpp lib/Modules/std/*.cpp lib/Modules/std.io/*.cpp \
   lib/util/jit/asmjit/src/asmjit/core/*.cpp lib/util/jit/asmjit/src/asmjit/arm/*.cpp lib/util/jit/asmjit/src/asmjit/x86/*.cpp > "$OUT/files.txt" 2>/dev/null
# a header changed -> everything older than the newest header is compiled again
NEWEST_H=$(find lib/runtime lib/core lib/Modules lib/util stdimports.h -name '*.h' -not -path 'lib/util/jit/*' -printf '%T@ %p\n' | sort -n | tail -1 | cut -d' ' -f2-)
export OUT FLAGS NEWEST_H
cat "$OUT/files.txt" | xargs -P $JOBS -I{} bash -c 'f="{}"; o="$OUT/$(echo "$f" | tr "/ " "__").o"; if [ ! -f "$o" ] || [ "$f" -nt "$o" ] || { [[ "$f" != lib/util/jit/* ]] && [ "$NEWEST_H" -nt "$o" ]; }; then g++ $FLAGS -c "$f" -o "$o" 2>"$o.err" || { echo "FAILED: $f"; grep -m4 "error" "$o.err"; rm -f "$o"; }; fi'
# an object whose source file is no longer in the list (a file that was removed or renamed) must not be linked
for o in "$OUT"/*.o; do n=$(basename "$o" .o); grep -qxF "$n" <(tr "/ " "__" < "$OUT/files.txt") || rm -f "$o"; done
echo "objects: $(ls "$OUT"/*.o 2>/dev/null | wc -l) of $(wc -l < "$OUT/files.txt")"
rm -f "$OUT/sharp.exe"
g++ -static $CRT_FLAG -o "$OUT/sharp.exe" "$OUT"/*.o -pthread -lm -lpsapi 2>&1 | grep -v warning | head -20
[ -x "$OUT/sharp.exe" ] || { echo "sharp.exe was not linked: a file above failed to compile (each one's messages: $OUT/<file>.o.err)"; exit 1; }
echo "built: $OUT/sharp.exe   ($("$OUT/sharp.exe" -V 2>&1 | head -1))"
