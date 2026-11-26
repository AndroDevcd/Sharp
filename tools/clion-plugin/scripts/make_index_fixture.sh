#!/bin/bash
# Makes the REAL compiler output the completion, hover and navigation tests read.
#
#   tools/clion-plugin/scripts/make_index_fixture.sh <sample.sharp> <destination folder>
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  LIBRARY=<the standard library folder>
#       (defaults: ~/sharp_work/sharp_cl/sharp, ~/sharp_work/out/boot_ide/sharpc_boot, ~/sharp_work/stdlib)
#
# Compiles the sample together with the whole standard library and `-ide`, asking for the sample's bodies
# file (a bodies_for.txt that lists only the sample). The folder the compiler wrote is left in the
# destination; `pack_index_fixture.sh` turns it into the small gzipped files the plugin's tests keep.
# Run it inside WSL (`wsl -e bash -lc '...'`): the compiler is a Linux program.

SHARP=${SHARP:-$HOME/sharp_work/sharp_cl/sharp}
COMPILER=${COMPILER:-$HOME/sharp_work/out/boot_ide/sharpc_boot}
LIBRARY=${LIBRARY:-$HOME/sharp_work/stdlib}
SAMPLE=${1:?usage: make_index_fixture.sh <sample.sharp> <destination folder>}
DEST=${2:?usage: make_index_fixture.sh <sample.sharp> <destination folder>}
LIB=$(find "$LIBRARY" -name "*.sharp" | sort)

rm -rf "$DEST"; mkdir -p "$DEST"
echo "$SAMPLE" > "$DEST/bodies_for.txt"
timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB "$SAMPLE" -ide "$DEST" > "$DEST/compiler_output.txt" 2>&1
tail -3 "$DEST/compiler_output.txt"
ls "$DEST"
