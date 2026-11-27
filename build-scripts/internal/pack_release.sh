#!/bin/bash
# Packs a release: releases/sharp-<version>.zip.   A PART OF release.sh (its last step).   (Linux / WSL)
#
#   sharp-<version>/
#     START-HERE.txt   VERSION.txt
#     windows/   win-install.cmd  win-uninstall.cmd  sharp.exe  sdb.exe  sharpc.cmd
#     linux/     unix-install.sh  unix-uninstall.sh  sharp      sdb      sharpc
#     shared/    compiler/sharpc-v<version>  sharpc-unoptimized-v<version>  sharpc-debug-v<version>  lib/  examples/
#     clion-plugin/   sharp-clion-plugin-<plugin version>.zip   (installed in the IDE by hand)
#
# It packs what the steps of release.sh before it built, and says which one is missing:
#   the Linux VM and the compiler   build.sh                         (-> $WORK/sharp_dev, $WORK/out/boot)
#   the release compilers           internal/release_compilers.sh    (-> $WORK/out/release_build)
#   the GUI library (Linux)         linux/build_gui_library.sh       (-> $WORK/out/gui_lib)
#   the Windows VM                  windows/build_vm.sh              (-> cmake-build-windows-vm)
#   the debugger                    internal/build_sdb.sh, windows/build_sdb.sh   (-> $WORK/sdb_dev, cmake-build-windows-sdb)
#   the GUI library (Windows)       windows/build_gui_library.sh     (-> build/gui)
#   the CLion plugin                windows/build_plugin.sh          (-> tools/clion-plugin/build/distributions)
# and the installers and texts of internal/zip-contents/, with the version filled in.
#
# The version in the names is the LANGUAGE version (compiler + standard library), read from the
# compiler's own source; the VM's version is written into VERSION.txt.
source "$(dirname "$0")/common.sh"
W=$WORK
T="$REPO/build-scripts/internal/zip-contents"
V=$(grep -o 'program_vers : string = "[^"]*"' "$REPO/compiler/configuration/configuration.sharp" | cut -d'"' -f2)
VM_V=$(grep -o '#define progvers "[^"]*"' "$REPO/runtime/main.h" | cut -d'"' -f2)$(grep -o '#define rev "[^"]*"' "$REPO/runtime/main.h" | cut -d'"' -f2)
[ -n "$V" ] || { echo "the compiler's version was not found"; exit 1; }
LINUX_OBJECTS=${LINUX_OBJECTS:-$W/sharp_dev}
WINDOWS_VM=${WINDOWS_VM:-$REPO/cmake-build-windows-vm/sharp.exe}
LINUX_SDB_OBJECTS=${LINUX_SDB_OBJECTS:-$W/sdb_dev}
WINDOWS_SDB=${WINDOWS_SDB:-$REPO/cmake-build-windows-sdb/sdb.exe}
COMPILERS=${COMPILERS:-$W/out/release_build}
STAGE=$W/out/release; D=$STAGE/sharp-$V
# the standard library in the repository; the release ships its contents as `lib/`
LIB_SRC="$REPO/stdlib"
for need in "$LINUX_OBJECTS/sharp" "$LINUX_SDB_OBJECTS/sdb" "$WINDOWS_VM" "$WINDOWS_SDB" "$COMPILERS/O/sharpc" "$COMPILERS/plain/sharpc" "$COMPILERS/debug/sharpc" "$LIB_SRC"; do
  [ -e "$need" ] || { echo "missing: $need   (run  bash build-scripts/release.sh  - it builds everything in order)"; exit 1; }
done
rm -rf "$STAGE"; mkdir -p "$D/windows" "$D/linux" "$D/shared/compiler" "$D/shared/examples"

