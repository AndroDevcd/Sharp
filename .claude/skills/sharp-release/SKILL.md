---
name: sharp-release
description: Build, version-bump, package and install the latest Sharp (VM, compiler, standard library) so the user's machine runs what is in the repository. Use when the user asks to "release", "build and install", "package it", "ship the latest", "update my install", "get me the latest changes", or after a batch of edits to runtime, compiler or stdlib that they want to use. Does NOT cover the CLion plugin (tools/clion-plugin) - that is built and installed separately, by hand.
---

# Sharp release: build, bump, package, install

Turns the repository's current state into `releases/sharp-<version>.zip` and puts it on the user's
machine, so `sharp` and `sharpc` in any terminal are the latest.

**Use ONLY the scripts in `build-scripts/`** (`build-scripts/README.md` is the map). The user runs
the same scripts: if one breaks or is missing a step, fix the script - never work around it with a
private helper.

**The CLion plugin** (`tools/clion-plugin`) is built by the release (`windows/build_plugin.sh`) and shipped in the zip's
`clion-plugin/` folder. It has its own version, `pluginVersion` in `tools/clion-plugin/gradle.properties`: bump it by hand
when the plugin changed. It is NOT installed by the release: the user installs the zip in the IDE (Install Plugin from Disk).

## Ground rules

- Do the work yourself, in the main session. Never push. Do not commit unless the user asks.
- The release takes about 25 minutes: run it in the background; do not poll it.
- Do NOT run the full tests (`test.sh full`, about an hour) unless asked. Say what was not covered.
- If a step fails, stop, show the output, and fix the cause. Never package or install a build whose checks failed.
- Report faithfully: which numbers moved and why, what passed, what was not run.

## Shells (this machine)

| where | used for | how |
|---|---|---|
| Git Bash | `release.sh`, everything in `build-scripts/windows/` | `bash build-scripts/release.sh` |
| WSL | `build.sh`, `test.sh`, when run alone | `MSYS_NO_PATHCONV=1 wsl bash -lc 'cd "/mnt/c/Users/Braxton Nunnally/Documents/Github/Sharp" && bash build-scripts/<script>'` |

Pitfalls that have bitten before:
- Without `MSYS_NO_PATHCONV=1`, Git Bash rewrites `/mnt/c/...` paths handed to `wsl`.
- Piping a script into `tail` hides its exit code: read the last line it prints (`TESTS: PASS`, `release compilers: OK`, `RELEASE DONE`).
- Heredocs in Git Bash halve backslashes and break on quotes: write files with the Write tool instead.
- Never set `debug_compiler` to false in the repository: the release script does that in its own copy.
- The compiler is built by its own last good build (the seed, see `build.sh`). A change that
  uses a NEW language feature inside `compiler` needs two builds: the feature first, then its use.

## Steps

### 1. See what changed

`git status --short` and `git diff --stat` (against the last release commit if unsure). Decide:

| changed | number that moves |
|---|---|
| `runtime/**` | the VM's patch number: `4.0.1` -> `4.0.2` (`progvers`). The revision `rN` is only for major VM changes |
| `compiler/**` or `stdlib/**` | the language version: patch + 1 (`0.4.3` -> `0.4.4`) |
| only docs, tests, build scripts, the plugin | nothing: tell the user there is nothing to release |

**The last number never rolls over.** It counts releases and goes as high as it needs to: after
`0.5.9` comes `0.5.10`, then `0.5.11` ... `0.5.38` is a fine version. Do NOT go to `0.6.0` because
the last number reached 9. The same holds for the VM's patch number (`4.0.9` -> `4.0.10`).

The middle number means a BREAKING CHANGE, and it (like the first number: `1.0.0`) moves only when
the user asks for it - never on your own judgment, also when a change does break programs.
If the current version was never installed or handed to the user (it was bumped earlier in this same
piece of work), keep it - do not bump twice for one release.

