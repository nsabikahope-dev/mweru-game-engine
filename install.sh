#!/usr/bin/env bash
# =============================================================================
# install.sh — Install the Game Engine on Ubuntu / Linux
#
# What this does:
#   1. Copies the AppImage to ~/.local/bin/
#   2. Creates a .desktop launcher so it appears in your app menu
#   3. Installs the icon
#
# Usage:
#   chmod +x install.sh
#   ./install.sh
#
# To uninstall:
#   ./install.sh --uninstall
# =============================================================================

set -euo pipefail

APPIMAGE_NAME="GameEngine-linux-x86_64.AppImage"
INSTALL_NAME="GameEngine"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APPIMAGE_SRC="$SCRIPT_DIR/$APPIMAGE_NAME"

BIN_DIR="$HOME/.local/bin"
APP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"

# --------------------------------------------------------------------------
# Uninstall
# --------------------------------------------------------------------------
if [[ "${1:-}" == "--uninstall" ]]; then
    echo "Uninstalling Game Engine..."
    rm -f "$BIN_DIR/$INSTALL_NAME"
    rm -f "$APP_DIR/GameEngine.desktop"
    rm -f "$ICON_DIR/GameEngine.png"
    gtk-update-icon-cache ~/.local/share/icons/hicolor/ 2>/dev/null || true
    echo "Done. Game Engine has been removed."
    exit 0
fi

# --------------------------------------------------------------------------
# Check the AppImage is present
# --------------------------------------------------------------------------
if [[ ! -f "$APPIMAGE_SRC" ]]; then
    echo "Error: $APPIMAGE_NAME not found in $SCRIPT_DIR" >&2
    echo "       Make sure the AppImage is in the same folder as this script." >&2
    exit 1
fi

# --------------------------------------------------------------------------
# Install
# --------------------------------------------------------------------------
echo "Installing Game Engine..."

mkdir -p "$BIN_DIR" "$APP_DIR" "$ICON_DIR"

# Copy AppImage
cp "$APPIMAGE_SRC" "$BIN_DIR/$INSTALL_NAME"
chmod +x "$BIN_DIR/$INSTALL_NAME"
echo "  Installed binary : $BIN_DIR/$INSTALL_NAME"

# Extract icon from AppImage (it's embedded as GameEngine.png inside the squashfs)
# We mount/extract a copy; if that fails, skip gracefully
if command -v unsquashfs &>/dev/null; then
    TMP_EXTRACT="$(mktemp -d)"
    unsquashfs -q -d "$TMP_EXTRACT/sq" "$APPIMAGE_SRC" '*.png' 2>/dev/null || true
    ICON_SRC="$(find "$TMP_EXTRACT" -name 'GameEngine.png' | head -1)"
    if [[ -n "$ICON_SRC" ]]; then
        cp "$ICON_SRC" "$ICON_DIR/GameEngine.png"
        echo "  Installed icon   : $ICON_DIR/GameEngine.png"
    fi
    rm -rf "$TMP_EXTRACT"
fi

# .desktop file
cat > "$APP_DIR/GameEngine.desktop" << DESKTOP
[Desktop Entry]
Name=Game Engine
Comment=2D Game Engine and Editor
Exec=$BIN_DIR/$INSTALL_NAME
Icon=GameEngine
Type=Application
Categories=Development;Game;
StartupNotify=true
DESKTOP
echo "  Installed launcher: $APP_DIR/GameEngine.desktop"

# Refresh icon and app caches
gtk-update-icon-cache ~/.local/share/icons/hicolor/ 2>/dev/null || true
update-desktop-database "$APP_DIR" 2>/dev/null || true

# Make sure ~/.local/bin is in PATH
if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    echo ""
    echo "  NOTE: Add this line to your ~/.bashrc or ~/.profile so the"
    echo "        command works from a terminal:"
    echo ""
    echo '    export PATH="$HOME/.local/bin:$PATH"'
    echo ""
fi

echo ""
echo "========================================================"
echo "  Installation complete!"
echo ""
echo "  To launch: search for 'Game Engine' in your app menu,"
echo "             or run:  GameEngine"
echo ""
echo "  To uninstall: ./install.sh --uninstall"
echo "========================================================"
