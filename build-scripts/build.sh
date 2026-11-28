#!/bin/bash
# BUILD SHARP from the repository as it is now.   (Linux / WSL)
#
#     bash build-scripts/build.sh               the VM, then the compiler
#     bash build-scripts/build.sh compiler      only the compiler - 10 seconds. Enough after changing
#                                               only Sharp files (compiler or the standard library).
#     bash build-scripts/build.sh install       the VM and the compiler, then INSTALL them on this machine
#                                               (see below; this one also runs from Git Bash on Windows)
#     bash build-scripts/build.sh install library     only the STANDARD LIBRARY of the repository, into the
#                                               Sharp installed here - seconds, nothing is built (below)
#     bash build-scripts/build.sh 0.7.1         first write this LANGUAGE VERSION into the sources, then build
#     bash build-scripts/build.sh install 0.7.1                                       ... then build and install
#
# THE VERSION NUMBERS MOVE DURING DEVELOPMENT, not only at a release: when the compiler or the
# standard library changed, give the next number here (it is written into the three places that
# hold it and is part of what you commit); when the VM changed, change runtime/main.h by hand.
# A release then takes the number the sources already say (release.sh without a number).
#
# INSTALL: what is in the repository NOW, on this machine, as a DEVELOPMENT BUILD - for the time
# between releases. No zip, no new version number, nothing in releases/ (a release is release.sh).
#
#   - the VM is built here: on Linux, in WSL, on a Raspberry Pi; from Git Bash, the Windows VM
#   - the compiler is built from the sources as they are (see THE SEED below for what compiles it)
#   - the standard library is the repository's
#   - sdb, the debugger, and the GUI library are built too where they can be (said when not)
#   - it goes where a release goes - ~/.sharp, or %LOCALAPPDATA%\Sharp - and takes its place:
#     `sharp` and `sharpc` in a new terminal are then this build. `sharpc -V` says
#     "<version>-dev", and VERSION.txt in the install folder says the commit it was built from.
#     To go back to a release, install the release again.
#   - nothing is tested first: run  bash test.sh  yourself when you want to know
#
# INSTALL LIBRARY: for a change that touched only the standard library. The repository's stdlib
# takes the place of the installed one (~/.sharp/lib, or %LOCALAPPDATA%\Sharp\lib); the installed VM
# and compiler stay as they are. It runs anywhere - Linux, a Pi, Git Bash - and needs no WSL.
#
#   - it REFUSES when the VM or the compiler changed since what is installed was built (different
#     version numbers, or changes in runtime/ or compiler/ since the commit VERSION.txt names): a
#     library may need what an older VM or compiler has not got. Then: build.sh install
#   - it proves itself the way an installer does - the installed compiler builds the hello example
#     with the new library, and it runs - and puts the library that was there BACK when that fails
#   - VERSION.txt gets a line saying which commit the library is from
#
# WHAT IT BUILDS, in the work folder ~/sharp_work (internal/common.sh says why there):
#
#     ~/sharp_work/sharp_dev/sharp           the VM
#     ~/sharp_work/out/boot/sharpc_boot      the compiler
#     ~/sharp_work/stdlib                    a copy of the standard library
#
# Compile and run a program with them:
#
#     ~/sharp_work/sharp_dev/sharp ~/sharp_work/out/boot/sharpc_boot main.sharp -o app -L ~/sharp_work/stdlib
#     ~/sharp_work/sharp_dev/sharp app
#
# The first run compiles every C++ file of the VM (several minutes). After that only what changed
# is redone (seconds).
#
#     JOBS=<n>   C++ files compiled at once (default: by cores and memory)
#
# THE COMPILER COMPILES ITSELF. It is a Sharp program, so a Sharp compiler has to compile it: the
# last good build of itself, called THE SEED.
#
#     ~/sharp_work/out/seed/sharpc_boot             the compiler that compiles the next one
#     ~/sharp_work/out/seed/sharpc_boot.previous    the seed before it (to go back to by hand)
#
#   - After a build with NO errors, the new compiler becomes the seed.
#   - A build WITH errors changes nothing: fix the error and run again.
#   - No seed yet (a fresh machine, or the work folder was deleted): the first seed is the compiler
#     of the release in releases/ that has the sources' version; else the compiler of the Sharp
#     installed on this machine (~/.sharp); else the compiler of the newest release in releases/.
#     That folder is not in the repository: on a machine with neither, the newest release's zip is
#     downloaded into it from the project's GitHub Releases page (and it says so).
#   - A change that USES a new language feature inside the compiler's own source needs two builds:
#     first add the feature (the seed can compile that), then use it.
#
# The Windows VM is built on Windows (windows/build_vm.sh); a Raspberry Pi builds and installs with
# linux/install_from_source.sh.
#
# The compiler step prints  errors: N ; built: 1  and the first errors. Exits 1 when anything failed.

