# Building, testing and releasing Sharp

Three scripts do everything:

| To | Run | Where | Time |
|---|---|---|---|
| build Sharp | `bash build-scripts/build.sh` | Linux / WSL | minutes the first time, then seconds |
| test it | `bash test.sh` (at the top of the repository) | Git Bash on Windows, or Linux / WSL | 3 minutes; `full`: more than an hour |
| make a release | `bash build-scripts/release.sh 0.7.1` | Git Bash on Windows | 25 minutes |

Each script starts with a comment that says what it does, its options, and what it prints.

```
build-scripts/
  build.sh       the VM and the compiler          (build.sh compiler = only the compiler, 10 seconds)
  test.sh        the numbered tests of the Linux half: what the repository's test.sh runs (start THAT one)
  release.sh     build + test + pack the zip      (release.sh 0.7.1 install = also install it on this PC)

  linux/         what only Linux does:    build_vm.sh   build_sdb.sh   build_gui_library.sh   install_from_source.sh
  windows/       what only Windows does:  build_vm.sh   build_sdb.sh   build_gui_library.sh   build_plugin.sh   install_release.sh
                                          suite.sh      (the suite on the Windows VM, with the JIT and without)
                                          sdb_tests.sh  (the debugger's tests on Windows)
  internal/      parts the three scripts use - nothing in it is run by hand

tests/cffi/      calls to C (extern blocks), in seconds:
                   engine_test.sh   the calling conventions alone, without the VM
                   run.sh           from Sharp: numbers, structs, memory, callbacks, mistakes, a project
                   arm.sh           both of them for 32- and 64-bit ARM, on a PC: a cross compiler and qemu
                 (both run in WSL and in Git Bash; on Windows, before a release is installed:
                  COMPILER=<the sharpc_boot built in WSL> bash tests/cffi/run.sh)
```

## What Sharp is made of

| Piece | Written in | Where | What it becomes |
|---|---|---|---|
| The VM | C++ | `runtime` | the program `sharp` (`sharp.exe`) |
| The debugger | C++ | `debugger` (with the VM's sources) | the program `sdb` (`sdb.exe`) |
| The compiler | Sharp | `compiler` | one file, `sharpc`, that the VM runs |
| The standard library | Sharp | `stdlib` | shipped as source files |
| The GUI's native part | C++ | `stdlib/std/guikit/platform/native` | `sharpgui.so` / `sharpgui.dll` |

- **Only the VM, the debugger and the GUI library differ per system**, so those are built once on
  Linux and once on Windows. The compiler and the library are the same files everywhere.
- **The debugger is the VM**: the same sources, with `debugger` compiled in and the hooks of
  `debugger/hooks.h` switched on (`linux/build_sdb.sh`, `windows/build_sdb.sh`).
  [The sdb guide](../debugger/doc/sdb.md) says how it is used and how it is made.
- **The CLion / IntelliJ plugin** (`tools/clion-plugin`) is built by `windows/build_plugin.sh` and shipped in the
  release's `clion-plugin/` folder, as the zip the IDE installs from disk. It has its own version
  (`pluginVersion` in `tools/clion-plugin/gradle.properties`).
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

There is ONE command for every test, `test.sh` at the top of the repository:

```
bash test.sh                      everything, the quick version (3 minutes)
bash test.sh full                 everything, all of it (more than an hour)
bash test.sh vm                   one part, quick;   bash test.sh vm full   all of it
bash test.sh changed              the parts whose files changed since the last commit
bash test.sh case lib_data_json   one case of the suite
bash test.sh help                 everything it can do;   bash test.sh list   the parts and their times
```

| Part | Covers |
|---|---|
| `compiler` | reading and checking source, the error messages; full: code generation, self-hosting |
| `library` | the standard library compiles; full: every case of the suite, run four ways |
| `vm` | the suite run once, calls to C++; full: the suite four ways, the kind checks, the Windows VM |
| `debugger` | sdb's tests; full: on Windows too |
| `tools` | the command line, the files the editor reads; full: the CLion plugin's tests |
| `programs` | the examples and the benchmarks |

- From Git Bash it runs the Linux half inside WSL and the Windows half itself. From Linux or WSL it
  runs the Linux half.
- It builds first, so there is nothing to run before it.
- The last lines give one result for each part, and for a failed part the command that runs it again.
- A new kind of test is added to this script, so that there stays one way to test.

Underneath, the Linux half is `build-scripts/test.sh`: numbered tests (`build-scripts/test.sh list`),
which `release.sh` also runs.

Tests run side by side: `TEST_GROUPS=<n>` says how many tests at once (default 4, `1` = one after the other) and
`TEST_JOBS=<n>` how many compiles and programs inside each (default: one per core).

`test.sh list` shows every test with its number. The tests themselves are in `tests/suite/`
(the language and the library, one program per case) and `compiler/tests/` (the compiler).

## Release

1. **Pick the version number** - the language version (compiler + library), for example `0.7.1`.
   If the VM changed too, raise its revision by hand first: `runtime/main.h`, `#define rev`
   (`r19` -> `r20`).

2. **Run it**, from Git Bash:

   ```
   bash build-scripts/release.sh 0.7.1 install
   ```

   It writes the number into the sources, builds both systems' halves (the VM, the debugger, the
   GUI library), runs the short tests, packs
   `releases/sharp-0.7.1.zip` and installs it on this PC. It stops at the first step that fails and
   says which.

   Without a number it releases the version the sources already say. Either way, when that
   version's zip already exists it asks `Replace it? [Y/n]` and only a typed `Y` goes on.

3. **Open a new terminal** and try it: `sharp -V`, `sharpc -V`, `sdb -V`.

4. **Commit and push**, the zip included. The script never does that for you.

The zip has an installer for each system (`windows\win-install.cmd`, `linux/unix-install.sh`).
A Raspberry Pi builds its own VM instead: `bash build-scripts/linux/install_from_source.sh`.

## When something goes wrong

- **A script says something is missing.** It names what to run first. `build.sh` is the start of everything.
- **The compiler does not build after an edit.** Nothing was replaced; fix the error and run again.
- **Something is in a strange state.** Delete `~/sharp_work` and run `build.sh`: nothing in that
  folder is precious, and it is all built again.
- **WSL runs out of memory building the VM.** `JOBS=2 bash build-scripts/build.sh`.
