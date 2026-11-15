#!/bin/bash
# Packages a release: releases/sharp-<version>.zip   (run in WSL / Linux, from anywhere)
#
#   sharp-<version>/
#     START-HERE.txt   VERSION.txt
#     windows/   win-install.cmd  win-uninstall.cmd  sharp.exe  sharpc.cmd
#     linux/     unix-install.sh  unix-uninstall.sh  sharp      sharpc
#     shared/    compiler/sharpc-v<version>  compiler/sharpc-unoptimized-v<version>  lib/  examples/
#
# BEFORE this script, build the four things it packs:
#   1. the Linux VM              bash ~/sharp_work/tools/sync.sh build       (-> ~/sharp_work/sharp_dev)
#   2. the bootstrap compiler    bash ~/sharp_work/tools/bootc.sh
#   3. the two release compilers bash build-scripts/build_release_compilers.sh   (-> ~/sharp_work/out/release_build)
#   4. the Windows VM            bash build-scripts/build_windows_vm.sh      (Git Bash, on Windows -> cmake-build-windows-vm)
#
# The version in the names is the LANGUAGE version (compiler + standard library), read from the
# compiler's own source; the VM's version is written into VERSION.txt.
REPO=$(cd "$(dirname "$0")/.." && pwd)
W=$HOME/sharp_work
T="$REPO/build-scripts/release"
V=$(grep -o 'program_vers : string = "[^"]*"' "$REPO/lib/support/bootstrap/configuration/configuration.sharp" | cut -d'"' -f2)
VM_V=$(grep -o '#define progvers "[^"]*"' "$REPO/lib/runtime/main.h" | cut -d'"' -f2)$(grep -o '#define rev "[^"]*"' "$REPO/lib/runtime/main.h" | cut -d'"' -f2)
[ -n "$V" ] || { echo "the compiler's version was not found"; exit 1; }
LINUX_OBJECTS=${LINUX_OBJECTS:-$W/sharp_dev}
WINDOWS_VM=${WINDOWS_VM:-$REPO/cmake-build-windows-vm/sharp.exe}
COMPILERS=${COMPILERS:-$W/out/release_build}
STAGE=$W/out/release; D=$STAGE/sharp-$V
for need in "$LINUX_OBJECTS/sharp" "$WINDOWS_VM" "$COMPILERS/O/sharpc" "$COMPILERS/plain/sharpc" "$REPO/lib/support/$V"; do
  [ -e "$need" ] || { echo "missing: $need   (see the top of this script for what to build first)"; exit 1; }
done
rm -rf "$STAGE"; mkdir -p "$D/windows" "$D/linux" "$D/shared/compiler" "$D/shared/examples"

# ---- the Linux VM, linked again with the C++ library inside it (so it runs where another g++ is installed, or none)
g++ -static-libstdc++ -static-libgcc -o "$D/linux/sharp" "$LINUX_OBJECTS"/*.o -pthread -ldl -lrt -lm 2>&1 | grep -v warning | head -5
[ -x "$D/linux/sharp" ] || { echo "the Linux VM did not link"; exit 1; }
strip "$D/linux/sharp" 2>/dev/null
cp "$WINDOWS_VM" "$D/windows/sharp.exe"
[ "$("$D/linux/sharp" -V 2>&1 | head -1 | awk '{print $2}')" = "$VM_V" ] || { echo "the Linux VM is not version $VM_V: rebuild it"; exit 1; }

# ---- the compilers and the library
cp "$COMPILERS/O/sharpc" "$D/shared/compiler/sharpc-v$V"
cp "$COMPILERS/plain/sharpc" "$D/shared/compiler/sharpc-unoptimized-v$V"
said=$("$D/linux/sharp" "$D/shared/compiler/sharpc-v$V" -V 2>&1 | head -1)
[ "$said" = "sharpc $V" ] || { echo "the packed compiler says '$said', not 'sharpc $V': rebuild the release compilers"; exit 1; }
cp -r "$REPO/lib/support/$V" "$D/shared/lib"

# ---- the scripts and texts: the version filled in; Windows files with Windows line ends, Unix files with Unix ones
fill() {   # fill <template> <target> <crlf|lf>
  sed "s/@VERSION@/$V/g" "$1" | tr -d '\r' > "$2"
  [ "$3" = crlf ] && sed -i 's/$/\r/' "$2"
  return 0
}
fill "$T/START-HERE.txt" "$D/START-HERE.txt" crlf
for f in win-install.cmd win-uninstall.cmd sharpc.cmd; do fill "$T/windows/$f" "$D/windows/$f" crlf; done
for f in unix-install.sh unix-uninstall.sh sharpc; do fill "$T/linux/$f" "$D/linux/$f" lf; chmod +x "$D/linux/$f"; done
fill "$T/shared/examples/hello.sharp" "$D/shared/examples/hello.sharp" lf
commit=$(git -C "$REPO" rev-parse --short HEAD 2>/dev/null)
[ -n "$(git -C "$REPO" status --porcelain 2>/dev/null | grep -v '^??' | head -1)" ] && commit="$commit + changes not committed yet"
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