# ---- the words of the command line: what to do, and perhaps a version number
ACTION=""; NEW_VERSION=""; LIBRARY_ONLY=0
for word in "$@"; do
    case "$word" in
        compiler|install|stage) ACTION=$word ;;
        library)                LIBRARY_ONLY=1 ;;
        [0-9]*.[0-9]*.[0-9]*)   NEW_VERSION=$word ;;
        *) echo "build.sh: unknown argument '$word' (nothing, or: compiler, install, install library - and a version like 0.7.1)"; exit 2 ;;
    esac
done

# ---- `install library`: only the standard library, into the Sharp that is installed (INSTALL LIBRARY, above)
if [ $LIBRARY_ONLY = 1 ]; then
    stop() { echo "$*"; echo "NOTHING WAS CHANGED."; exit 1; }
    [ "$ACTION" = install ] && [ -z "$NEW_VERSION" ] || { echo "build.sh: 'library' goes with install and nothing else:  bash build-scripts/build.sh install library"; exit 2; }
    REPO=$(cd "$(dirname "$0")/.." && pwd)
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*) INSTALLED="$LOCALAPPDATA/Sharp"; sharp() { "$INSTALLED/bin/sharp.exe" "$@"; }
                              sharpc() { cmd.exe //c "$(cygpath -w "$INSTALLED/bin/sharpc.cmd")" "$@" | tr -d '\r'; return "${PIPESTATUS[0]}"; } ;;
        *)                    INSTALLED="${SHARP_HOME:-$HOME/.sharp}"; sharp() { "$INSTALLED/bin/sharp" "$@"; }; sharpc() { "$INSTALLED/bin/sharpc" "$@"; } ;;
    esac
    [ -d "$INSTALLED/lib" ] && [ -d "$INSTALLED/compiler" ] || stop "no Sharp is installed in $INSTALLED: there is nothing to put a library into. Install first:  bash build-scripts/build.sh install"
    [ -d "$REPO/stdlib/std" ] && [ -d "$REPO/stdlib/platform" ] || stop "no standard library at $REPO/stdlib"

    # the same VM and compiler as the sources? first by their numbers ...
    vm_here=$(sharp -V 2>&1 | head -1 | awk '{print $2}')
    compiler_here=$(sharpc -V 2>&1 | head -1 | awk '{print $2}')
    vm_sources=$(grep -o '#define progvers "[^"]*"' "$REPO/runtime/main.h" | cut -d'"' -f2)$(grep -o '#define rev "[^"]*"' "$REPO/runtime/main.h" | cut -d'"' -f2)
    compiler_sources=$(grep -o 'program_vers : string = "[^"]*"' "$REPO/compiler/configuration/configuration.sharp" | cut -d'"' -f2)
    if [ "$vm_here" != "$vm_sources" ] || [ "${compiler_here%-dev}" != "$compiler_sources" ]; then
        stop "installed: VM $vm_here, compiler $compiler_here - the sources: VM $vm_sources, compiler $compiler_sources." \
             "The library of the sources may need what the installed ones have not got. Install all of it:  bash build-scripts/build.sh install"
    fi
    # ... then by what changed since the commit the installed build names (VERSION.txt of a development build)
    built_from=$(grep -o 'commit [0-9a-f]*' "$INSTALLED/VERSION.txt" 2>/dev/null | head -1 | cut -d' ' -f2)
    if [ -n "$built_from" ] && git -C "$REPO" cat-file -e "$built_from^{commit}" 2>/dev/null; then
        changed=$(git -C "$REPO" -c core.fileMode=false -c core.autocrlf=true diff --name-only "$built_from" -- runtime compiler ':!compiler/tests' 2>/dev/null)
        if [ -n "$changed" ]; then
            echo "THE VM OR THE COMPILER CHANGED since what is installed was built (commit $built_from):"
            echo "$changed" | head -8 | sed 's/^/    /'; [ "$(echo "$changed" | wc -l)" -gt 8 ] && echo "    ... and $(( $(echo "$changed" | wc -l) - 8 )) more"
            stop "A library alone is not enough then. Install all of it:  bash build-scripts/build.sh install"
        fi
        grep -q "commit $built_from + changes not committed" "$INSTALLED/VERSION.txt" && \
            echo "note: what is installed was built from commit $built_from AND changes that were not committed then; those cannot be compared."
    else
        echo "note: the installed Sharp does not name a commit of this repository (a release?): only the version numbers were compared."
    fi

    # the new library beside the old one, then in its place (Unix line endings, as an installer's:
    # internal/pack_release.sh says why); the old one is kept until the new one has proved itself
    rm -rf "$INSTALLED/lib.new" "$INSTALLED/lib.previous"
    cp -r "$REPO/stdlib" "$INSTALLED/lib.new" || stop "could not write $INSTALLED/lib.new"
    find "$INSTALLED/lib.new" -type f \( -name '*.sharp' -o -name '*.cpp' -o -name '*.h' \) -exec sed -i 's/\r$//' {} +
    mv "$INSTALLED/lib" "$INSTALLED/lib.previous" && mv "$INSTALLED/lib.new" "$INSTALLED/lib" || {
        [ -d "$INSTALLED/lib" ] || mv "$INSTALLED/lib.previous" "$INSTALLED/lib"
        rm -rf "$INSTALLED/lib.new"; stop "could not put the library in place (is a Sharp program or an editor holding $INSTALLED/lib open?)"; }

    # the proof: the installed compiler builds the hello example with it, and it runs
    PROOF=$(mktemp -d)
    if (cd "$PROOF" && sharpc "$(cd "$INSTALLED/examples" && pwd)/hello.sharp" -o hello > compile.txt 2>&1 && sharp hello > run.txt 2>&1 && grep -q "Hello from Sharp" run.txt); then
        rm -rf "$PROOF" "$INSTALLED/lib.previous"
    else
        grep -a -A6 "^error" "$PROOF/compile.txt" | cut -c1-190 | head -20; tail -3 "$PROOF/run.txt" 2>/dev/null | cut -c1-190
        rm -rf "$INSTALLED/lib"; mv "$INSTALLED/lib.previous" "$INSTALLED/lib"; rm -rf "$PROOF"
        echo "THE NEW LIBRARY DOES NOT WORK with the installed compiler and VM (above): the library that was there is back."
        echo "Install all of it:  bash build-scripts/build.sh install"; exit 1
    fi

    commit=$(git -C "$REPO" rev-parse --short HEAD 2>/dev/null)
    [ -n "$(git -C "$REPO" -c core.fileMode=false -c core.autocrlf=true status --porcelain -- stdlib 2>/dev/null | head -1)" ] && commit="$commit + changes not committed yet"
    if [ -f "$INSTALLED/VERSION.txt" ]; then
        sed -i '/^standard library alone:/d' "$INSTALLED/VERSION.txt"
        printf 'standard library alone: %s, commit %s\r\n' "$(date -u +%Y-%m-%d)" "$commit" >> "$INSTALLED/VERSION.txt"
    fi
    echo "installed, the standard library alone: $(find "$INSTALLED/lib" -name '*.sharp' | wc -l) files from commit $commit, into $INSTALLED/lib"
    echo "(the VM $vm_here and the compiler $compiler_here are as they were; no new terminal is needed)"
    exit 0
