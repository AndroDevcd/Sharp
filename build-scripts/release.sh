#!/bin/bash
# THE WHOLE RELEASE, one command: builds everything, checks it, and packs
# releases/sharp-<version>.zip.   (Git Bash, on a Windows PC that has WSL)
#
#     bash build-scripts/release.sh 0.7.1             release version 0.7.1 - about 25 minutes
#     bash build-scripts/release.sh 0.7.1 install     the same, then installs it on this PC
#     bash build-scripts/release.sh                   release the version the sources already say
#
# THE VERSION NUMBER is the language version (the compiler and the standard library); the zip is
# named after it. Given a number, this script writes it into the three places of the sources that
# hold it (set_version below). Given none, it uses the number that is already there.
# (The VM has its own number, lib/runtime/main.h `#define rev`: change it by hand when the VM changed.)
#
# IT NEVER REPLACES A RELEASE SILENTLY. When releases/sharp-<version>.zip already exists it asks
#     Replace it? [Y/n]
# and goes on only after a typed Y. Anything else - n, or no answer at all (a run with nobody at
# the keyboard) - stops before anything is built or changed.
#
# A release holds programs for two systems, so it is built on both: the Linux half inside WSL,
# the Windows half here. The steps, each a script that can also be run alone:
#
#     1  WSL      build.sh                         the Linux VM and the compiler
#     2  WSL      test.sh                          the short tests - must end TESTS: PASS
#     3  WSL      internal/release_compilers.sh    the three compilers that ship
#     4  WSL      linux/build_gui_library.sh       sharpgui.so, and the bridge for step 6
#     5  Windows  windows/build_vm.sh              sharp.exe
#     6  Windows  windows/build_gui_library.sh     sharpgui.dll
#     7  WSL      internal/pack_release.sh         the zip
#
# It stops at the first step that fails; fix the cause and run it again (every step only redoes
# what changed, so a second run is quick up to the point of failure).
#
# NOT DONE HERE: the full tests (test.sh full, an hour - run them yourself before a release
# that changed code generation, the optimizer or the VM), the commit, and the push.
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/.." && pwd)
CONFIGURATION="$REPO/lib/compiler/configuration/configuration.sharp"
OPTIONS="$REPO/lib/compiler/configuration/options.sharp"

# ---- the arguments: a version number and/or `install`, in any order
VERSION=""; INSTALL=0
for argument in "$@"; do
    case "$argument" in
        install) INSTALL=1 ;;
        [0-9]*.[0-9]*.[0-9]*) VERSION="$argument" ;;
        *) echo "release.sh: unknown argument '$argument'   (a version like 0.7.1, and/or: install)"; exit 2 ;;
    esac
done
[[ -z "$VERSION" || "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "release.sh: '$VERSION' is not a version number (three numbers: 0.7.1)"; exit 2; }
CURRENT=$(grep -o 'program_vers : string = "[^"]*"' "$CONFIGURATION" | cut -d'"' -f2)
[ -n "$CURRENT" ] || { echo "the version was not found in $CONFIGURATION"; exit 1; }
[ -n "$VERSION" ] || VERSION="$CURRENT"

# ---- an existing release of this version is replaced only after a typed Y
ZIP="$REPO/releases/sharp-$VERSION.zip"
echo "release: Sharp $VERSION$([ "$VERSION" = "$CURRENT" ] && echo "   (the version in the sources)" || echo "   (the sources say $CURRENT now)")"
if [ -f "$ZIP" ]; then
    echo ""
    echo "releases/sharp-$VERSION.zip ALREADY EXISTS  ($(date -r "$ZIP" '+made %Y-%m-%d %H:%M'))."
    echo "Going on builds it again and replaces that file. For a new release, give a new number:  release.sh <version>"
    answer=""
    while [ "$answer" != Y ] && [ "$answer" != y ] && [ "$answer" != N ] && [ "$answer" != n ]; do
        printf "Replace it? [Y/n] "
        read -r answer || { echo ""; echo "no answer: nothing was built or changed"; exit 1; }
    done
    [ "$answer" = Y ] || [ "$answer" = y ] || { echo "stopped: nothing was built or changed"; exit 1; }
fi

# ---- write the version into the sources (only when it is not the one already there)
set_version() {
    # 1. the compiler's own version          configuration.sharp:  program_vers : string = "X";
    sed -i "s/^program_vers : string = \"[^\"]*\";/program_vers : string = \"$VERSION\";/" "$CONFIGURATION"
    # 2. the library version programs get    options.sharp:        static library_version : string = "X";
    sed -i "s/static library_version : string = \"[^\"]*\";/static library_version : string = \"$VERSION\";/" "$OPTIONS"
    # 3. the versions the compiler accepts   options.sharp:        supported_versions := new string[] { ..., "X" };
    #    (the new number goes after the last one of the list, which gets a comma)
    if ! sed -n '/^supported_versions/,/^};/p' "$OPTIONS" | grep -q "\"$VERSION\""; then
        sed -i "/^supported_versions/,/^};/ { /\"$CURRENT\"\r\{0,1\}\$/ s/\"$CURRENT\"/\"$CURRENT\",\n    \"$VERSION\"/ }" "$OPTIONS"
    fi
    grep -q "^program_vers : string = \"$VERSION\";" "$CONFIGURATION" \
        && grep -q "static library_version : string = \"$VERSION\";" "$OPTIONS" \
        && sed -n '/^supported_versions/,/^};/p' "$OPTIONS" | grep -q "\"$VERSION\"" \
        || { echo "the version could not be written into the sources: set it by hand (build-scripts/README.md says where)"; exit 1; }
    echo "the sources now say $VERSION   (configuration.sharp, options.sharp - part of what you commit)"
}
[ "$VERSION" = "$CURRENT" ] || set_version
# the same folder as WSL sees it:  C:\Users\me\Sharp\build-scripts  ->  /mnt/c/Users/me/Sharp/build-scripts
IN_WSL=$(MSYS_NO_PATHCONV=1 wsl wslpath -a "$(cygpath -m "$HERE")" 2>/dev/null | tr -d '\r')
[ -n "$IN_WSL" ] || { echo "WSL did not answer: this script needs WSL (wsl --install), with g++, rsync, python3 and libsdl2-dev in it"; exit 1; }

step() {   # step <number> <where> <script> [arguments]
    local n=$1 where=$2 script=$3; shift 3
    echo ""
    echo "======== $n of 7: $script $*"
    if [ "$where" = wsl ]; then MSYS_NO_PATHCONV=1 wsl bash "$IN_WSL/$script" "$@"
    else bash "$HERE/$script" "$@"; fi
    local code=$?
    [ $code = 0 ] || { echo ""; echo "RELEASE STOPPED at step $n ($script): fix what it reports above and run release.sh again"; exit 1; }
}
step 1 wsl     build.sh
step 2 wsl     test.sh
step 3 wsl     internal/release_compilers.sh
step 4 wsl     linux/build_gui_library.sh
step 5 windows windows/build_vm.sh
step 6 windows windows/build_gui_library.sh
step 7 wsl     internal/pack_release.sh
if [ $INSTALL = 1 ]; then
    echo ""
    echo "======== installing on this PC"
    bash "$HERE/windows/install_release.sh" || exit 1
fi
echo ""
echo "RELEASE DONE. Not done for you: the commit and the push."
