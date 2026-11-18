#!/bin/bash
# SHARED BY THE BUILD SCRIPTS: where things are. Not run on its own - build.sh, test.sh and the
# others read it (`source .../internal/common.sh`).
#
# THE WORK FOLDER. Building straight out of the repository is slow when the repository is on a
# Windows drive seen from WSL (/mnt/c/...): ten times slower, and long builds make WSL drop the
# connection. So the Linux-side scripts first COPY what they compile into a folder on Linux's own
# disk and build there. Nothing in it is precious: delete it and the next build makes it again
# (the first build is then a full one).
#
#     SHARP_WORK       the work folder                        (default ~/sharp_work)
#
#     $WORK/src        a copy of the repository's sources     (copy_sources below makes it)
#     $WORK/stdlib     a copy of the standard library         ( "  )
#     $WORK/suite      a copy of tests/suite                  ( "  )
#     $WORK/sharp_dev  the Linux VM and its object files      (build.sh)
#     $WORK/out/boot   the compiler: sharpc_boot              (build.sh)
#     $WORK/out/seed   the compiler that builds the next one  (build.sh explains)
#     $WORK/out/...    what the tests and the release steps write
#
# (The folder names inside are historical - "sharp_dev", "boot" - and are kept because the test
#  scripts in lib/compiler/tests use them as their defaults.)

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${SHARP_WORK:-$HOME/sharp_work}"

VM="$WORK/sharp_dev/sharp"                  # the Linux VM the scripts build and use
COMPILER="$WORK/out/boot/sharpc_boot"       # the compiler they build and use (a Sharp program: run as  $VM $COMPILER ...)
SEED="$WORK/out/seed/sharpc_boot"           # the compiler that compiles the compiler
STDLIB="$WORK/stdlib"                       # the copy of the standard library the compiles read

# the standard library's folder in the repository: lib/support/<number>, the newest one
LIBRARY_SOURCE="$REPO/lib/support/$(ls "$REPO/lib/support" 2>/dev/null | sort -V | tail -1)"

# the two version numbers, read from the sources (never typed anywhere else)
LANGUAGE_VERSION=$(grep -o 'program_vers : string = "[^"]*"' "$REPO/lib/compiler/configuration/configuration.sharp" 2>/dev/null | cut -d'"' -f2)
VM_VERSION=$(grep -o '#define progvers "[^"]*"' "$REPO/lib/runtime/main.h" 2>/dev/null | cut -d'"' -f2)$(grep -o '#define rev "[^"]*"' "$REPO/lib/runtime/main.h" 2>/dev/null | cut -d'"' -f2)

# Copy what gets compiled from the repository into the work folder (see the top of this file).
# Cheap when little changed: only changed files are copied.
copy_sources() {
    command -v rsync > /dev/null || { echo "rsync is needed:  sudo apt install rsync"; return 1; }
    mkdir -p "$WORK/src" "$WORK/suite" "$WORK/stdlib" "$WORK/out"
    rsync -a --delete --exclude .git --exclude 'cmake-build*' --exclude bin --exclude build --exclude releases \
          --exclude '*.aout' --exclude doc --exclude examples --exclude benchmark --exclude tools \
          --exclude /tests "$REPO/" "$WORK/src/"
    rsync -a --delete "$LIBRARY_SOURCE/" "$WORK/stdlib/"
    rsync -a --delete "$REPO/tests/suite/" "$WORK/suite/"
}

# stop with a message
fail() { echo "$*"; exit 1; }
