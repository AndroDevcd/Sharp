# sharpc, written in Sharp

This folder is the Sharp compiler written in Sharp ("the bootstrap
compiler"). It compiles the whole language, the standard library and itself,
and writes executables the existing VM (`sharp`) runs. This page says how to
build it and how to use it. How it works inside is in `doc/`:

| document | what it describes |
|---|---|
| `doc/grammar.md` | the grammar the parser reads |
| `doc/backend.md` | the analysis: names, types, every rule and every error code |
| `doc/codegen.md` | code generation, the executable file, the optimizer, self-hosting |

## Building it

A compiler written in its own language needs a compiler to start from. That
is the C++ compiler (`lib/grammar`, the program `sharpc`); from there the
compiler builds itself.

```
SRC   = every .sharp file below compiler, except tests/
LIB   = the standard library folder, stdlib

stage 1   sharpc $SRC -o sharpc_boot -L $LIB                      the C++ compiler builds it
stage 2   sharp sharpc_boot $SRC -o sharpc_self -L $LIB           it builds itself
          sharp sharpc_boot $SRC -o sharpc_self -L $LIB -O        ... optimized: the one to use
```

`sharpc_self` built with `-O` is the compiler to keep: it compiles about a
third faster than the unoptimized one (the numbers are in doc/codegen.md
13.7). From then on the C++ compiler is not needed: `sharp sharpc_self $SRC -o
sharpc_next -L $LIB -O` builds the next compiler from changed sources.

A compiler built by itself must write the same file as the compiler that
built it. `tests/selfhost.sh` checks exactly that (stage 2 compiling the
sources again gives stage 2, byte for byte), and that the self-built compiler
gives the same messages and the same executables as stage 1 for the library,
the tests and the whole suite. It is step 13 of the gates.

## Running it

The compiler is a Sharp executable: it runs on the VM.

```
sharp sharpc_self main.sharp -o app -L /path/to/stdlib
sharp app
```

For real use, start the VM with more stack and memory than its defaults:

```
sharp -istack 1mb -mem 4gb sharpc_self <files and options>
```

- `-istack 1mb`: the stack the compiler itself runs on. With the VM's
  default (64 KB) the compiler built with `-O` follows code nested about 100
  blocks deep (the unoptimized one about 70) and then runs out of stack; with
  1 MB the compiler's own limits apply instead, the same in every build: 256
  levels of nesting in the parser, 150 statements deep in the analysis. Code
  that is nested too deeply is never a crash: it is error E0103 or E0992
  with the place (doc/codegen.md 11.8 has the table).
- `-mem 4gb`: compiling the compiler itself (100,000 lines with the library)
  uses about 650 MB; with the default limit the collector runs more often
  and the same compile takes half as long again.

A wrapper script named `sharpc` with that one line (`exec sharp -istack 1mb
-mem 4gb /path/to/sharpc_self "$@"`) makes it a command.

### The command line

```
sharpc {options} file.sharp ...
```

Options and files may be mixed. An option nobody knows is an error.

| option | what it does |
|---|---|
| `-o <file>` | the executable to write. Without it: `out` |
| `-c` | check only: every error and warning, no file |
| `-run` | run the program after a compile without errors (`sharp` must be on the PATH) |
| `-L <folder>` | a library: every `.sharp` file in the folder and below. May be given several times |
| `-O` (`-O1` .. `-O3`) | optimize. `-O0`: do not (the default). `-n<name>` leaves one optimization out, `-Oreport` prints what was done |
| `-dbg`, `-debug` | keep the source text in the executable: stack traces print the lines |
| `-R`, `-release` | no source text in the executable |
| `-obf` | rename classes, fields and functions in the executable; the map is written to `<output>.names` |
| `-v <version>`, `-target <name>` | the version and platform written into the executable |
| `-w`, `-w<kind>`, `-wextra`, `-wlib`, `-werror` | warnings: none, without one kind, with the quiet ones, with the library's, as errors (`--hw` lists the kinds) |
| `-errlmt <n>` | stop printing after n errors (syntax errors included); a whole number from 1 to 100000 |
| `-a` | accepted and changes nothing: every error is always shown (the C++ compiler hides follow-on errors without it) |
| `-j <n>` | how many threads parse the files |
| `-g`, `-green` | use half the threads |
| `-ignore <file>`, `-ignoredir <folder>` | leave a file or a folder out |
| `-p`, `-P <folder>` | compile a project (below) |
| `--new-project <folder>` | make a new project |
| `--clean`, `-cache <folder>`, `-cacheoff` | the build cache: forget it, keep it elsewhere, do not use it |
| `-eager` | keep every function and class in the executable, also what the program never reaches |
| `-timings` | print how long each stage took |
| `-listing`, `-d` | also write `<output>.listing`: the executable as readable text |
| `-ir`, `-analysis`, `-declarations`, `-parsetree` | print what a stage made of the last file given (`-ir` and `-analysis`: the last file of the program, never a library file `-L` added) |
| `-V`, `-showversion`, `--h`, `-?`, `--hw` | version and help |
| `-sni <folder>` | write the C++ files of the program's `native` functions there: `sni.h`, `<library>_sni.h`, `<library>_sni.cpp` (doc/sni.md). A project needs none: `build/outputs/sni` |
| `-cxx <compiler>` | the C++ compiler that builds a project's native libraries (`"native"` in settings.json; doc/sni.md 5b) |

**The exit code** is 0 when the program compiled, 1 when it has errors or
the command line was refused, and with `-run` whatever the program ended
with: what `main` returned, the n of `exit(n)`, or 1 after an exception
nobody caught (on a runtime built since 2026-11-15; the older one ended
every program with 0). So `sharpc main.sharp -o app && sharp app` does what
it says.

### The standard compiler and the debug compiler

The same sources are built into two kinds of compiler (`configuration/build_kind.sharp`):

- **standard** (`sharpc`, and `sharpc --unoptimized`): prints errors, warnings and what was built for a native
  library - nothing when a compile goes well.
- **debug** (`sharpc --debug`): also prints how the compile went (`parsed ...`, `names resolved ...`,
  `analysis complete`, `code generation: ...`) and has the options that look inside the compiler: `-Oreport`,
  `-Otrace`, `-ir`, `-listing`, `-timings`, `-parsetree`, `-declarations`, `-analysis`, `-eager`. The standard
  compiler refuses those and says to use the debug one.

The compiler built from the repository (`bootc.sh`, the gates) is the debug one; a release makes the two
standard ones by setting `debug_compiler` to false in a copy of the sources.

### Projects

```
sharpc --new-project demo        makes demo/, demo/src/main.sharp, demo/settings.json
sharpc -P demo -L <library>      compiles every .sharp file in demo/ into demo/build/outputs/demo
cd demo && sharpc -p             the same, from inside the project
```

`settings.json` holds the options of the build, so the command line stays
short:

```
{
  "name": "demo",
  "output": "demo",
  "extern_lib": [ "/path/to/stdlib" ],
  "ignore_folders": [ "build" ],
  "optimize_level": 1,
  "warnings_as_errors": false
}
```

Every setting is listed at the top of `configuration/settings_processor.sharp`.
A name that is not a setting is a warning with the nearest names (a misspelt
setting must not silently do nothing); a file that is not JSON stops the
build. A second
build of a project tells you how many files changed since the last one (the
build cache, `build/cache`).

## Testing it

Each script prints one line per failure and a summary.

| script | what it checks |
|---|---|
| `tests/run.sh` | the parser |
| `tests/backend.sh` | the analysis: programs that must be accepted, programs that must give exactly these errors |
| `tests/helps.sh` | the fixes the messages propose compile |
| `tests/codegen.sh` | generated programs run and print what they must, with and without `-O` |
| `tests/suite_runner.sh`, `tests/kindcheck.sh`, `tests/programs.sh` | the language suite, the benchmarks and the examples, built by this compiler |
| `tests/driver.sh` | the command line: folders, threads, the output file, exit codes, `-run`, projects |
| `tests/selfhost.sh` | the compiler compiles itself and the result is the same compiler |
