#!/bin/bash
# THE SHARP NATIVE INTERFACE, END TO END (lib/runtime/sni, backend/finalizer/sni)
#
# For each program in tests/sni/: compile it with `-sni <folder>`, build its C++ into the
# libraries with g++, run it with the JIT and with -nojit, and compare what it prints with
# expected.txt. Then the things that must be REFUSED, each with a message that says what to do:
# a library built against another declaration, a library file that is not there, a native call
# before any library was loaded. (A program loads its libraries itself: load_library("path").)
#
#   tests/sni.sh
#       SHARP=<the runtime>  COMPILER=<the compiled compiler>  LIBRARY=<the standard library folder>
#       OUT=<a folder to work in>   CXX=<the C++ compiler, default g++>
#
# Works on Linux and, from Git Bash with a MinGW g++ on the PATH, on Windows (the libraries are
# then .dll files). Prints one line per failure and a summary; exits 1 when anything failed.
HERE="$(cd "$(dirname "$0")" && pwd)"
SHARP=${SHARP:-${SHARP_WORK:-$HOME/sharp_work}/sharp_dev/sharp}
COMPILER=${COMPILER:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sharpc_boot}
LIBRARY=${LIBRARY:-${SHARP_WORK:-$HOME/sharp_work}/stdlib}
OUT=${OUT:-${SHARP_WORK:-$HOME/sharp_work}/out/boot/sni}
CXX=${CXX:-g++}
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) lib_file() { echo "$1.dll"; }; PIC="-static" ;;
    *)                    lib_file() { echo "$1.so"; }; PIC="-fPIC" ;;
esac
rm -rf "$OUT"; mkdir -p "$OUT"

passed=0
failed=0
pass() { passed=$((passed+1)); }
fail() { failed=$((failed+1)); echo "FAIL  $1"; shift; for line in "$@"; do echo "        $line"; done; }

# build_library <work folder> <name> <sources...>: the generated glue + the test's own C++ -> one library
build_library() {
    local work="$1" name="$2"; shift 2
    "$CXX" -shared $PIC -std=c++11 -O1 -I "$work/native" "$work/native/${name}_sni.cpp" "$@" -o "$work/$(lib_file "$name")" 2> "$work/$name.build.txt"
}

# ---- 1. the basic program: every kind of value, both ways
W="$OUT/basic"; mkdir -p "$W"
( cd "$W" && "$SHARP" "$COMPILER" "$HERE/sni/basic/main.sharp" -j 1 -L "$LIBRARY" -sni native -o basic > compile.txt 2>&1 )
if [ -f "$W/basic" ] && [ -f "$W/native/sni.h" ] && [ -f "$W/native/demo_sni.h" ] && [ -f "$W/native/app_sni.cpp" ]; then pass
else fail "basic: compile" "$(head -12 "$W/compile.txt")"; fi
build_library "$W" demo "$HERE/sni/basic/native/demo.cpp" && build_library "$W" app "$HERE/sni/basic/native/app.cpp"
if [ -f "$W/$(lib_file demo)" ] && [ -f "$W/$(lib_file app)" ]; then pass
else fail "basic: the C++ did not build" "$(cat "$W"/*.build.txt | head -12)"; fi
for mode in "" "-nojit"; do
    ( cd "$W" && "$SHARP" $mode basic > "run$mode.txt" 2>&1; echo "exit $?" >> "run$mode.txt" )
    if diff <(tr -d '\r' < "$W/run$mode.txt") <(cat "$HERE/sni/basic/expected.txt"; echo "exit 0") > "$W/diff$mode.txt"; then pass
    else fail "basic: output ${mode:-jit}" "$(head -12 "$W/diff$mode.txt")"; fi
done

# ---- 2. a library built against ANOTHER declaration is refused, with both signatures
W2="$OUT/mismatch"; mkdir -p "$W2/native"
cat > "$W2/main.sharp" <<'EOF'
mod app;
import ( platform )
native "demo" { def add(a: _int32, b: decimal) : _int32; }
def main(args: string[]) { load_library("demo"); println("add " + add(1, 2)); }
EOF
( cd "$W2" && "$SHARP" "$COMPILER" main.sharp -j 1 -L "$LIBRARY" -o mismatch > compile.txt 2>&1 )
cp "$W/$(lib_file demo)" "$W2/" 2>/dev/null
( cd "$W2" && "$SHARP" mismatch > run.txt 2>&1 )
if grep -q "built for another version" "$W2/run.txt" && grep -q "(_int32,decimal)_int32" "$W2/run.txt" && grep -q "(_int32,_int32)_int32" "$W2/run.txt"; then pass
else fail "mismatch: no message with both signatures" "$(head -8 "$W2/run.txt")"; fi

