---
name: sharp-release
description: Build, version-bump, package and install the latest Sharp (VM, compiler, standard library) so the user's machine runs what is in the repository. Use when the user asks to "release", "build and install", "package it", "ship the latest", "update my install", "get me the latest changes", or after a batch of edits to lib/runtime, lib/compiler or lib/support that they want to use. Does NOT cover the CLion plugin (tools/clion-plugin) - that is built and installed separately, by hand.
---

# Sharp release: build, bump, package, install

Turns the repository's current state into `releases/sharp-<version>.zip` and puts it on the user's
machine, so `sharp` and `sharpc` in any terminal are the latest.

**Not part of this skill:** the CLion plugin. Do not build, bump or mention installing it here.

## Ground rules

- Do the work yourself, in the main session. Never push. Do not commit unless the user asks.
- Long steps (the release compilers, about 5 minutes) run in the background; do not poll them.
- Do NOT run the full gates (about an hour). Use only the short checks below, and say what was not covered.
- If a step fails, stop, show the output, and fix the cause. Never package or install a build whose checks failed.
- Report faithfully: which numbers moved and why, what passed, what was not run.

## The three shells (this machine)

| where | used for | how |
|---|---|---|
| WSL | Linux VM, the compiler's bootstrap, release compilers, packaging | `MSYS_NO_PATHCONV=1 wsl bash -lc '...'` from Git Bash |
| Git Bash | the Windows VM | `bash build-scripts/build_windows_vm.sh` |
| PowerShell | the Windows installer | `& "<path>\win-install.cmd" /quiet` |

Pitfalls that have bitten before:
- Without `MSYS_NO_PATHCONV=1`, Git Bash rewrites `/mnt/c/...` paths handed to `wsl`.
- `wsl bash -lc '... $var ...'` loses variables set inside it: for anything with variables, write a script file with the Write tool and run `wsl bash "/mnt/c/.../script.sh"`.
- Heredocs in Git Bash halve backslashes and break on quotes: write files with the Write tool instead.
- WSL has no `unzip`: extract with `python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall('.')" <zip>`.
- `cmd //c win-install.cmd` from Git Bash does not find the file: run the installer from PowerShell.
- Never set `debug_compiler` to false in the repository: the release script does that in its own copy.
- The compiler is bootstrapped by a frozen C++ compiler that segfaults on `new file(x).method()` written inline (put the file in a local first) and treats `native` and `base` as keywords (not usable as variable names).

## Steps

### 1. See what changed

`git status --short` and `git diff --stat` (against the last release commit if unsure). Decide:

| changed | number that moves |
|---|---|
| `lib/runtime/**`, `lib/core/**`, `main.cpp` | the VM revision: `rN` -> `rN+1` |
| `lib/compiler/**` or `lib/support/**` | the language version: patch + 1 (`0.4.3` -> `0.4.4`) |
| only docs, tests, build scripts, the plugin | nothing: tell the user there is nothing to release |

A minor or major language bump (`0.5.0`, `1.0.0`) only when the user asks for it.
If the current version was never installed or handed to the user (it was bumped earlier in this same
piece of work), keep it - do not bump twice for one release.

**Library-only shortcut.** When ONLY `lib/support/**` changed and the user wants it quickly, skip the
build: copy the changed `.sharp` files to the same relative path under `%LOCALAPPDATA%\Sharp\lib\`
(and `~/.sharp/lib/` in WSL if that exists), say which files were copied, and say that the installed
version number did not move. Do the full release when they ask for a release or a version.

### 2. Bump the versions

- VM: `lib/runtime/main.h`, `#define rev "rN"`.
- Language - all three, kept in step:
  - `lib/compiler/configuration/configuration.sharp`: `program_vers : string = "X"`
  - `lib/compiler/configuration/options.sharp`: `static library_version : string = "X"`
  - `lib/compiler/configuration/options.sharp`: add `"X"` to the end of `supported_versions`
- The library folder stays `lib/support/0.4.0` whatever the version (the release script takes the folder that is there).
- If `lib/runtime/sni/sni.h` changed: `python build-scripts/embed_sni_header.py` (the compiler carries a copy).