**Library-only shortcut.** When ONLY `stdlib/**` changed and the user wants it quickly, skip the
build: copy the changed `.sharp` files to the same relative path under `%LOCALAPPDATA%\Sharp\lib\`
(and `~/.sharp/lib/` in WSL if that exists), say which files were copied, and say that the installed
version number did not move. Do the full release when they ask for a release or a version.

### 2. Bump the versions

- VM: `runtime/main.h`, the patch number in `#define progvers "4.0.N" rev`. Leave `#define rev "rN"`
  alone unless the user calls the change major.
- Language: do NOT edit by hand - pass the new number to `release.sh` in step 3 (`release.sh 0.7.1 install`); it writes
  `program_vers` (configuration.sharp), `library_version` and `supported_versions` (options.sharp).
- The standard library is the folder `stdlib`: it has no version in its name and ships with each release.
- If `runtime/sni/sni.h` changed: `python build-scripts/internal/embed_sni_header.py` (the compiler carries a copy).

### 3. Build, test, pack and install: one command (background, about 25 minutes)

From Git Bash, at the repository root:

```
bash build-scripts/release.sh <new version> install 2>&1 | grep -v "ld.exe\|^time:\|^unknown" | cut -c1-200 | tail -70
```

Leave the number out only when the version in the sources was never released (step 1).

**The overwrite question.** When `releases/sharp-<version>.zip` already exists the script asks
`Replace it? [Y/n]` and stops without a typed Y - so a background run stops right there
(`no answer: nothing was built or changed`). That question is THE USER'S: ask them in the chat
whether to replace that zip, and only after their yes in that turn run it as
`echo Y | bash build-scripts/release.sh ...`. Never pipe the Y on your own judgment.

Ten steps, each a script that also runs alone (the top of `release.sh` lists them): the Linux VM
and the compiler, the short tests (which build and test the Linux sdb), the three release compilers,
the Linux GUI library, the Linux sdb, the Windows VM, the Windows GUI library, the Windows sdb, the CLion plugin, the zip. It stops at the first failure with
`RELEASE STOPPED at step N (<script>)`; fix the cause and run it again.

It must show, in order: `built: Sharp X (VM Y)`, `TESTS: PASS (tests 1 2 4 5 12 14 15 16)`,
`release compilers: OK`, both `built: ...sharpgui...` lines, `release: .../sharp-X.zip`, `installed: sharp <VM version> ..., Sharp X`, `the GUI library is installed`, `the debugger is installed: sdb ...`,
`built: ...sharp-clion-plugin-<version>.zip`,
`RELEASE DONE`, and no `WARNING: no ...` line.

Extra tests by what changed: ALWAYS through the repository's own `test.sh` (Git Bash, at the root), never a private helper -
`bash test.sh changed` (the parts the changed files need), `bash test.sh <part> full`, `bash test.sh suite <group>` (a part of the suite's one program, e.g. `suite.library.collections`).
If it cannot run something, add that to `test.sh`. Say when `full` was not run.

### 4. Install

**Windows.** `release.sh install` already did it. Alone, from Git Bash:

```
bash build-scripts/windows/install_release.sh
```

It unpacks the newest zip and runs the release's own installer, which proves itself by compiling and
running the hello example. It must end `installed: sharp <VM version> ..., Sharp <version>` and
`the GUI library is installed`.

**Linux / WSL (when the user asks for Linux, or `~/.sharp` already exists in WSL).** In WSL: extract the zip
(`python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall('.')" <zip>`),
`chmod +x sharp-<version>/linux/*`, then `bash sharp-<version>/linux/unix-install.sh`.
Check with `~/.sharp/bin/sharp -V` and `~/.sharp/bin/sharpc -V`.

### 5. Record and report

- Update the memory file `sharp-versioning.md` (and its line in `MEMORY.md`) with the new numbers.
- Tell the user, in a few lines:
  - the versions before and after, and what change moved each;
  - the zip: `releases/sharp-<version>.zip`;
  - what is installed now, and "open a new terminal";
  - what was tested (the short tests, the release compilers, the installer's own test) and that the full tests were not run;
  - that nothing was committed, unless they asked for a commit.
- Older zips stay in `releases/` unless the user says to remove them.