# ---- 3. a library that is not there: the message says where it was looked for
W3="$OUT/missing"; mkdir -p "$W3"
cp "$W2/main.sharp" "$W3/"
( cd "$W3" && "$SHARP" "$COMPILER" main.sharp -j 1 -L "$LIBRARY" -o missing > compile.txt 2>&1; "$SHARP" missing > run.txt 2>&1 )
if grep -q "could not load the native library .demo" "$W3/run.txt" && grep -q "Looked for" "$W3/run.txt" && grep -q "$(lib_file demo)" "$W3/run.txt"; then pass
else fail "missing: no message that says where the library was looked for" "$(head -8 "$W3/run.txt")"; fi

# ---- 3b. a native call with no library loaded: the message says to call load_library
W5="$OUT/unloaded"; mkdir -p "$W5"
cat > "$W5/main.sharp" <<'EOF'
mod app;
native "demo" { def add(a: _int32, b: _int32) : _int32; }
def main(args: string[]) { println("add " + add(1, 2)); }
EOF
cp "$W/$(lib_file demo)" "$W5/" 2>/dev/null       # the file is right there, and still nothing loads it
( cd "$W5" && "$SHARP" "$COMPILER" main.sharp -j 1 -L "$LIBRARY" -o unloaded > compile.txt 2>&1; "$SHARP" unloaded > run.txt 2>&1 )
if grep -q "no native library is loaded" "$W5/run.txt" && grep -q "load_library(" "$W5/run.txt"; then pass
else fail "unloaded: a native call without load_library did not say to load the library" "$(head -8 "$W5/run.txt")"; fi

# ---- 3c. the path given to load_library: relative (from the folder `sharp` is run in) and absolute
W6="$OUT/paths"; mkdir -p "$W6/libs"
cat > "$W6/main.sharp" <<'EOF'
mod app;
import ( platform )
native "demo" { def add(a: _int32, b: _int32) : _int32; }
def main(args: string[]) { load_library(args[0]); println("add " + add(40, 2)); }
EOF
cp "$W/$(lib_file demo)" "$W6/libs/" 2>/dev/null
( cd "$W6" && "$SHARP" "$COMPILER" main.sharp -j 1 -L "$LIBRARY" -o paths > compile.txt 2>&1
  "$SHARP" paths libs/demo > relative.txt 2>&1
  "$SHARP" paths "$(pwd -W 2>/dev/null || pwd)/libs/demo" > absolute.txt 2>&1
  "$SHARP" paths demo > bare.txt 2>&1 )
if grep -q "^add 42" "$W6/relative.txt"; then pass; else fail "paths: a relative path" "$(head -6 "$W6/relative.txt")"; fi
if grep -q "^add 42" "$W6/absolute.txt"; then pass; else fail "paths: an absolute path" "$(head -6 "$W6/absolute.txt")"; fi
# (the library is in libs/, not here: a bare name must NOT find it - nothing is searched)
if grep -q "could not load the native library" "$W6/bare.txt"; then pass; else fail "paths: a bare name found a library in another folder" "$(head -6 "$W6/bare.txt")"; fi

# (a file that IS there and is not a library of this system: the message says so, not "no such file")
( cd "$W6" && echo "not a library" > "$(lib_file fake)" && "$SHARP" paths fake > fake.txt 2>&1 )
if grep -q "is there, but it could not be loaded" "$W6/fake.txt"; then pass; else fail "paths: a file that is not a library" "$(head -8 "$W6/fake.txt")"; fi

