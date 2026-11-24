#!/bin/bash
# Builds Sharp on THIS machine and installs it for the current user - for a machine the release has no
# ready-made VM for (a Raspberry Pi, any ARM or 32-bit Linux).
#
#     bash build-scripts/linux/install_from_source.sh
#
# What it does:
#   1. builds the VM here                       (build-scripts/linux/build_vm.sh  ->  build/linux-vm/sharp)
#   2. takes the compiler and the standard library from the release of the version the sources
#      say (releases/sharp-<program_vers>.zip), or the newest release when that one is not there
#      (they are Sharp programs and Sharp files: the same on every machine - only the VM is built)
#   3. installs into ~/.sharp exactly as the release's own installer does, with the VM from step 1,
#      and proves it by compiling and running the hello example
#
# Afterwards, in a NEW terminal:   sharpc main.sharp -o app    then    sharp app
#
# Needs: g++ (C++17) and python3 (to unpack the release; any Raspberry Pi OS has it).
#   4. builds the GUI's native library here too (the release's is for 64-bit x86) and puts it beside
#      the VM - when SDL2 is installed (sudo apt install libsdl2-dev); without it everything but
#      GUI programs works, and it says so
#   5. builds sdb, the debugger, here as well (the release's is for 64-bit x86) - before step 3 installs
HERE="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$HERE" || exit 1

bash build-scripts/linux/build_vm.sh "$@" || exit 1
VM="${OUT:-$HERE/build/linux-vm}/sharp"

# the release of the version in the sources - not simply the highest number: an abandoned higher
# number (0.7.0, when the sources went back to the 0.4 line) must not win over the current one
WANTED=$(grep -o 'program_vers : string = "[^"]*"' lib/compiler/configuration/configuration.sharp 2>/dev/null | cut -d'"' -f2)
ZIP="releases/sharp-$WANTED.zip"
[ -n "$WANTED" ] && [ -f "$ZIP" ] || ZIP=$(ls releases/sharp-*.zip 2>/dev/null | sort -V | tail -1)
[ -n "$ZIP" ] || { echo "no release in releases/: there is nothing to take the compiler and the library from"; exit 1; }
WORK="$HERE/build/install-from-source"; rm -rf "$WORK"; mkdir -p "$WORK"
python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" "$ZIP" "$WORK" || { echo "could not unpack $ZIP (python3 is needed)"; exit 1; }
RELEASE=$(ls -d "$WORK"/sharp-* | head -1)
echo "compiler and library: from $ZIP"

# the release's VM is for 64-bit x86: the one built here takes its place
cp "$VM" "$RELEASE/linux/sharp"
rm -f "$RELEASE/linux/sharpgui.so"          # built for 64-bit x86: it would not load here (see the top of this script)

# ---- sdb, the debugger: the release's is for 64-bit x86 too, so it is built here as well (the VM's
# sources once more, with the debugger compiled in). Without it everything but debugging works.
rm -f "$RELEASE/linux/sdb"
echo ""
if OUT= bash "$HERE/build-scripts/linux/build_sdb.sh" "$@" && [ -x "$HERE/build/linux-sdb/sdb" ]; then
    cp "$HERE/build/linux-sdb/sdb" "$RELEASE/linux/sdb"
else
    echo "NOTE: sdb, the debugger, did not build (above) - everything but debugging works"
fi
chmod +x "$RELEASE"/linux/*
bash "$RELEASE/linux/unix-install.sh" || exit 1

# ---- the GUI: its native library, built on this machine for the Sharp just installed
echo ""
if command -v sdl2-config > /dev/null; then
    bash "$HERE/build-scripts/linux/build_gui_library.sh" installed         || echo "NOTE: the GUI library did not build (above) - everything but GUI programs works"
else
    echo "NOTE: no GUI: SDL2 is not installed. For GUI programs:  sudo apt install libsdl2-dev"
    echo "      then:  bash build-scripts/linux/build_gui_library.sh"
fi
