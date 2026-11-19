#!/bin/sh
# Installs Sharp @VERSION@ for the current user. No sudo needed.
#
#     sh unix-install.sh
#
# What it does: copies Sharp into ~/.sharp (set SHARP_HOME to choose another folder), adds
# ~/.sharp/bin to your PATH (one marked line in your shell's start-up files), and then
# compiles and runs a small program to prove that everything works.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
SHARED="$HERE/../shared"
DEST=${SHARP_HOME:-$HOME/.sharp}
MARK="# Sharp (added by unix-install.sh)"

if [ ! -f "$HERE/sharp" ] || [ ! -d "$SHARED/lib" ] || [ ! -d "$SHARED/compiler" ]; then
    echo "This script must stay inside the unzipped sharp-@VERSION@ folder: it needs the"
    echo "'shared' folder next to the 'linux' folder. Unzip the whole archive and run it from there."
    exit 1
fi

echo "Installing Sharp @VERSION@ into $DEST ..."
rm -rf "$DEST/bin" "$DEST/compiler" "$DEST/lib" "$DEST/examples"
mkdir -p "$DEST/bin"
cp "$HERE/sharp" "$HERE/sharpc" "$DEST/bin/"
# the GUI's native library goes beside sharp: std.guikit loads it from the VM's own folder
[ -f "$HERE/sharpgui.so" ] && cp "$HERE/sharpgui.so" "$DEST/bin/"
chmod +x "$DEST/bin/sharp" "$DEST/bin/sharpc"
cp -r "$SHARED/compiler" "$SHARED/lib" "$SHARED/examples" "$DEST/"
cp "$HERE/../VERSION.txt" "$DEST/" 2>/dev/null || true
cp "$HERE/unix-uninstall.sh" "$DEST/" 2>/dev/null || true

# ---- the PATH: one line, in the files your shell reads when it starts
LINE="export PATH=\"$DEST/bin:\$PATH\"  $MARK"
for rc in "$HOME/.profile" "$HOME/.bashrc" "$HOME/.zshrc"; do
    # .profile is always given the line; the others only when you already have them
    if [ "$rc" != "$HOME/.profile" ] && [ ! -f "$rc" ]; then continue; fi
    if [ -f "$rc" ] && grep -qF "$MARK" "$rc"; then
        # (grep -v says "failed" when no other line is left: the steps are not chained on it)
        grep -vF "$MARK" "$rc" > "$rc.sharp-tmp" || true
        cat "$rc.sharp-tmp" > "$rc"
        rm -f "$rc.sharp-tmp"
    fi
    printf '%s\n' "$LINE" >> "$rc"
done

# ---- the proof: compile and run the example
WORK=$(mktemp -d)
if "$DEST/bin/sharpc" "$DEST/examples/hello.sharp" -o "$WORK/hello" > "$WORK/compile.txt" 2>&1 \
   && "$DEST/bin/sharp" "$WORK/hello" > "$WORK/run.txt" 2>&1 && grep -q "Hello from Sharp" "$WORK/run.txt"; then
    echo ""
    grep "Hello from Sharp" "$WORK/run.txt"
    echo ""
    echo "Sharp @VERSION@ is installed."
    echo "Open a NEW terminal (so it sees the new PATH), then from any folder:"
    echo "    sharpc myprogram.sharp -o myprogram"
    echo "    sharp myprogram"
    echo "An example to start from: $DEST/examples/hello.sharp"
    rm -rf "$WORK"
else
    echo ""
    echo "Sharp was copied to $DEST, but the test program did not run. What it said:"
    tail -15 "$WORK/compile.txt" 2>/dev/null
    tail -5 "$WORK/run.txt" 2>/dev/null
    echo "(the full output is in $WORK)"
    exit 1
fi
