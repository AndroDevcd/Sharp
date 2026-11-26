#!/bin/bash
# Installs the release of the version the sources say on this PC.   (Git Bash, on Windows)
#
#     bash build-scripts/windows/install_release.sh
#
# It unpacks releases/sharp-<program_vers>.zip (the newest release when that one is not there)
# into build/install-release/ and runs the release's own
# installer (windows\win-install.cmd /quiet) - exactly what someone who downloaded the zip would run.
# The installer replaces %LOCALAPPDATA%\Sharp and proves itself by compiling and running the hello
# example. Open a NEW terminal afterwards: `sharp` and `sharpc` are then the new version.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
# the release of the version in the sources - not simply the highest number: an abandoned higher
# number (0.7.0, when the sources went back to the 0.4 line) must not win over the current one
WANTED=$(grep -o 'program_vers : string = "[^"]*"' "$ROOT/compiler/configuration/configuration.sharp" 2>/dev/null | cut -d'"' -f2)
ZIP="$ROOT/releases/sharp-$WANTED.zip"
[ -n "$WANTED" ] && [ -f "$ZIP" ] || ZIP=$(ls "$ROOT"/releases/sharp-*.zip 2>/dev/null | sort -V | tail -1)
[ -n "$ZIP" ] || { echo "no release in releases/: run  bash build-scripts/release.sh  first"; exit 1; }
TO="$ROOT/build/install-release"; rm -rf "$TO"; mkdir -p "$TO"
unzip -q "$ZIP" -d "$TO" || { echo "could not unpack $ZIP"; exit 1; }
INSTALLER=$(ls -d "$TO"/sharp-*/windows/win-install.cmd | head -1)
echo "installing $(basename "$ZIP")"
# cmd.exe wants a Windows path, and //c is how Git Bash passes /c through
cmd.exe //c "$(cygpath -w "$INSTALLER")" //quiet | tail -6
[ "${PIPESTATUS[0]}" = 0 ] || { echo "NOT INSTALLED: the installer stopped (its message is above)"; exit 1; }
INSTALLED="$LOCALAPPDATA/Sharp"
[ -x "$INSTALLED/bin/sharp.exe" ] || { echo "the installer did not leave a sharp.exe in $INSTALLED/bin"; exit 1; }
echo ""
echo "installed: $("$INSTALLED/bin/sharp.exe" -V 2>&1 | head -1), $(head -1 "$INSTALLED/VERSION.txt" | tr -d '\r')"
[ -f "$INSTALLED/bin/sharpgui.dll" ] && echo "the GUI library is installed" || echo "NOTE: no sharpgui.dll was installed - GUI programs will not run"
[ -x "$INSTALLED/bin/sdb.exe" ] && echo "the debugger is installed: $("$INSTALLED/bin/sdb.exe" -V 2>&1 | head -1)" || echo "NOTE: no sdb.exe was installed - there is no debugger"