fi

# ---- `install` from Git Bash, on Windows: the compiler and the library are made ready inside WSL
# (`stage`, below), the Windows VM is built here, and the release's own Windows installer puts it in place
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*)
    HERE=$(cd "$(dirname "$0")" && pwd); REPO=$(cd "$HERE/.." && pwd)
    [ "$ACTION" = install ] || { echo "build.sh builds on Linux or inside WSL. From Git Bash only this works:  bash build-scripts/build.sh install"; exit 2; }
    # the tools, checked before anything is built (what WSL itself is missing, `stage` says at its start)
    missing=""
    command -v wsl > /dev/null 2>&1 || missing="$missing    WSL (the compiler is built inside it)    get it with:  wsl --install   then, inside it:  sudo apt install g++ rsync python3\n"
    ( source "$HERE/windows/mingw.sh" ) > /dev/null 2>&1 || missing="$missing    a MinGW g++ (it builds sharp.exe)    get it with:  CLion (it ships one), or any MinGW-w64 and  MINGW=<the folder with g++.exe>\n"
    if [ -n "$missing" ]; then echo "THIS PC IS MISSING what the install needs:"; printf "$missing"; echo "Nothing was built. Install the above and run this again."; exit 1; fi
    IN_WSL=$(MSYS_NO_PATHCONV=1 wsl wslpath -a "$(cygpath -m "$HERE")" 2>/dev/null | tr -d '\r')
    [ -n "$IN_WSL" ] || { echo "WSL is there but did not answer (no Linux installed in it yet?):  wsl --install   then, inside it:  sudo apt install g++ rsync python3"; exit 1; }
    MSYS_NO_PATHCONV=1 wsl bash "$IN_WSL/build.sh" stage $NEW_VERSION < /dev/null || exit 1
    D=$(ls -d "$REPO"/build/install-dev/sharp-* 2>/dev/null | head -1)
    [ -d "$D/shared/compiler" ] || { echo "nothing was made ready in build/install-dev"; exit 1; }
    quiet() { grep -v "ld.exe\|^time:\|^unknown"; }
    bash "$HERE/windows/build_vm.sh" 2>&1 | quiet | tail -3
    [ -x "$REPO/cmake-build-windows-vm/sharp.exe" ] || { echo "the Windows VM did not build"; exit 1; }
    cp "$REPO/cmake-build-windows-vm/sharp.exe" "$D/windows/sharp.exe"
    # the debugger and the GUI library: everything else works without them, so a failure is said and not fatal
    bash "$HERE/windows/build_sdb.sh" 2>&1 | quiet | tail -1
    if [ -x "$REPO/cmake-build-windows-sdb/sdb.exe" ]; then cp "$REPO/cmake-build-windows-sdb/sdb.exe" "$D/windows/sdb.exe"
    else echo "NOTE: sdb, the debugger, did not build - everything but debugging works"; fi
    rm -f "$REPO/build/gui/sharpgui.dll"
    bash "$HERE/windows/build_gui_library.sh" 2>&1 | quiet | tail -2
    if [ -f "$REPO/build/gui/sharpgui.dll" ]; then cp "$REPO/build/gui/sharpgui.dll" "$D/windows/sharpgui.dll"
    else echo "NOTE: the GUI library did not build - everything but GUI programs works (windows/build_gui_library.sh says what it needs)"; fi
    # cmd.exe wants a Windows path, and //c is how Git Bash passes /c through
    cmd.exe //c "$(cygpath -w "$D/windows/win-install.cmd")" //quiet | tail -6
    [ "${PIPESTATUS[0]}" = 0 ] || { echo "NOT INSTALLED: the installer stopped (its message is above)"; exit 1; }
    INSTALLED="$LOCALAPPDATA/Sharp"
    echo ""
    echo "installed, a development build: $("$INSTALLED/bin/sharp.exe" -V 2>&1 | head -1), $(cmd.exe //c "$(cygpath -w "$INSTALLED/bin/sharpc.cmd")" -V 2>&1 | head -1 | tr -d '\r')"
    echo "($(sed -n '6p' "$INSTALLED/VERSION.txt" | tr -d '\r' | sed 's/  */ /g'); open a NEW terminal to use it)"
    exit 0 ;;
