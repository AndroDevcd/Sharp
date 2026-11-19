# legacy

Things that are no longer part of the project but are kept in case they are needed.

## sharpc++.zip - the C++ compiler

Until 15 Nov 2026 the Sharp compiler was a C++ program in `lib/grammar`. The compiler is now
`lib/compiler`, written in Sharp and compiled by itself; every release carries it
(`releases/sharp-<version>.zip`). The C++ one was archived here so the tree holds one compiler.

The archive is a complete snapshot taken from the commit before the move - everything the C++
compiler needs to build, with the paths it had:

```
sharpc++/
  lib/grammar/       the compiler
  lib/core/          shared with the VM at the time (opcodes, the executable's format)
  lib/util/          shared helpers
  main.cpp  stdimports.h  CMakeLists.txt  build.sh
```

Its version is 0.4.0 (the same language version as the first release of the Sharp-written compiler).

### Building it again

Unzip it anywhere and, inside `sharpc++/`:

```
g++ -std=c++17 -O1 -w -pthread -DMAKE_COMPILER \
    main.cpp lib/util/*.cpp lib/util/zip/*.cpp lib/core/*.cpp lib/core/opcode/*.cpp \
    $(find lib/grammar -name '*.cpp') -o sharpc -lrt -lm
```

or with CMake: set `SHARPC` in `CMakeLists.txt` (the line `#set(SHARPC "build")`) and build the
`Sharpc` target. Always run it with `-j 1`: its multi-threaded front end has a race.

This was checked when the archive was made: the unzipped copy builds and compiles a program that runs.

### Why it might be needed

- To compare what the two compilers do with the same program (the tests under `lib/compiler/tests`
  did this while the new compiler was being written; they use a C++ compiler binary when one is
  given, and the expected outputs they compare with are stored in the test files).
- To rebuild the Sharp-written compiler from nothing but source on a machine that has no release:
  the C++ compiler compiles `lib/compiler` (with the standard library), and the result compiles itself.
  With a release at hand this is not needed - the released compiler compiles the next one.

Comments in `lib/compiler` that say "C++ comparison: lib/grammar/..." point into this archive.
