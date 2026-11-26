#!/bin/bash
# BUILD SHARP from the repository as it is now.   (Linux / WSL)
#
#     bash build-scripts/build.sh               the VM, then the compiler
#     bash build-scripts/build.sh compiler      only the compiler - 10 seconds. Enough after changing
#                                               only Sharp files (compiler or the standard library).
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
#   - No seed yet (a fresh machine, or the work folder was deleted): the compiler of the newest
#     release in releases/ is the first seed. That folder is not in the repository: put the zip of
#     the newest release there (the project's GitHub Releases page has it).
#   - A change that USES a new language feature inside the compiler's own source needs two builds:
#     first add the feature (the seed can compile that), then use it.
#
# The Windows VM is built on Windows (windows/build_vm.sh); a Raspberry Pi builds and installs with
# linux/install_from_source.sh.
#
# The compiler step prints  errors: N ; built: 1  and the first errors. Exits 1 when anything failed.
source "$(dirname "$0")/internal/common.sh"
[ -z "$1" ] || [ "$1" = compiler ] || fail "build.sh: unknown argument '$1' (nothing, or: compiler)"
copy_sources || exit 1

# ---- the VM
if [ "$1" != compiler ]; then
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
    [ -f "$ZIP" ] || ZIP=$(ls "$REPO"/releases/sharp-*.zip 2>/dev/null | sort -V | tail -1)
    [ -n "$ZIP" ] || fail "no seed compiler at $SEED and no release in releases/ to take one from: download the newest sharp-<version>.zip from the project's GitHub Releases page into $REPO/releases/"
    mkdir -p "$(dirname "$SEED")"
    python3 - "$ZIP" "$SEED" <<'EOF' || fail "could not take the compiler out of the release (python3 is needed)"
import sys, zipfile
z = zipfile.ZipFile(sys.argv[1])
# shared/compiler/sharpc-v<version>: the standard compiler (not the -unoptimized or -debug one)
name = [n for n in z.namelist() if "/shared/compiler/sharpc-v" in n][0]
open(sys.argv[2], "wb").write(z.read(name))
EOF
    echo "first seed: the compiler of $(basename "$ZIP")"
fi
OUT="$WORK/out/boot"; mkdir -p "$OUT"
cd "$WORK/src/compiler" || exit 1
rm -f "$COMPILER"
# every .sharp file of the compiler except its tests; -L: with the standard library
"$VM" "$SEED" $(find . -name "*.sharp" -not -path "./tests/*" | sort) -j 1 -o "$COMPILER" -L "$STDLIB" > "$OUT/c.txt" 2>&1
code=$?
errors=$(grep -c '^error' "$OUT/c.txt")
built=0; [ -f "$COMPILER" ] && built=1
echo "exit $code ; errors: $errors ; built: $built   (the compiler: $COMPILER; whole output: $OUT/c.txt)"
grep -A6 "^error" "$OUT/c.txt" | cut -c1-190 | head -45
if [ "$errors" != 0 ] || [ "$built" != 1 ]; then
    [ "$errors" = 0 ] && tail -5 "$OUT/c.txt" | cut -c1-190       # no error line and no file: show how it ended
    rm -f "$COMPILER"
    exit 1
fi
cp -f "$SEED" "$SEED.previous"
cp -f "$COMPILER" "$SEED"
echo "built: Sharp $LANGUAGE_VERSION (VM $VM_VERSION)"
