#!/bin/bash
# Builds Sharp on THIS machine and installs it for the current user - for a machine the release has no
# ready-made VM for (a Raspberry Pi, any ARM or 32-bit Linux).
#
#     bash build-scripts/install_from_source.sh
#
# What it does:
#   1. builds the VM here                       (build-scripts/build_linux_vm.sh  ->  build/linux-vm/sharp)
#   2. takes the compiler and the standard library from the newest release in releases/
#      (they are Sharp programs and Sharp files: the same on every machine - only the VM is built)
#   3. installs into ~/.sharp exactly as the release's own installer does, with the VM from step 1,
#      and proves it by compiling and running the hello example
#
# Afterwards, in a NEW terminal:   sharpc main.sharp -o app    then    sharp app
#
# Needs: g++ (C++17) and python3 (to unpack the release; any Raspberry Pi OS has it).
HERE="$(cd "$(dirname "$0")/.." && pwd)"
cd "$HERE" || exit 1

bash build-scripts/build_linux_vm.sh "$@" || exit 1
VM="${OUT:-$HERE/build/linux-vm}/sharp"

ZIP=$(ls releases/sharp-*.zip 2>/dev/null | sort -V | tail -1)
[ -n "$ZIP" ] || { echo "no release in releases/: there is nothing to take the compiler and the library from"; exit 1; }
WORK="$HERE/build/install-from-source"; rm -rf "$WORK"; mkdir -p "$WORK"
python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" "$ZIP" "$WORK" || { echo "could not unpack $ZIP (python3 is needed)"; exit 1; }
RELEASE=$(ls -d "$WORK"/sharp-* | head -1)
echo "compiler and library: from $ZIP"

# the release's VM is for 64-bit x86: the one built here takes its place
cp "$VM" "$RELEASE/linux/sharp"
chmod +x "$RELEASE"/linux/*
bash "$RELEASE/linux/unix-install.sh"