# ---- the Linux VM, linked again with the C++ library inside it (so it runs where another g++ is installed, or none)
g++ -static-libstdc++ -static-libgcc -o "$D/linux/sharp" "$LINUX_OBJECTS"/*.o -pthread -ldl -lrt -lm 2>&1 | grep -v warning | head -5
[ -x "$D/linux/sharp" ] || { echo "the Linux VM did not link"; exit 1; }
strip "$D/linux/sharp" 2>/dev/null
cp "$WINDOWS_VM" "$D/windows/sharp.exe"

# ---- sdb, the debugger: the same VM with debugger compiled in, beside each VM
g++ -static-libstdc++ -static-libgcc -o "$D/linux/sdb" "$LINUX_SDB_OBJECTS"/*.o -pthread -ldl -lrt -lm 2>&1 | grep -v warning | head -5
[ -x "$D/linux/sdb" ] || { echo "the Linux sdb did not link"; exit 1; }
strip "$D/linux/sdb" 2>/dev/null
cp "$WINDOWS_SDB" "$D/windows/sdb.exe"
# (sdb says the VM it is built from: it must be THIS release's, or it could not load this release's programs)
"$D/linux/sdb" -V 2>&1 | grep -q "($(basename "$D/linux/sharp") $VM_V)" || { echo "the Linux sdb is not built from VM $VM_V: rebuild it (internal/build_sdb.sh)"; exit 1; }

# ---- the GUI's native library, beside each VM (std.guikit loads it from the VM's own folder).
# Built by linux/build_gui_library.sh and windows/build_gui_library.sh. A release without one still
# works for everything but GUI programs - so a missing library is said loudly, not fatal.
LINUX_GUI=${LINUX_GUI:-$W/out/gui_lib/sharpgui.so}
WINDOWS_GUI=${WINDOWS_GUI:-$REPO/build/gui/sharpgui.dll}
GUI_NOTE=""
if [ -f "$LINUX_GUI" ]; then cp "$LINUX_GUI" "$D/linux/sharpgui.so"; else GUI_NOTE="$GUI_NOTE linux"; echo "WARNING: no $LINUX_GUI - GUI programs will not run from the Linux install"; fi
if [ -f "$WINDOWS_GUI" ]; then cp "$WINDOWS_GUI" "$D/windows/sharpgui.dll"; else GUI_NOTE="$GUI_NOTE windows"; echo "WARNING: no $WINDOWS_GUI - GUI programs will not run from the Windows install"; fi
[ "$("$D/linux/sharp" -V 2>&1 | head -1 | awk '{print $2}')" = "$VM_V" ] || { echo "the Linux VM is not version $VM_V: rebuild it"; exit 1; }

# ---- the CLion / IntelliJ plugin, as the zip the IDE installs from disk. It has its own version.
# Built by windows/build_plugin.sh. A release without it is complete for the command line - so a
# missing plugin is said loudly, not fatal.
PLUGIN_V=$(grep '^pluginVersion=' "$REPO/tools/clion-plugin/gradle.properties" | cut -d= -f2 | tr -d '')
PLUGIN_ZIP=${PLUGIN_ZIP:-$REPO/tools/clion-plugin/build/distributions/sharp-clion-plugin-$PLUGIN_V.zip}
if [ -f "$PLUGIN_ZIP" ]; then mkdir -p "$D/clion-plugin"; cp "$PLUGIN_ZIP" "$D/clion-plugin/"; else PLUGIN_V=""; echo "WARNING: no $PLUGIN_ZIP - this release has no CLion plugin"; fi

# ---- the compilers and the library
cp "$COMPILERS/O/sharpc" "$D/shared/compiler/sharpc-v$V"
cp "$COMPILERS/plain/sharpc" "$D/shared/compiler/sharpc-unoptimized-v$V"
cp "$COMPILERS/debug/sharpc" "$D/shared/compiler/sharpc-debug-v$V"
[ "$("$D/linux/sharp" "$D/shared/compiler/sharpc-debug-v$V" -V 2>&1 | head -1)" = "sharpc $V (debug)" ] || { echo "the packed debug compiler is not 'sharpc $V (debug)': rebuild the release compilers"; exit 1; }
said=$("$D/linux/sharp" "$D/shared/compiler/sharpc-v$V" -V 2>&1 | head -1)
[ "$said" = "sharpc $V" ] || { echo "the packed compiler says '$said', not 'sharpc $V': rebuild the release compilers"; exit 1; }
cp -r "$LIB_SRC" "$D/shared/lib"
# Unix line endings in the library: a Windows checkout has Windows ones, and on Linux a line then ends in an
# invisible \r that every `sed` or `grep ...$` of the installed files trips over (the compiler reads both)
find "$D/shared/lib" -type f \( -name '*.sharp' -o -name '*.cpp' -o -name '*.h' \) -exec sed -i 's/\r$//' {} +

# ---- the scripts and texts: the version filled in; Windows files with Windows line ends, Unix files with Unix ones
fill() {   # fill <template> <target> <crlf|lf>
  sed "s/@VERSION@/$V/g" "$1" | tr -d '\r' > "$2"
  [ "$3" = crlf ] && sed -i 's/$/\r/' "$2"
  return 0
}
fill "$T/START-HERE.txt" "$D/START-HERE.txt" crlf
# the licence, and the terms of the code by others that is built in: a copy goes with every release
for f in LICENSE doc/NOTICE doc/THIRD_PARTY_LICENSES.txt; do tr -d '\r' < "$REPO/$f" | sed 's/$/\r/' > "$D/$(basename "$f")"; done
for f in win-install.cmd win-uninstall.cmd sharpc.cmd; do fill "$T/windows/$f" "$D/windows/$f" crlf; done
for f in unix-install.sh unix-uninstall.sh sharpc; do fill "$T/linux/$f" "$D/linux/$f" lf; chmod +x "$D/linux/$f"; done
fill "$T/shared/examples/hello.sharp" "$D/shared/examples/hello.sharp" lf
commit=$(git -C "$REPO" rev-parse --short HEAD 2>/dev/null)
# (seen from WSL a Windows checkout differs in line ends and file modes only: those are not changes)
[ -n "$(git -C "$REPO" -c core.fileMode=false -c core.autocrlf=true status --porcelain 2>/dev/null | grep -v '^??' | head -1)" ] && commit="$commit + changes not committed yet"
{
  echo "Sharp $V"
  echo ""
  echo "language (compiler and standard library): $V"
  echo "virtual machine:                          $VM_V"
  echo "built:                                    $(date -u +%Y-%m-%d)"
  echo "source:                                   commit $commit"
  echo ""
  echo "shared/compiler/sharpc-v$V                 the compiler, compiled by itself with -O"
  echo "shared/compiler/sharpc-unoptimized-v$V     the same compiler, compiled by itself without -O"
  echo "linux/sharp                               x86-64 Linux, needs glibc $(ldd --version | head -1 | grep -o '[0-9]*\.[0-9]*$') or newer"
  echo "windows/sharp.exe                         x86-64 Windows, needs nothing else"
  echo "linux/sdb, windows/sdb.exe                the debugger: $("$D/linux/sdb" -V 2>&1 | head -1)   (sdb -h; build programs with -dbg)"
  [ -f "$D/linux/sharpgui.so" ] && echo "linux/sharpgui.so                         the GUI (std.guikit); needs SDL2 on the machine:  sudo apt install libsdl2-2.0-0"
  [ -f "$D/windows/sharpgui.dll" ] && echo "windows/sharpgui.dll                      the GUI (std.guikit); needs nothing else"
  [ -n "$GUI_NOTE" ] && echo "(no GUI library in this release for:$GUI_NOTE)"
  [ -n "$PLUGIN_V" ] && echo "clion-plugin/                             the CLion / IntelliJ plugin $PLUGIN_V (Settings > Plugins > Install Plugin from Disk)"
} | sed 's/$/\r/' > "$D/VERSION.txt"

# ---- the zip (python: the same on every machine, and the Unix files keep their "can be run" bit)
mkdir -p "$REPO/releases"
ZIP="$REPO/releases/sharp-$V.zip"
rm -f "$ZIP"
python3 - "$STAGE" "sharp-$V" "$ZIP" <<'EOF'
import os, sys, zipfile
stage, top, target = sys.argv[1:4]
with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for folder, folders, files in os.walk(os.path.join(stage, top)):
        folders.sort()
        for name in sorted(files):
            path = os.path.join(folder, name)
            z.write(path, os.path.relpath(path, stage))
print("files:", len(zipfile.ZipFile(target).namelist()))
EOF
ls -la "$ZIP" | cut -c1-140
echo "release: $ZIP   (language $V, VM $VM_V)"
