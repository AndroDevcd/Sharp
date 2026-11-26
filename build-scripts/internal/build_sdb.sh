#!/bin/bash
# Builds sdb, the Sharp debugger, for Linux from the work folder's copy of the sources - the way
# build.sh builds the VM.   A PART OF release.sh and of the debugger's tests.   (Linux / WSL)
#
#     bash build-scripts/internal/build_sdb.sh        -> ~/sharp_work/sdb_dev/sdb
#
# (linux/build_sdb.sh alone builds straight out of the repository, into build/linux-sdb: that is the
#  one for a machine without the work folder, a Raspberry Pi say.)
source "$(dirname "$0")/common.sh"
copy_sources || exit 1
# WSL's usual 3 GB is not enough for many compiles at once: 3 at a time below 4 GB (as build.sh does)
if [ -z "$JOBS" ] && [ "$(awk '/MemTotal/ {print int($2/1024)}' /proc/meminfo)" -lt 4000 ]; then export JOBS=3; fi
SOURCES="$WORK/src" OUT="$WORK/sdb_dev" bash "$REPO/build-scripts/linux/build_sdb.sh" || exit 1
[ -x "$WORK/sdb_dev/sdb" ] || fail "no sdb was built"