esac

source "$(dirname "$0")/internal/common.sh"
if [ -n "$NEW_VERSION" ]; then set_language_version "$NEW_VERSION" || exit 1; fi

# ---- the tools, checked before anything is built: everything that is missing is said at once
tools=( "rsync:sudo apt install rsync" "awk:sudo apt install gawk" )
[ "$ACTION" = compiler ] || tools+=( "${CXX:-g++}:sudo apt install g++   (C++17: g++ 8 or newer)" )
# (a first seed out of a release's zip - see THE SEED above - is unpacked with python3)
[ -f "$SEED" ] || ls "${SHARP_HOME:-$HOME/.sharp}"/compiler/sharpc-v* > /dev/null 2>&1 || tools+=( "python3:sudo apt install python3" )
need_tools "${tools[@]}"
copy_sources || exit 1

# ---- the VM
if [ "$ACTION" != compiler ]; then
    # WSL's usual 3 GB is not enough for the 6 compiles at once that build_vm.sh would pick from the
    # memory alone (-O3 on the VM's largest files takes over 500 MB each): 3 at a time below 4 GB
    if [ -z "$JOBS" ] && [ "$(awk '/MemTotal/ {print int($2/1024)}' /proc/meminfo)" -lt 4000 ]; then export JOBS=3; fi
    # -O3: what the released VM has always been built with (linux/build_vm.sh alone uses -O2, for small machines)
    SOURCES="$WORK/src" OUT="$WORK/sharp_dev" bash "$REPO/build-scripts/linux/build_vm.sh" -O3 || exit 1
    [ "$("$VM" -V 2>&1 | head -1 | awk '{print $2}')" = "$VM_VERSION" ] || fail "the VM says '$("$VM" -V 2>&1 | head -1)', the sources say $VM_VERSION"
