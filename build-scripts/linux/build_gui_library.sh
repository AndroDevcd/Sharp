#!/bin/bash
# Builds the GUI's native library for Linux: sharpgui.so - the one file that goes beside the
# `sharp` program so that `import ( std.guikit.* )` works.   (Linux / WSL / a Raspberry Pi)
#
#     bash build-scripts/linux/build_gui_library.sh               for the Sharp being worked on, or else the installed one
#     bash build-scripts/linux/build_gui_library.sh installed     for the installed Sharp, always
#
# Needs:  sudo apt install g++ libsdl2-dev
#
# WHICH SHARP IT IS FOR - it finds that out by itself:
#
#   THE SHARP BEING WORKED ON (build.sh has built a VM and a compiler in ~/sharp_work):
#       the bridge is written by that compiler from the repository's library
#       -> ~/sharp_work/out/gui_lib/sharpgui.so      (release.sh packs it from there)
#
#   AN INSTALLED SHARP (~/.sharp, or SHARP_HOME, or `sharpc` on the PATH) - a Raspberry Pi:
#       the bridge is written by the installed compiler from the INSTALLED library, and the
#       library is built from the installed library's C++, so all three are the same version
#       -> build/gui/sharpgui.so, and COPIED INTO THE INSTALLATION'S bin/ - nothing else to do.
#       (linux/install_from_source.sh runs this itself at its end.)
#
# TWO STEPS
#   1. THE BRIDGE. internal/every_native.sharp names every native function of the GUI. Compiling it with
#      -sni makes the compiler write the C++ that connects Sharp to those functions
#      (sharpgui_sni.cpp / .h). It is plain C++, the same for every system.
#   2. THE LIBRARY. The bridge and the C++ of std/guikit/platform/native, compiled into one file.
#
#     GLUE=<folder>   a bridge that already exists (sharpgui_sni.cpp in it): skip step 1
#     OUT=<folder>    where sharpgui.so goes
#     CXX=<compiler>  the C++ compiler (default g++)
source "$(dirname "$0")/../internal/common.sh"
CXX=${CXX:-g++}
command -v sdl2-config > /dev/null || fail "SDL2's development files are missing:  sudo apt install libsdl2-dev"
[ -z "$1" ] || [ "$1" = installed ] || fail "build_gui_library.sh: unknown argument '$1' (nothing, or: installed)"

# ---- which Sharp: the one being worked on, or an installed one
# (the installed `sharpc` adds its own library itself - giving it another -L would load
#  the whole library twice, and every function would be there two times)
INSTALL=""
if [ "$1" != installed ] && [ -x "$VM" ] && [ -f "$COMPILER" ]; then
    MAKE_BRIDGE=( "$VM" "$COMPILER" -L "$LIBRARY_SOURCE" )
    NATIVE="$LIBRARY_SOURCE/std/guikit/platform/native"
    OUT=${OUT:-$WORK/out/gui_lib}
    echo "for the Sharp being worked on (the compiler in $WORK)"
else
    INSTALL="${SHARP_HOME:-$HOME/.sharp}"
    if [ ! -x "$INSTALL/bin/sharpc" ]; then
        found=$(command -v sharpc)
        [ -n "$found" ] || fail "no Sharp to build it for: install Sharp first (bash build-scripts/linux/install_from_source.sh)"
        INSTALL=$(cd "$(dirname "$found")/.." && pwd)
    fi
    MAKE_BRIDGE=( "$INSTALL/bin/sharpc" )
    NATIVE="$INSTALL/lib/std/guikit/platform/native"
    OUT=${OUT:-$REPO/build/gui}
    echo "for the Sharp installed in $INSTALL ($("$INSTALL/bin/sharp" -V 2>&1 | head -1))"
fi
[ -d "$NATIVE" ] || fail "no GUI sources at $NATIVE"
mkdir -p "$OUT"

# ---- 1. the bridge
if [ -n "$GLUE" ]; then
    [ -f "$GLUE/sharpgui_sni.cpp" ] || fail "no sharpgui_sni.cpp in $GLUE"
else
    GLUE="$REPO/build/gui/glue"; rm -rf "$GLUE"; mkdir -p "$REPO/build/gui"
    "${MAKE_BRIDGE[@]}" "$REPO/build-scripts/internal/every_native.sharp" -j 1 -sni "$GLUE" -o "$OUT/every_native" > "$OUT/bridge_compile.txt" 2>&1
    [ -f "$GLUE/sharpgui_sni.cpp" ] || { echo "the bridge was not written:"; grep -v "^$" "$OUT/bridge_compile.txt" | head -20; exit 1; }
fi
echo "bridge: $(grep -c 'sharpgui_' "$GLUE/sharpgui_sni.h" 2>/dev/null) native functions in $GLUE"

# ---- 2. the library
TARGET="$OUT/sharpgui.so"; rm -f "$TARGET"
"$CXX" -shared -fPIC -std=c++17 -O2 -w -I "$GLUE" -I "$NATIVE" $(sdl2-config --cflags) \
    "$GLUE/sharpgui_sni.cpp" "$NATIVE"/*.cpp -o "$TARGET" $(sdl2-config --libs) 2> "$OUT/native_build.txt"
if [ ! -f "$TARGET" ]; then echo "the library did not build:"; grep -m 20 "error" "$OUT/native_build.txt"; exit 1; fi
echo "built: $TARGET   ($(stat -c %s "$TARGET") bytes)"

# ---- an installed Sharp: put it where std.guikit looks for it, beside the VM
if [ -n "$INSTALL" ]; then
    cp "$TARGET" "$INSTALL/bin/sharpgui.so"
    echo "installed: $INSTALL/bin/sharpgui.so - GUI programs run now"
else
    echo "it goes beside the sharp program (the installers put it in bin/)"
fi
