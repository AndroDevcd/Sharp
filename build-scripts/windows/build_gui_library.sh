#!/bin/bash
# Builds the GUI's native library for Windows: sharpgui.dll - the one file that goes beside
# sharp.exe so that `import ( std.guikit.* )` works.   (Git Bash, on Windows)
#
#     bash build-scripts/windows/build_gui_library.sh
#
#     -> build/gui/sharpgui.dll
#
# NEEDS
#   1. THE BRIDGE: build/gui/glue/sharpgui_sni.cpp - the C++ that connects Sharp to the GUI's native
#      functions. linux/build_gui_library.sh writes it there (run that one first, in WSL). It is
#      plain C++, the same for every system. When it is not there and Sharp is installed on this
#      PC, the installed `sharpc` writes it instead.
#   2. A MinGW g++. The one that ships with CLion is used unless MINGW says otherwise.
#   3. SDL2's MinGW development files: download SDL2-devel-<version>-mingw.zip from
#      https://github.com/libsdl-org/SDL/releases (an SDL 2.x release) and unpack it into build/deps/,
#      so that build/deps/SDL2-<version>/x86_64-w64-mingw32/ exists.
#      SDL2 is linked INTO sharpgui.dll: no SDL2.dll has to be shipped beside it.
#
#     MINGW=<folder>   the folder with g++.exe          (default: CLion's MinGW)
#     SDL2=<folder>    .../x86_64-w64-mingw32            (default: the newest in build/deps/)
#     GLUE=<folder>    the bridge                        (default: build/gui/glue)
#     OUT=<folder>     where sharpgui.dll goes           (default: build/gui)
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${OUT:-$ROOT/build/gui}; mkdir -p "$OUT"
GLUE=${GLUE:-$ROOT/build/gui/glue}
LIBRARY="$ROOT/lib/support/$(ls "$ROOT/lib/support" | sort -V | tail -1)"
NATIVE="$LIBRARY/std/guikit/platform/native"
SDL2=${SDL2:-$(ls -d "$ROOT"/build/deps/SDL2-*/x86_64-w64-mingw32 2>/dev/null | sort -V | tail -1)}
source "$ROOT/build-scripts/windows/mingw.sh"
[ -f "$SDL2/lib/libSDL2.a" ] || { echo "SDL2's MinGW development files were not found in build/deps/ (the top of this script says where to get them)"; exit 1; }

# ---- the bridge
if [ ! -f "$GLUE/sharpgui_sni.cpp" ]; then
    # (the installed compiler is started directly, not through sharpc.cmd: a .cmd started
    #  from this shell breaks on a folder with a space in its name)
    INSTALLED="$LOCALAPPDATA/Sharp"
    COMPILER=$(ls "$INSTALLED"/compiler/sharpc-v* 2>/dev/null | sort -V | tail -1)
    [ -n "$COMPILER" ] || { echo "no bridge in $GLUE: run  bash build-scripts/linux/build_gui_library.sh  in WSL first"; exit 1; }
    "$INSTALLED/bin/sharp.exe" "$COMPILER" "$ROOT/build-scripts/internal/every_native.sharp" -j 1 -L "$LIBRARY" -sni "$GLUE" -o "$OUT/every_native" > "$OUT/bridge_compile.txt" 2>&1
    [ -f "$GLUE/sharpgui_sni.cpp" ] || { echo "the bridge was not written:"; grep -v "^$" "$OUT/bridge_compile.txt" | head -20; exit 1; }
fi
echo "bridge: $(grep -c 'sharpgui_' "$GLUE/sharpgui_sni.h" 2>/dev/null) native functions in $GLUE"

# ---- the library
# -static: SDL2 and the C++ runtime go INTO the dll; what follows -lSDL2 is what SDL2 itself needs from Windows
TARGET="$OUT/sharpgui.dll"; rm -f "$TARGET"
g++ -shared -std=c++17 -O2 -w $CRT_FLAG -I "$GLUE" -I "$NATIVE" -I "$SDL2/include" -I "$SDL2/include/SDL2" \
    "$GLUE/sharpgui_sni.cpp" "$NATIVE"/*.cpp -o "$TARGET" \
    -static -L "$SDL2/lib" -lSDL2 \
    -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid \
    2> "$OUT/native_build.txt"
if [ ! -f "$TARGET" ]; then echo "the library did not build:"; grep -m 20 "error" "$OUT/native_build.txt"; exit 1; fi
echo "built: $TARGET   ($(stat -c %s "$TARGET") bytes)"
echo "it goes beside sharp.exe (the installer puts it in bin\\)"