fi
[ -x "$VM" ] || fail "no VM at $VM yet: run  bash build-scripts/build.sh  without 'compiler' once"

# ---- the compiler
if [ ! -f "$SEED" ]; then
    # the release of the version the sources say - not simply the highest number: an abandoned higher
    # number (0.7.0, when the sources went back to the 0.4 line) must not win over the current one
    ZIP="$REPO/releases/sharp-$LANGUAGE_VERSION.zip"
    # ... else the compiler of the Sharp installed here (a machine that pulled the repository has no releases/):
    # sharpc-v<version>, the standard one (the glob does not fit the -unoptimized and -debug ones)
    INSTALLED_COMPILER=$(ls "${SHARP_HOME:-$HOME/.sharp}"/compiler/sharpc-v* 2>/dev/null | sort -V | tail -1)
    mkdir -p "$(dirname "$SEED")"
    if [ ! -f "$ZIP" ] && [ -n "$INSTALLED_COMPILER" ]; then
        cp "$INSTALLED_COMPILER" "$SEED"
        echo "first seed: the installed compiler ($INSTALLED_COMPILER)"
    else
        [ -f "$ZIP" ] || ZIP=$(ls "$REPO"/releases/sharp-*.zip 2>/dev/null | sort -V | tail -1)
        # nothing on this machine at all (a fresh clone): the newest release is downloaded (internal/common.sh)
        [ -n "$ZIP" ] || ZIP=$(fetch_release) || exit 1
        python3 - "$ZIP" "$SEED" <<'EOF' || fail "could not take the compiler out of the release (python3 is needed)"
import sys, zipfile
z = zipfile.ZipFile(sys.argv[1])
# shared/compiler/sharpc-v<version>: the standard compiler (not the -unoptimized or -debug one)
name = [n for n in z.namelist() if "/shared/compiler/sharpc-v" in n][0]
open(sys.argv[2], "wb").write(z.read(name))
EOF
        echo "first seed: the compiler of $(basename "$ZIP")"
    fi