### 3. Build the VMs

```
MSYS_NO_PATHCONV=1 wsl bash -lc 'SHARP_OUT=$HOME/sharp_work/sharp_dev bash ~/sharp_work/tools/build_sharp.sh 2>&1 | grep FAILED -A5; ~/sharp_work/sharp_dev/sharp -V'
bash build-scripts/build_windows_vm.sh 2>&1 | grep -v "ld.exe" | tail -2 && cmake-build-windows-vm/sharp.exe -V
```

Both must print the new VM version. (A changed header rebuilds everything: a few minutes.)

### 4. Build the compiler and run the short checks

```
MSYS_NO_PATHCONV=1 wsl bash -lc 'bash ~/sharp_work/tools/gates.sh only 1 2 4 5 12 14 15 2>&1 | grep -v "^time:\|^unknown" | cut -c1-200 | tail -18'
```

Step 1 is the build (`errors: 0 ; built: 1`); then library, frontend, backend, driver, ide and the native
interface. A few minutes: run it in the background. It must end `GATES: PARTIAL PASS` with no `FAIL` line.
Add step 11 (the repository's programs) when code generation or the optimizer changed.

### 5. Build the three release compilers (background, about 8 minutes)

```
MSYS_NO_PATHCONV=1 wsl bash -lc 'cd "/mnt/c/Users/Braxton Nunnally/Documents/Github/Sharp" && bash build-scripts/build_release_compilers.sh 2>&1 | tail -5'
```

It builds `sharpc-v<version>` (standard, with -O), `sharpc-unoptimized-v<version>` (standard, without) and
`sharpc-debug-v<version>` (the debug compiler: `sharpc --debug`). The two standard ones are the sources with
`debug_compiler` set to false in a copy (`lib/compiler/configuration/build_kind.sharp`): they print nothing
on a good compile and refuse the options that look inside the compiler. In the repository the flag stays
TRUE - the compiler `bootc.sh` and the gates build is the debug one, and the test scripts read its lines.

Must end `release compilers: OK`: the two standard compilers agree with each other, are quiet, and the
debug one talks (`standard / debug: 0 problem(s)`).

### 6. Package

```
MSYS_NO_PATHCONV=1 wsl bash -lc 'cd "/mnt/c/Users/Braxton Nunnally/Documents/Github/Sharp" && bash build-scripts/make_release.sh 2>&1 | tail -2'
```

Prints `release: .../releases/sharp-<version>.zip   (language X, VM 4.0.0rN)`. It refuses to package when
the VM or the compiler does not say the version in the sources - rebuild the one it names.

### 7. Install

**Windows (always).** Extract the zip into the job's temp folder, then from PowerShell:

```
& "<temp>\sharp-<version>\windows\win-install.cmd" /quiet
& "$env:LOCALAPPDATA\Sharp\bin\sharp.exe" -V
Get-Content "$env:LOCALAPPDATA\Sharp\VERSION.txt" -TotalCount 4
```

The installer replaces `%LOCALAPPDATA%\Sharp` (bin, compiler, lib, examples) and proves itself by compiling
and running the hello example. It must print `Sharp <version> is installed.`

**Linux / WSL (when the user asks for Linux, or `~/.sharp` already exists in WSL).** In WSL: extract the zip
(python, see pitfalls), `chmod +x sharp-<version>/linux/*`, then `bash sharp-<version>/linux/unix-install.sh`.
It installs into `~/.sharp` and adds `~/.sharp/bin` to the PATH in the shell's start file. Check with
`~/.sharp/bin/sharp -V` and `~/.sharp/bin/sharpc -V`.

### 8. Record and report

- Update the memory file `sharp-versioning.md` (and its line in `MEMORY.md`) with the new numbers.
- Tell the user, in a few lines:
  - the versions before and after, and what change moved each;
  - the zip: `releases/sharp-<version>.zip`;
  - what is installed now (`sharp -V`, `VERSION.txt`), and "open a new terminal";
  - what was checked (the short checks, the release compilers, the installer's own test) and that the full gates were not run;
  - that nothing was committed, unless they asked for a commit.
- Older zips stay in `releases/` unless the user says to remove them.