# ---- 3d. a PROJECT needs no -sni: the C++ files go to build/outputs/sni when the program has native
#          functions, and a project without any gets no such folder
W7="$OUT/project"; mkdir -p "$W7/withnative/src" "$W7/plain/src"
for k in withnative plain; do printf '{\n  "name": "%s",\n  "version": "1.0",\n  "output": "%s"\n}\n' $k $k > "$W7/$k/settings.json"; done
cp "$W6/main.sharp" "$W7/withnative/src/main.sharp"
printf 'mod app;\ndef main(args: string[]) { println("plain"); }\n' > "$W7/plain/src/main.sharp"
( cd "$W7" && "$SHARP" "$COMPILER" -P withnative -L "$LIBRARY" > withnative.txt 2>&1; "$SHARP" "$COMPILER" -P plain -L "$LIBRARY" > plain.txt 2>&1 )
if [ -f "$W7/withnative/build/outputs/withnative" ] && [ -f "$W7/withnative/build/outputs/sni/sni.h" ] && [ -f "$W7/withnative/build/outputs/sni/demo_sni.h" ] \
   && [ -f "$W7/withnative/build/outputs/sni/demo_sni.cpp" ] && grep -q "native interface: 1 group" "$W7/withnative.txt" \
   && grep -q '"native": { "sources"' "$W7/withnative.txt"; then pass
else fail "project: the C++ files are not in build/outputs/sni" "$(tail -4 "$W7/withnative.txt")" "$(ls "$W7/withnative/build/outputs" 2>&1)"; fi
if [ -f "$W7/plain/build/outputs/plain" ] && [ ! -e "$W7/plain/build/outputs/sni" ]; then pass
else fail "project: a project without native functions got an sni folder (or was not built)" "$(tail -4 "$W7/plain.txt")"; fi

# ---- 3e. A PROJECT BUILDS ITS C++ ITSELF: "native": { "sources": [...] } in settings.json. One command
#          makes the executable, the glue and the library; the program loads the library from its own
#          folder (program_folder()), so it runs from anywhere; a second build compiles no C++ again;
#          a C++ compiler that is not there is an error that says where it was named; a "processor"
#          block is used for the processor the C++ compiler builds for and for no other
W8="$OUT/autobuild/app"; mkdir -p "$W8/src/cpp/deep"
cat > "$W8/settings.json" <<'EOF'
{
  "name": "app",
  "output": "app",
  "native": { "sources": [ "src/cpp" ], "toolchain": { "flags": [ "-O1" ], "processor": PROCESSORS } }
}
EOF
# "processor" blocks: the one for THIS machine's processor must be used (the C++ refuses to compile without
# its flag), and the ones for other processors must not (they name a library that does not exist)
case "$("$CXX" -dumpmachine)" in
    aarch64*|arm*) mine=arm; other=x86 ;;
    *)             mine=x86; other=arm ;;
esac
sed -i "s/PROCESSORS/{ \"$mine\": { \"flags\": [ \"-DFOR_THIS_PROCESSOR\" ], \"sources\": [ \"native\/$mine\" ] }, \"$other\": { \"libraries\": [ \"a_library_that_is_not_there\" ], \"sources\": [ \"native\/$other\" ] } }/" "$W8/settings.json"
# C++ written once per processor, the same file names in both folders: only this machine's is compiled
# (the other one would stop the build), and its header is found without an "include"
mkdir -p "$W8/native/$mine" "$W8/native/$other"
echo "int processor_bonus();" > "$W8/native/$mine/bonus.h"
echo "int processor_bonus() { return 0; }" > "$W8/native/$mine/bonus.cpp"
echo "#error the C++ of the OTHER processor was compiled" > "$W8/native/$other/bonus.cpp"
cat > "$W8/src/main.sharp" <<'EOF'
mod app;
import ( platform )
native "demo" { def add(a: _int32, b: _int32) : _int32; }
def main(args: string[]) { load_library(program_folder() + "/demo"); println("add " + add(40, 2)); }
EOF
# (a header in the "sources" folder, used from a file in a folder BELOW it: found because every
#  sources folder is searched for headers - no "include" names it)
echo "#define THE_ANSWER_OFFSET 0" > "$W8/src/cpp/shared.h"
cat > "$W8/src/cpp/deep/demo.cpp" <<'EOF'
#include "demo_sni.h"
#include "shared.h"
#include "bonus.h"
#ifndef FOR_THIS_PROCESSOR
#error the "processor" block of settings.json for this machine was not used
#endif
int32_t demo_add(sni_env* env, int32_t a, int32_t b) { return a + b + THE_ANSWER_OFFSET + processor_bonus(); }
EOF
( cd "$OUT/autobuild" && "$SHARP" "$COMPILER" -P app -L "$LIBRARY" > first.txt 2>&1; echo "exit $?" >> first.txt
  "$SHARP" "$COMPILER" -P app -L "$LIBRARY" > second.txt 2>&1 )
