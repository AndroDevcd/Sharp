#!/bin/bash
# Builds sdb, the Sharp debugger (sdb.exe), for Windows x64, with a MinGW g++.
#
#   bash build-scripts/windows/build_sdb.sh           from Git Bash  ->  cmake-build-windows-sdb/sdb.exe
#
# sdb is the VM's own sources with debugger compiled in and -DSHARP_DEBUGGER_ set
# (debugger/hooks.h says how the two are one code base), built without the machine-code
# compiler: the interpreter runs everything, which is where the debugger's hooks are.
# So this script only says what is different and lets build_vm.sh do the work; MINGW, OUT and
# JOBS mean what they mean there, and arguments are more flags for the compiler.
#
# Its objects are its own (cmake-build-windows-sdb): the VM's are compiled without the hooks.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${OUT:-$ROOT/cmake-build-windows-sdb} PROGRAM=sdb MAIN=debugger/main.cpp MORE_SOURCES="debugger/*/*.cpp" SHARP_NO_JIT=1 \
    bash "$ROOT/build-scripts/windows/build_vm.sh" -DSHARP_DEBUGGER_ "$@"
