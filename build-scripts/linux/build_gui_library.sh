#!/bin/bash
# Builds the GUI's native library for Linux: sharpgui.so - the one file that goes beside the
# `sharp` program so that `import ( std.guikit.* )` works.   (Linux / WSL)
#
#     bash build-scripts/linux/build_gui_library.sh
#
#     -> $WORK/out/gui_lib/sharpgui.so         (WORK is ~/sharp_work)
#     -> build/gui/glue/                       the bridge, in the repository's build folder -
#                                              windows/build_gui_library.sh builds sharpgui.dll from it
#
# Needs:  sudo apt install g++ libsdl2-dev     and build.sh run first (the bridge is written by the compiler).
#
# TWO STEPS
#   1. THE BRIDGE. internal/every_native.sharp names every native function of the GUI. Compiling it with
#      -sni makes the compiler write the C++ that connects Sharp to those functions
#      (sharpgui_sni.cpp / .h). It is plain C++, the same for every system.
#   2. THE LIBRARY. The bridge and the C++ of std/guikit/platform/native, compiled into one file.
#
#     GLUE=<folder>   a bridge that already exists (sharpgui_sni.cpp in it): skip step 1. For a machine
#                     with no Sharp compiler yet - copy build/gui/glue to it.
#     OUT=<folder>    where sharpgui.so goes
#     CXX=<compiler>  the C++ compiler (default g++)
#
# A Raspberry Pi: run this ON the Pi after linux/install_from_source.sh, with
#     SHARP_COMPILER="sharpc" OUT=build/gui bash build-scripts/linux/build_gui_library.sh
# and copy build/gui/sharpgui.so to ~/.sharp/bin/.
#     SHARP_COMPILER=<command>   how to run the compiler for step 1 (default: the one build.sh built)
source "$(dirname "$0")/../internal/common.sh"
OUT=${OUT:-$WORK/out/gui_lib}; mkdir -p "$OUT"
NATIVE="$LIBRARY_SOURCE/std/guikit/platform/native"
CXX=${CXX:-g++}
[ -d "$NATIVE" ] || fail "no GUI sources at $NATIVE"
command -v sdl2-config > /dev/null || fail "SDL2's development files are missing:  sudo apt install libsdl2-dev"

# ---- 1. the bridge
if [ -n "$GLUE" ]; then
    [ -f "$GLUE/sharpgui_sni.cpp" ] || fail "no sharpgui_sni.cpp in $GLUE"
else
    GLUE="$REPO/build/gui/glue"; rm -rf "$GLUE"; mkdir -p "$REPO/build/gui"
    if [ -z "$SHARP_COMPILER" ]; then
        [ -x "$VM" ] && [ -f "$COMPILER" ] || fail "run  bash build-scripts/build.sh  first (the compiler writes the bridge)"
        SHARP_COMPILER="$VM $COMPILER"
    fi
    $SHARP_COMPILER "$REPO/build-scripts/internal/every_native.sharp" -j 1 -L "$LIBRARY_SOURCE" -sni "$GLUE" -o "$OUT/every_native" > "$OUT/bridge_compile.txt" 2>&1
    [ -f "$GLUE/sharpgui_sni.cpp" ] || { echo "the bridge was not written:"; grep -v "^$" "$OUT/bridge_compile.txt" | head -20; exit 1; }
fi
echo "bridge: $(grep -c 'sharpgui_' "$GLUE/sharpgui_sni.h" 2>/dev/null) native functions in $GLUE"

# ---- 2. the library
TARGET="$OUT/sharpgui.so"; rm -f "$TARGET"
"$CXX" -shared -fPIC -std=c++17 -O2 -w -I "$GLUE" -I "$NATIVE" $(sdl2-config --cflags) \
    "$GLUE/sharpgui_sni.cpp" "$NATIVE"/*.cpp -o "$TARGET" $(sdl2-config --libs) 2> "$OUT/native_build.txt"
if [ ! -f "$TARGET" ]; then echo "the library did not build:"; grep -m 20 "error" "$OUT/native_build.txt"; exit 1; fi
echo "built: $TARGET   ($(stat -c %s "$TARGET") bytes)"
echo "it goes beside the sharp program (the installers put it in bin/)"