fi
# THE MEMORY THE COMPILER GETS. Compiling itself, it takes about 450 MB of the VM's memory (measured
# on x64 and on 32-bit ARM). The VM's own ceiling is a quarter of the machine's memory: too little on
# a small machine - a Pi with 1 GB stopped at 235 MB. Below 2.4 GB the VM is told its ceiling here:
# three quarters of what the machine has.
MEMORY=()
MACHINE_MB=$(awk '/MemTotal/ {print int($2/1024)}' /proc/meminfo)
[ "$MACHINE_MB" -lt 2400 ] && MEMORY=(-mem "$((MACHINE_MB * 3 / 4))mb")
# a compile that ran out of memory says so in the VM's words only ("managed memory: ... kb of ... kb")
out_of_memory() {
    grep -aq "^managed memory:" "$1" || return 0
    echo "THE COMPILER RAN OUT OF MEMORY: $(grep -a "^managed memory:" "$1" | head -1)"
    echo "    this machine has $MACHINE_MB MB; compiling the compiler takes about 450 MB of it."
    echo "    Close other programs or add swap, and run this again."
}
OUT="$WORK/out/boot"; mkdir -p "$OUT"
cd "$WORK/src/compiler" || exit 1
rm -f "$COMPILER"
# every .sharp file of the compiler except its tests; -L: with the standard library
"$VM" "${MEMORY[@]}" "$SEED" $(find . -name "*.sharp" -not -path "./tests/*" | sort) -j 1 -o "$COMPILER" -L "$STDLIB" > "$OUT/c.txt" 2>&1
code=$?
errors=$(grep -c '^error' "$OUT/c.txt")
built=0; [ -f "$COMPILER" ] && built=1
echo "exit $code ; errors: $errors ; built: $built   (the compiler: $COMPILER; whole output: $OUT/c.txt)"
grep -A6 "^error" "$OUT/c.txt" | cut -c1-190 | head -45
if [ "$errors" != 0 ] || [ "$built" != 1 ]; then
    [ "$errors" = 0 ] && tail -5 "$OUT/c.txt" | cut -c1-190       # no error line and no file: show how it ended
    out_of_memory "$OUT/c.txt"
    rm -f "$COMPILER"
    exit 1
fi
cp -f "$SEED" "$SEED.previous"
cp -f "$COMPILER" "$SEED"
echo "built: Sharp $LANGUAGE_VERSION (VM $VM_VERSION)"
[ "$ACTION" = install ] || [ "$ACTION" = stage ] || exit 0

# ---- `install`: what was just built, laid out as a release is (internal/pack_release.sh) in
# build/install-dev/, and put in place by the release's own installer.
# (`stage` is the part of it the Windows half asks WSL for: everything but the last step.)
V=$LANGUAGE_VERSION
STAGE="$REPO/build/install-dev"; D="$STAGE/sharp-$V"
T="$REPO/build-scripts/internal/zip-contents"
rm -rf "$STAGE"; mkdir -p "$D/linux" "$D/windows" "$D/shared/compiler" "$D/shared/examples"

# the compiler that is installed: the sources with `debug_compiler` false, as a release's
# (internal/release_compilers.sh says why; the change is made in a copy), and "-dev" behind its
# version. Compiled WITHOUT -O: seconds instead of minutes - a release's compiler is the faster one.
MADE="$WORK/out/install_dev"; STANDARD="$MADE/sources"; mkdir -p "$STANDARD"
rsync -a --delete --exclude tests "$WORK/src/compiler/" "$STANDARD/"
sed -i 's/^debug_compiler : decimal = true;/debug_compiler : decimal = false;/' "$STANDARD/configuration/build_kind.sharp"
sed -i "s/^program_vers : string = \"[^\"]*\";/program_vers : string = \"$V-dev\";/" "$STANDARD/configuration/configuration.sharp"
grep -q '^debug_compiler : decimal = false;' "$STANDARD/configuration/build_kind.sharp" || fail "build_kind.sharp: the debug_compiler line was not found"
grep -q "^program_vers : string = \"$V-dev\";" "$STANDARD/configuration/configuration.sharp" || fail "configuration.sharp: the program_vers line was not found"
rm -f "$MADE/sharpc"
# (from inside its own folder: the name given to -o is written into the file)
(cd "$MADE" && "$VM" "${MEMORY[@]}" "$COMPILER" $(find "$STANDARD" -name "*.sharp" | sort) -o sharpc -L "$STDLIB" -j 1 > compile.txt 2>&1)
if [ ! -f "$MADE/sharpc" ]; then grep -a -A6 "^error" "$MADE/compile.txt" | cut -c1-190 | head -30; out_of_memory "$MADE/compile.txt"; fail "the compiler to install did not build (whole output: $MADE/compile.txt)"; fi
said=$("$VM" "$MADE/sharpc" -V 2>&1 | head -1)
[ "$said" = "sharpc $V-dev" ] || fail "the compiler to install says '$said', not 'sharpc $V-dev'"
# the three names a release has, so its `sharpc` starter works as it is: there is one compiler here,
# and the debug one is the compiler this script built first
cp "$MADE/sharpc" "$D/shared/compiler/sharpc-v$V"
cp "$MADE/sharpc" "$D/shared/compiler/sharpc-unoptimized-v$V"
cp "$COMPILER" "$D/shared/compiler/sharpc-debug-v$V"

