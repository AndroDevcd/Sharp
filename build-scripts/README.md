# Building, testing and releasing Sharp

Three scripts do everything:

| To | Run | Where | Time |
|---|---|---|---|
| build Sharp | `bash build-scripts/build.sh` | Linux / WSL | minutes the first time, then seconds |
| test it | `bash build-scripts/test.sh` | Linux / WSL | 1 to 2 minutes |
| make a release | `bash build-scripts/release.sh 0.7.1` | Git Bash on Windows | 25 minutes |

Each script starts with a comment that says what it does, its options, and what it prints.

```
build-scripts/
  build.sh       the VM and the compiler          (build.sh compiler = only the compiler, 10 seconds)
  test.sh        the tests                        (test.sh full = all of them, 1 hour)
  release.sh     build + test + pack the zip      (release.sh 0.7.1 install = also install it on this PC)

  linux/         what only Linux does:    build_vm.sh   build_gui_library.sh   install_from_source.sh
  windows/       what only Windows does:  build_vm.sh   build_gui_library.sh   install_release.sh
                                          suite.sh  (the suite on the Windows VM, with the JIT and without)
  internal/      parts the three scripts use - nothing in it is run by hand

tests/cffi/      calls to C (extern blocks), in seconds:
                   engine_test.sh   the calling conventions alone, without the VM
                   run.sh           from Sharp: numbers, structs, memory, callbacks, mistakes, a project
                 (both run in WSL and in Git Bash; on Windows, before a release is installed:
                  COMPILER=<the sharpc_boot built in WSL> bash tests/cffi/run.sh)
```

## What Sharp is made of

| Piece | Written in | Where | What it becomes |
|---|---|---|---|
| The VM | C++ | `lib/runtime`, `lib/core`, `main.cpp` | the program `sharp` (`sharp.exe`) |
| The compiler | Sharp | `lib/compiler` | one file, `sharpc`, that the VM runs |
| The standard library | Sharp | `lib/support/0.4.0` | shipped as source files |
| The GUI's native part | C++ | `lib/support/0.4.0/std/guikit/platform/native` | `sharpgui.so` / `sharpgui.dll` |

- **Only the VM (and the GUI library) differs per system**, so those are built once on Linux and
  once on Windows. The compiler and the library are the same files everywhere.
- **The compiler compiles itself**: it is built by its own last good build. `build.sh` explains.

## What you need installed

In Linux / WSL: `sudo apt install g++ rsync python3 libsdl2-dev`

On Windows, only for a release: Git Bash, a MinGW g++ (the one inside CLion is found by itself, see `windows/mingw.sh`),
and SDL2's MinGW files in `build/deps/` (`windows/build_gui_library.sh` says where to get them).

On a Windows PC, "Linux / WSL" means:

```
wsl
cd "/mnt/c/Users/<you>/Documents/Github/Sharp"
```

## Build

```
bash build-scripts/build.sh
```

It ends with `built: Sharp <version> (VM <version>)`. What it built is in `~/sharp_work`:

```
~/sharp_work/sharp_dev/sharp           the VM
~/sharp_work/out/boot/sharpc_boot      the compiler
~/sharp_work/stdlib                    the standard library
```

Try it on a program:

```
~/sharp_work/sharp_dev/sharp ~/sharp_work/out/boot/sharpc_boot main.sharp -o app -L ~/sharp_work/stdlib
~/sharp_work/sharp_dev/sharp app
```

## Test

```
bash build-scripts/test.sh
```

The last line is the answer: `TESTS: PASS (...)` or `TESTS: FAIL`. On FAIL, the test above it that
reports failures names the cases.

| What you changed | Run |
|---|---|
| anything | `test.sh` |
| the standard library | also `test.sh only 8` (the suite: every case of `tests/suite` compiled and run) |
| code generation or the optimizer | also `test.sh only 7 8 11 13` |
| the VM, or before a big release | `test.sh full` |

Tests run side by side: `TEST_GROUPS=<n>` says how many tests at once (default 4, `1` = one after the other) and
`TEST_JOBS=<n>` how many compiles and programs inside each (default: one per core).

`test.sh list` shows every test with its number. The tests themselves are in `tests/suite/`
(the language and the library, one program per case) and `lib/compiler/tests/` (the compiler).

## Release

1. **Pick the version number** - the language version (compiler + library), for example `0.7.1`.
   If the VM changed too, raise its revision by hand first: `lib/runtime/main.h`, `#define rev`
   (`r19` -> `r20`).

2. **Run it**, from Git Bash:

   ```
   bash build-scripts/release.sh 0.7.1 install
   ```

   It writes the number into the sources, builds both systems' halves, runs the short tests, packs
   `releases/sharp-0.7.1.zip` and installs it on this PC. It stops at the first step that fails and
   says which.

   Without a number it releases the version the sources already say. Either way, when that
   version's zip already exists it asks `Replace it? [Y/n]` and only a typed `Y` goes on.

3. **Open a new terminal** and try it: `sharp -V`, `sharpc -V`.

4. **Commit and push**, the zip included. The script never does that for you.

The zip has an installer for each system (`windows\win-install.cmd`, `linux/unix-install.sh`).
A Raspberry Pi builds its own VM instead: `bash build-scripts/linux/install_from_source.sh`.

## When something goes wrong

- **A script says something is missing.** It names what to run first. `build.sh` is the start of everything.
- **The compiler does not build after an edit.** Nothing was replaced; fix the error and run again.
- **Something is in a strange state.** Delete `~/sharp_work` and run `build.sh`: nothing in that
  folder is precious, and it is all built again.
- **WSL runs out of memory building the VM.** `JOBS=2 bash build-scripts/build.sh`.
