#!/usr/bin/env bash
set -e
PKG_DIR="$(cd "$(dirname "$0")" && pwd)"

if [ -f "$PKG_DIR/bin/quasar" ]; then SRC_BIN="$PKG_DIR/bin/quasar"
elif [ -f "$PKG_DIR/../build/quasar" ]; then SRC_BIN="$PKG_DIR/../build/quasar"
elif [ -f "$PKG_DIR/build/quasar" ]; then SRC_BIN="$PKG_DIR/build/quasar"
else echo "Error: quasar binary not found"; exit 1; fi

mkdir -p "$HOME/.local/bin" "$HOME/.local/share/applications" "$HOME/.local/share/icons/hicolor/scalable/apps"
cp "$SRC_BIN" "$HOME/.local/bin/quasar"
chmod +x "$HOME/.local/bin/quasar"

for d in "$PKG_DIR/share/applications/quasar.desktop" "$PKG_DIR/../assets/quasar.desktop" "$PKG_DIR/assets/quasar.desktop"; do
  if [ -f "$d" ]; then cp "$d" "$HOME/.local/share/applications/quasar.desktop"; break; fi
done
chmod +x "$HOME/.local/share/applications/quasar.desktop"

for d in "$PKG_DIR/share/icons/hicolor/scalable/apps/quasar.svg" "$PKG_DIR/../assets/quasar.svg" "$PKG_DIR/assets/quasar.svg"; do
  if [ -f "$d" ]; then cp "$d" "$HOME/.local/share/icons/hicolor/scalable/apps/quasar.svg"; break; fi
done

gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
echo "=== Quasar installed. Run: quasar ==="