# the library (Unix line endings: internal/pack_release.sh says why), the VM
cp -r "$LIBRARY_SOURCE" "$D/shared/lib"
find "$D/shared/lib" -type f \( -name '*.sharp' -o -name '*.cpp' -o -name '*.h' \) -exec sed -i 's/\r$//' {} +
cp "$VM" "$D/linux/sharp"

# the debugger and the GUI library: everything else works without them, so a failure is said and not fatal
if [ "$ACTION" = install ]; then
    if bash "$REPO/build-scripts/internal/build_sdb.sh" > "$MADE/sdb.txt" 2>&1 && [ -x "$WORK/sdb_dev/sdb" ]; then cp "$WORK/sdb_dev/sdb" "$D/linux/sdb"
    else tail -3 "$MADE/sdb.txt"; echo "NOTE: sdb, the debugger, did not build - everything but debugging works"; fi
fi
# (also for `stage`: it writes the bridge the Windows library is built from, build/gui/glue)
if command -v sdl2-config > /dev/null; then
    rm -f "$WORK/out/gui_lib/sharpgui.so"
    bash "$REPO/build-scripts/linux/build_gui_library.sh" > "$MADE/gui.txt" 2>&1
    if [ -f "$WORK/out/gui_lib/sharpgui.so" ]; then cp "$WORK/out/gui_lib/sharpgui.so" "$D/linux/sharpgui.so"
    else tail -3 "$MADE/gui.txt"; echo "NOTE: the GUI library did not build - everything but GUI programs works"; fi
else
    echo "NOTE: no GUI: SDL2 is not installed here. For GUI programs:  sudo apt install libsdl2-dev   and install again"
fi

# the starters, the installers and the example of a release, with the version filled in
fill() {   # fill <template> <target> <crlf|lf>
    sed "s/@VERSION@/$V/g" "$1" | tr -d '\r' > "$2"
    [ "$3" = crlf ] && sed -i 's/$/\r/' "$2"
    return 0
}
for f in win-install.cmd win-uninstall.cmd sharpc.cmd; do fill "$T/windows/$f" "$D/windows/$f" crlf; done
for f in unix-install.sh unix-uninstall.sh sharpc; do fill "$T/linux/$f" "$D/linux/$f" lf; chmod +x "$D/linux/$f"; done
fill "$T/shared/examples/hello.sharp" "$D/shared/examples/hello.sharp" lf
commit=$(git -C "$REPO" rev-parse --short HEAD 2>/dev/null)
# (seen from WSL a Windows checkout differs in line ends and file modes only: those are not changes)
[ -n "$(git -C "$REPO" -c core.fileMode=false -c core.autocrlf=true status --porcelain 2>/dev/null | grep -v '^??' | head -1)" ] && commit="$commit + changes not committed yet"
{
    echo "Sharp $V-dev"
    echo ""
    echo "A DEVELOPMENT BUILD (build-scripts/build.sh install): not a release."
    echo "language (compiler and standard library): $V-dev"
    echo "virtual machine:                          $VM_VERSION"
    echo "built: $(date -u +%Y-%m-%d), branch $(git -C "$REPO" rev-parse --abbrev-ref HEAD 2>/dev/null), commit $commit"
} | sed 's/$/\r/' > "$D/VERSION.txt"

if [ "$ACTION" = stage ]; then echo "made ready: $D"; exit 0; fi
sh "$D/linux/unix-install.sh" | tail -8
[ "${PIPESTATUS[0]}" = 0 ] || fail "NOT INSTALLED: the installer stopped (its message is above)"
INSTALLED="${SHARP_HOME:-$HOME/.sharp}"
echo ""
echo "installed, a development build: $("$INSTALLED/bin/sharp" -V 2>&1 | head -1), $("$INSTALLED/bin/sharpc" -V 2>&1 | head -1)"
echo "($(sed -n '6p' "$INSTALLED/VERSION.txt" | tr -d '\r'); open a NEW terminal to use it)"