if [ -f "$W8/build/outputs/$(lib_file demo)" ] && grep -q "^C++ compiler: " "$OUT/autobuild/first.txt" && grep -q "native library: .* built from 2 C++ file" "$OUT/autobuild/first.txt" \
   && grep -q "exit 0" "$OUT/autobuild/first.txt"; then pass
else fail "autobuild: the project did not build its C++" "$(tail -8 "$OUT/autobuild/first.txt")"; fi
if grep -q "is up to date" "$OUT/autobuild/second.txt"; then pass; else fail "autobuild: the second build compiled the C++ again" "$(tail -5 "$OUT/autobuild/second.txt")"; fi
# (run from ANOTHER folder: the library is found through program_folder(), not through where we are)
( cd "$OUT" && "$SHARP" "autobuild/app/build/outputs/app" > "$OUT/autobuild/run.txt" 2>&1 )
if grep -q "^add 42" "$OUT/autobuild/run.txt"; then pass; else fail "autobuild: the program did not load its library from its own folder" "$(head -8 "$OUT/autobuild/run.txt")"; fi
# a changed C++ file is built again
sleep 1; echo "// changed" >> "$W8/src/cpp/deep/demo.cpp"
( cd "$OUT/autobuild" && "$SHARP" "$COMPILER" -P app -L "$LIBRARY" > third.txt 2>&1 )
if grep -q "built from 2 C++ file" "$OUT/autobuild/third.txt"; then pass; else fail "autobuild: a changed C++ file was not built again" "$(tail -5 "$OUT/autobuild/third.txt")"; fi
# a C++ mistake: the C++ compiler's message is shown, and the build fails
echo "this is not C++" >> "$W8/src/cpp/deep/demo.cpp"
( cd "$OUT/autobuild" && "$SHARP" "$COMPILER" -P app -L "$LIBRARY" > broken.txt 2>&1; echo "exit $?" >> broken.txt )
if grep -q "demo.cpp" "$OUT/autobuild/broken.txt" && grep -q "did not build" "$OUT/autobuild/broken.txt" && ! grep -q "exit 0" "$OUT/autobuild/broken.txt"; then pass
else fail "autobuild: a C++ error did not fail the build with the C++ compiler's message" "$(tail -8 "$OUT/autobuild/broken.txt")"; fi
# -cxx names a compiler that is not there
( cd "$OUT/autobuild" && "$SHARP" "$COMPILER" -P app -L "$LIBRARY" -cxx /nowhere/g++ > nocxx.txt 2>&1; echo "exit $?" >> nocxx.txt )
if grep -q "is not there" "$OUT/autobuild/nocxx.txt" && grep -q "the command line (-cxx)" "$OUT/autobuild/nocxx.txt" && ! grep -q "exit 0" "$OUT/autobuild/nocxx.txt"; then pass
else fail "autobuild: a missing -cxx compiler" "$(tail -6 "$OUT/autobuild/nocxx.txt")"; fi

# ---- 4. stopping a thread that is inside C++: the polite loop stops at once; the stubborn one is
#         waited for 2 seconds, left behind with a warning, and the program still ends
W4="$OUT/stop"; mkdir -p "$W4"
( cd "$W4" && "$SHARP" "$COMPILER" "$HERE/sni/stop/main.sharp" -j 1 -L "$LIBRARY" -sni native -o stop > compile.txt 2>&1 )
build_library "$W4" stop "$HERE/sni/stop/native/stop.cpp"
begun=$(date +%s)
( cd "$W4" && timeout 60 "$SHARP" stop > run.txt 2>&1; echo "exit $?" >> run.txt )
took=$(( $(date +%s) - begun ))
if grep -q "polite: stopped" "$W4/run.txt" && grep -q "still inside the native function .app#__srt_global.spin_stubbornly" "$W4/run.txt"    && grep -q "main ends" "$W4/run.txt" && grep -q "exit 0" "$W4/run.txt" && [ $took -ge 2 ] && [ $took -le 50 ]; then pass
else fail "stop: polite / stubborn / the program ends (took ${took}s)" "$(head -12 "$W4/run.txt")" "$(head -5 "$W4/compile.txt" "$W4/stop.build.txt" 2>/dev/null)"; fi

echo "sni: $passed passed, $failed failed"
[ $failed -eq 0 ]
