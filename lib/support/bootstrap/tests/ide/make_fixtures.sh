#!/bin/bash
# Make real compiler output for the plugin's tests (doc/ide.md 7.3, "fixtures copied from real compiler output").
#
#   tests/ide/make_fixtures.sh <destination folder>
#       SHARP=<the runtime>  COMPILER=<the compiled bootstrap compiler>  LIBRARY=<the standard library folder>
#       (defaults: ~/sharp_work/sharp_cl/sharp, ~/sharp_work/out/boot_ide/sharpc_boot, ~/sharp_work/stdlib)
#
# For each sample of tests/ide/samples it compiles the sample with the standard library and `-ide`,
# asking for the sample's bodies file (a bodies_for.txt that lists only the sample), and leaves the
# folder the compiler wrote in <destination>/<sample name>/. The files name the paths of THIS machine;
# the plugin's fixture loader is told where the library and the samples are.
#
# A sample of two files is `<name>.sharp` and `<name>_more.sharp` (corners): the two are compiled
# together into <destination>/<name>/, and both get a bodies file.
#
# The output for the library is about 1.2 MB (declarations.json): do not check it in; make it when needed.

HERE="$(cd "$(dirname "$0")" && pwd)"
SHARP=${SHARP:-$HOME/sharp_work/sharp_cl/sharp}
COMPILER=${COMPILER:-$HOME/sharp_work/out/boot_ide/sharpc_boot}
LIBRARY=${LIBRARY:-$HOME/sharp_work/stdlib}
LIB=$(find "$LIBRARY" -name "*.sharp" | sort)
DEST=${1:?usage: make_fixtures.sh <destination folder>}

for sample in "$HERE"/samples/*.sharp; do
    name=$(basename "$sample" .sharp)
    case "$name" in *_more) continue ;; esac            # the second file of a sample: compiled with the first
    folder="$DEST/$name"
    rm -rf "$folder"; mkdir -p "$folder"
    echo "$sample" > "$folder/bodies_for.txt"
    more="$HERE/samples/${name}_more.sharp"
    if [ -f "$more" ]; then
        echo "$more" >> "$folder/bodies_for.txt"
        timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB "$sample" "$more" -ide "$folder" > "$folder/compiler_output.txt" 2>&1
    else
        timeout -s KILL 300 "$SHARP" "$COMPILER" $LIB "$sample" -ide "$folder" > "$folder/compiler_output.txt" 2>&1
    fi
    echo "$name: $(ls "$folder" | tr '\n' ' ')"
done
