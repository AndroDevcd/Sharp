#!/bin/bash
# Copies the compiler just built on Linux to where the Windows half of the tests takes it from
# (build/test/sharpc): the compiler is a Sharp program, the same file runs on both VMs.
# A PART OF test.sh (the one at the top of the repository).   (Linux / WSL)
source "$(dirname "$0")/common.sh"
[ -f "$COMPILER" ] || { echo "no compiler at $COMPILER: run  bash build-scripts/build.sh  first"; exit 1; }
mkdir -p "$REPO/build/test"
cp "$COMPILER" "$REPO/build/test/sharpc"
echo "handed over: $REPO/build/test/sharpc"
