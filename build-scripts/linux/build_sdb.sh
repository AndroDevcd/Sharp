#!/bin/bash
# Builds sdb, the Sharp debugger, on the machine it is run on.
#
#     bash build-scripts/linux/build_sdb.sh           -> build/linux-sdb/sdb
#
# sdb is the VM's own sources with debugger compiled in and -DSHARP_DEBUGGER_ set
# (debugger/hooks.h says how the two are one code base), built without the machine-code
# compiler: the interpreter runs everything, which is where the debugger's hooks are.
# So this script only says what is different and lets build_vm.sh do the work; OUT, SOURCES,
# JOBS, CXX and LDFLAGS mean what they mean there, and arguments are more flags for the compiler.
#
# Its objects are its own (build/linux-sdb): the VM's are compiled without the hooks.
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
OUT=${OUT:-$REPO/build/linux-sdb} PROGRAM=sdb MAIN=debugger/main.cpp MORE_SOURCES="debugger/*/*.cpp" SHARP_NO_JIT=1 \
    bash "$REPO/build-scripts/linux/build_vm.sh" -DSHARP_DEBUGGER_ "$@"
