#!/bin/bash
# SHARED BY THE BUILD SCRIPTS: where things are. Not run on its own - build.sh, test.sh and the
# others read it (`source .../internal/common.sh`).
#
# THE WORK FOLDER. Building straight out of the repository is slow when the repository is on a
# Windows drive seen from WSL (/mnt/c/...): ten times slower, and long builds make WSL drop the
# connection. So the Linux-side scripts first COPY what they compile into a folder on Linux's own
# disk and build there. Nothing in it is precious: delete it and the next build makes it again
# (the first build is then a full one).
#
#     SHARP_WORK       the work folder                        (default ~/sharp_work)
#
#     $WORK/src        a copy of the repository's sources     (copy_sources below makes it)
#     $WORK/stdlib     a copy of the standard library         ( "  )
#     $WORK/suite      a copy of tests/suite                  ( "  )
#     $WORK/sharp_dev  the Linux VM and its object files      (build.sh)
#     $WORK/out/boot   the compiler: sharpc_boot              (build.sh)
#     $WORK/out/seed   the compiler that builds the next one  (build.sh explains)
#     $WORK/out/...    what the tests and the release steps write
#
# (The folder names inside are historical - "sharp_dev", "boot" - and are kept because the test
#  scripts in compiler/tests use them as their defaults.)

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${SHARP_WORK:-$HOME/sharp_work}"

VM="$WORK/sharp_dev/sharp"                  # the Linux VM the scripts build and use
COMPILER="$WORK/out/boot/sharpc_boot"       # the compiler they build and use (a Sharp program: run as  $VM $COMPILER ...)
SEED="$WORK/out/seed/sharpc_boot"           # the compiler that compiles the compiler
STDLIB="$WORK/stdlib"                       # the copy of the standard library the compiles read

# the standard library in the repository
LIBRARY_SOURCE="$REPO/stdlib"

# the two version numbers, read from the sources (never typed anywhere else)
LANGUAGE_VERSION=$(grep -o 'program_vers : string = "[^"]*"' "$REPO/compiler/configuration/configuration.sharp" 2>/dev/null | cut -d'"' -f2)
VM_VERSION=$(grep -o '#define progvers "[^"]*"' "$REPO/runtime/main.h" 2>/dev/null | cut -d'"' -f2)$(grep -o '#define rev "[^"]*"' "$REPO/runtime/main.h" 2>/dev/null | cut -d'"' -f2)

# Copy what gets compiled from the repository into the work folder (see the top of this file).
# Cheap when little changed: only changed files are copied.
copy_sources() {
    command -v rsync > /dev/null || { echo "rsync is needed:  sudo apt install rsync"; return 1; }
    mkdir -p "$WORK/src" "$WORK/suite" "$WORK/stdlib" "$WORK/out"
    rsync -a --delete --exclude .git --exclude 'cmake-build*' --exclude bin --exclude build --exclude releases \
          --exclude '*.aout' --exclude doc --exclude examples --exclude benchmark --exclude tools \
          --exclude /tests "$REPO/" "$WORK/src/"
    rsync -a --delete "$LIBRARY_SOURCE/" "$WORK/stdlib/"
    rsync -a --delete --exclude /build "$REPO/tests/suite/" "$WORK/suite/"     # (not its build/: the tests build it there)
    # the suite says which standard library it is built with: the repository's, by a path from where
    # it lies there ("standard_library": "../../stdlib"). In this copy that is the copy of the library.
    sed -i "s|\"standard_library\": *\"[^\"]*\"|\"standard_library\": \"$WORK/stdlib\"|" "$WORK/suite/settings.json"
    grep -q "\"standard_library\": \"$WORK/stdlib\"" "$WORK/suite/settings.json" || { echo "tests/suite/settings.json: no \"standard_library\" setting to point at $WORK/stdlib"; return 1; }
}

# How many compiles and test programs run at once: one per core, but no more than the free memory
# allows (a compile takes up to about 500 MB).   TEST_JOBS=<n> chooses it yourself.
if [ -z "$TEST_JOBS" ]; then
    TEST_JOBS=$(nproc 2>/dev/null || echo 4)
    by_memory=$(( $(awk '/MemAvailable/ {print int($2/1024)}' /proc/meminfo 2>/dev/null || echo 2048) / 500 ))
    [ "$by_memory" -lt "$TEST_JOBS" ] && TEST_JOBS=$by_memory
    [ "$TEST_JOBS" -lt 1 ] && TEST_JOBS=1
fi
export TEST_JOBS

# stop with a message
fail() { echo "$*"; exit 1; }

