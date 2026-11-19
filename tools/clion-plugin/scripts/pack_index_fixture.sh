#!/bin/bash
# Turns the folder make_index_fixture.sh left into the files the plugin's tests keep:
#   tools/clion-plugin/scripts/pack_index_fixture.sh <compiler's folder> <fixture folder>
# The two big files (declarations.json is 1.1 MB: the whole standard library) are gzipped; the test
# helper (IndexFixture) unzips them into a temporary folder and puts the test file's path in place of
# the sample's own path.
SRC=${1:?usage: pack_index_fixture.sh <compiler folder> <fixture folder>}
DST=${2:?usage: pack_index_fixture.sh <compiler folder> <fixture folder>}
mkdir -p "$DST/bodies"
gzip -9 -c "$SRC/declarations.json" > "$DST/declarations.json.gz"
gzip -9 -c "$SRC/ide.json" > "$DST/ide.json.gz"
cp "$SRC/types.json" "$SRC/diagnostics.json" "$DST/"
cp "$SRC"/bodies/*.json "$DST/bodies/"
ls -la "$DST"
