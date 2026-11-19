#!/bin/bash
# Finds the MinGW g++ for the Windows builds.   A PART OF build_vm.sh and build_gui_library.sh: they `source` it.
#
#   MINGW       the folder with g++.exe. Given by you, or found here: the MinGW that ships with CLion,
#               in a Toolbox install or in the newest CLion under Program Files.
#   CRT_FLAG    what g++ needs to link a program with -static, empty for most MinGWs.
#
# THE C LIBRARY. A MinGW is built for one of Windows' two C libraries (the old msvcrt or the newer UCRT).
# The one in CLion 2026 has libraries built for UCRT, but g++ links against msvcrt unless told otherwise,
# and then -static ends in `undefined reference to __intrinsic_setjmpex`. So a small threaded program is
# linked here: when it works as it is nothing is added, and when it needs -mcrtdll=ucrt that is the flag.
# (UCRT is part of Windows 10 and later.)
if [ -z "$MINGW" ]; then
    MINGW="$LOCALAPPDATA/Programs/CLion/bin/mingw/bin"
    if [ ! -x "$MINGW/g++.exe" ]; then
        newest=$(ls -d "/c/Program Files/JetBrains/CLion"*/bin/mingw/bin 2>/dev/null | sort -V | tail -1)
        [ -n "$newest" ] && MINGW="$newest"
    fi
fi
[ -x "$MINGW/g++.exe" ] || { echo "no g++.exe in $MINGW (set MINGW=<folder>)"; exit 1; }
export PATH="$MINGW:$PATH"

CRT_FLAG=""
probe=$(mktemp -d)
printf '#include <thread>\n#include <cstdio>\nint main() { std::thread t([] { char b[8]; snprintf(b, 8, "%%d", 1); }); t.join(); }\n' > "$probe/probe.cpp"
if ! g++ -std=c++17 -static -pthread "$probe/probe.cpp" -o "$probe/probe.exe" 2> /dev/null; then
    if g++ -std=c++17 -static -pthread -mcrtdll=ucrt "$probe/probe.cpp" -o "$probe/probe.exe" 2> /dev/null; then
        CRT_FLAG="-mcrtdll=ucrt"
    else
        echo "this MinGW cannot link a static program ($MINGW)"; rm -rf "$probe"; exit 1
    fi
fi
rm -rf "$probe"