# WRITE THE LANGUAGE VERSION INTO THE SOURCES:  set_language_version 0.7.1
# (the three places that hold it; release.sh and build.sh both take a number and call this).
# Also run when the number is the one already there: a run that stopped half-way leaves
# program_vers new and the list without it, and a compiler built from that refuses its own library.
set_language_version() {
    local version=$1
    local configuration="$REPO/compiler/configuration/configuration.sharp" options="$REPO/compiler/configuration/options.sharp"
    [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "'$version' is not a version number (three numbers: 0.7.1)"; return 1; }
    # 1. the compiler's own version          configuration.sharp:  program_vers : string = "X";
    sed -i "s/^program_vers : string = \"[^\"]*\";/program_vers : string = \"$version\";/" "$configuration"
    # 2. the library version programs get    options.sharp:        static library_version : string = "X";
    sed -i "s/static library_version : string = \"[^\"]*\";/static library_version : string = \"$version\";/" "$options"
    # 3. the versions the compiler accepts   options.sharp:        supported_versions := new string[] { ..., "X" };
    #    (the new number goes after the last one of the list - the one line without a comma, whatever
    #    number it holds: the list is not in order - and that line gets a comma)
    if ! sed -n '/^supported_versions/,/^};/p' "$options" | grep -q "\"$version\""; then
        sed -i "/^supported_versions/,/^};/ { s/^\( *\"[^\"]*\"\)\(\r\{0,1\}\)\$/\1,\2\n    \"$version\"\2/ }" "$options"
    fi
    grep -q "^program_vers : string = \"$version\";" "$configuration" \
        && grep -q "static library_version : string = \"$version\";" "$options" \
        && sed -n '/^supported_versions/,/^};/p' "$options" | grep -q "\"$version\"" \
        || { echo "the version could not be written into the sources: set it by hand (build-scripts/README.md says where)"; return 1; }
    LANGUAGE_VERSION=$version
    echo "the sources now say $version   (configuration.sharp, options.sharp - part of what you commit)"
}

# CHECK THE TOOLS FIRST, and say everything that is missing at once - before anything is built.
#     need_tools "g++:sudo apt install g++" "rsync:sudo apt install rsync" ...
# Each word is <program>:<how to get it>. Stops the script when any is missing.
need_tools() {
    local missing=0 item program
    for item in "$@"; do
        program=${item%%:*}
        if ! command -v "$program" > /dev/null 2>&1; then
            [ $missing = 0 ] && echo "THIS MACHINE IS MISSING what the build needs:"
            missing=1
            echo "    $program    get it with:  ${item#*:}"
        fi
    done
    [ $missing = 0 ] || { echo "Nothing was built. Install the above and run this again."; exit 1; }
}

# A FIRST COMPILER FOR A MACHINE THAT HAS NONE. Sharp's compiler is written in Sharp, so building it
# takes a Sharp compiler; a fresh clone has none (releases/ is not in the repository). This fetches
# the newest release's zip from the project's GitHub Releases page into releases/ and prints its
# path. Returns 1, having said what to do by hand, when it cannot.
RELEASES_PAGE="https://github.com/AndroDevcd/Sharp/releases/latest"
fetch_release() {
    local by_hand="download sharp-<version>.zip from $RELEASES_PAGE into $REPO/releases/ by hand, then run this again"
    local page url name
    if command -v curl > /dev/null 2>&1; then page=$(curl -fsSL "https://api.github.com/repos/AndroDevcd/Sharp/releases/latest" 2>/dev/null)
    elif command -v wget > /dev/null 2>&1; then page=$(wget -qO- "https://api.github.com/repos/AndroDevcd/Sharp/releases/latest" 2>/dev/null)
    else echo "NO COMPILER TO START FROM, and neither curl nor wget to download one:  sudo apt install curl   - or $by_hand" >&2; return 1; fi
    url=$(echo "$page" | grep -o '"browser_download_url": *"[^"]*/sharp-[0-9][^"/]*\.zip"' | head -1 | cut -d'"' -f4)
    [ -n "$url" ] || { echo "NO COMPILER TO START FROM, and the newest release could not be found (no network, or GitHub did not answer): $by_hand" >&2; return 1; }
    name=$(basename "$url")
    mkdir -p "$REPO/releases"
    echo "no Sharp compiler on this machine to start from: downloading $name from the project's GitHub Releases page ..." >&2
    if command -v curl > /dev/null 2>&1; then curl -fL --progress-bar -o "$REPO/releases/$name.part" "$url" >&2
    else wget -q --show-progress -O "$REPO/releases/$name.part" "$url" >&2; fi
    if [ $? != 0 ] || [ ! -s "$REPO/releases/$name.part" ]; then
        rm -f "$REPO/releases/$name.part"
        echo "the download of $url did not finish: $by_hand" >&2; return 1
    fi
    mv "$REPO/releases/$name.part" "$REPO/releases/$name"
    echo "$REPO/releases/$name"
}
