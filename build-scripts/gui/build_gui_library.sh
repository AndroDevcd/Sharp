#!/bin/bash
# Builds the GUI's native library - sharpgui.so on Linux, sharpgui.dll on Windows - the one file
# that goes beside the `sharp` program so that `import ( std.guikit.* )` works.
#
#   bash build-scripts/gui/build_gui_library.sh
#
#   SHARPC   how to run the Sharp compiler (default: sharpc)            e.g. "sharp /path/sharpc_boot"
#   LIB      the standard library folder to compile against (default: lib/support/<newest> of this repo)
#   OUT      where the bridge and the library go (default: build/gui)
#   GLUE     a folder that already holds sharpgui_sni.cpp / .h / sni.h: skip the compile step.
#            (The bridge is plain C++, the same for every system: make it once, anywhere a Sharp
#             compiler runs, and build the library from it on each system.)
#   CXX      the C++ compiler (default: g++)
#   SDL2     Windows only: the folder of SDL2's MinGW development files, the one with include/ and
#            lib/ in it (x86_64-w64-mingw32 of SDL2-devel-<version>-mingw). Default: build/deps/SDL2-*/x86_64-w64-mingw32
#
# TWO STEPS
#   1. the bridge: every_native.sharp names every native function of the GUI; compiling it with -sni
#      makes the compiler write the C++ bridge for ALL of them (see that file for why).
#   2. the library: the bridge and the C++ of std/guikit/platform/native, compiled into one shared library.
#
# WHAT IT NEEDS
#   Linux     g++, SDL2's development files:   sudo apt install g++ libsdl2-dev
#   Windows   a MinGW g++ (Git Bash + the one that ships with CLion works), and SDL2's MinGW
#             development files unpacked somewhere (SDL2=..., or build/deps/). SDL2 is linked INTO
#             sharpgui.dll, so no SDL2.dll has to be shipped beside it.
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
OUT=${OUT:-$ROOT/build/gui}; mkdir -p "$OUT"
LIB=${LIB:-$(ls -d "$ROOT"/lib/support/*/ | sort -V | tail -1)}
NATIVE="${LIB%/}/std/guikit/platform/native"
CXX=${CXX:-g++}
[ -d "$NATIVE" ] || { echo "no GUI sources at $NATIVE (set LIB=<standard library folder>)"; exit 1; }

# ---- 1. the bridge
if [ -n "$GLUE" ]; then
    [ -f "$GLUE/sharpgui_sni.cpp" ] || { echo "no sharpgui_sni.cpp in $GLUE"; exit 1; }
else
    GLUE="$OUT/glue"; rm -rf "$GLUE"
    ${SHARPC:-sharpc} "$HERE/every_native.sharp" -j 1 -L "${LIB%/}" -sni "$GLUE" -o "$OUT/every_native" > "$OUT/bridge_compile.txt" 2>&1
    [ -f "$GLUE/sharpgui_sni.cpp" ] || { echo "the bridge was not written:"; grep -v "^$" "$OUT/bridge_compile.txt" | head -20; exit 1; }
fi
echo "bridge: $(grep -c 'sharpgui_' "$GLUE/sharpgui_sni.h" 2>/dev/null) native functions in $GLUE"

# ---- 2. the library
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    SDL2=${SDL2:-$(ls -d "$ROOT"/build/deps/SDL2-*/x86_64-w64-mingw32 2>/dev/null | sort -V | tail -1)}
    [ -f "$SDL2/lib/libSDL2.a" ] || { echo "SDL2's MinGW development files were not found (set SDL2=<.../x86_64-w64-mingw32>)"; exit 1; }
    TARGET="$OUT/sharpgui.dll"
    # -static: SDL2 and the C++ runtime go INTO the dll; what follows -lSDL2 is what SDL2 itself needs from Windows
    "$CXX" -shared -std=c++17 -O2 -w -I "$GLUE" -I "$NATIVE" -I "$SDL2/include" -I "$SDL2/include/SDL2" \
        "$GLUE/sharpgui_sni.cpp" "$NATIVE"/*.cpp -o "$TARGET" \
        -static -L "$SDL2/lib" -lSDL2 \
        -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid \
        2> "$OUT/native_build.txt"
    ;;
  *)
    command -v sdl2-config > /dev/null || { echo "SDL2's development files are missing:  sudo apt install libsdl2-dev"; exit 1; }
    TARGET="$OUT/sharpgui.so"
    "$CXX" -shared -fPIC -std=c++17 -O2 -w -I "$GLUE" -I "$NATIVE" $(sdl2-config --cflags) \
        "$GLUE/sharpgui_sni.cpp" "$NATIVE"/*.cpp -o "$TARGET" $(sdl2-config --libs) 2> "$OUT/native_build.txt"
    ;;
esac
if [ ! -f "$TARGET" ] || grep -q "error" "$OUT/native_build.txt"; then
    echo "the library did not build:"; grep -m 20 "error" "$OUT/native_build.txt"; exit 1
fi
echo "built: $TARGET   ($(ls -la "$TARGET" | awk '{print $5}') bytes)"
echo "put it beside the sharp program (the same folder as sharp / sharp.exe)"
