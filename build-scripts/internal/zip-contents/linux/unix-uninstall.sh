#!/bin/sh
# Removes Sharp from this user account: the folder ~/.sharp (or SHARP_HOME) and the PATH line
# unix-install.sh added. Programs you compiled are yours and are not touched.
#
#     sh unix-uninstall.sh
DEST=${SHARP_HOME:-$HOME/.sharp}
MARK="# Sharp (added by unix-install.sh)"
for rc in "$HOME/.profile" "$HOME/.bashrc" "$HOME/.zshrc"; do
    if [ -f "$rc" ] && grep -qF "$MARK" "$rc"; then
        grep -vF "$MARK" "$rc" > "$rc.sharp-tmp"; cat "$rc.sharp-tmp" > "$rc"; rm -f "$rc.sharp-tmp"
    fi
done
if [ -d "$DEST/compiler" ] || [ -d "$DEST/bin" ]; then
    rm -rf "$DEST/bin" "$DEST/compiler" "$DEST/lib" "$DEST/examples" "$DEST/VERSION.txt" "$DEST/unix-uninstall.sh"
    rmdir "$DEST" 2>/dev/null || true
    echo "Sharp was removed from $DEST. Open a new terminal: 'sharp' and 'sharpc' are gone from it."
else
    echo "Sharp is not installed in $DEST (nothing to remove)."
fi
