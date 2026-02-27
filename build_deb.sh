#!/usr/bin/env bash
# =============================================================================
# build_deb.sh — Build a .deb installer for the Game Engine
#
# Usage:
#   ./build_deb.sh                    # version defaults to 1.0.0
#   ./build_deb.sh --version 1.2.0
#   ./build_deb.sh --version 2.0.0 --maintainer "Your Name <you@email.com>"
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VERSION="1.0.0"
MAINTAINER="Gillian <your@email.com>"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)    VERSION="$2";    shift 2 ;;
        --maintainer) MAINTAINER="$2"; shift 2 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

DEB_NAME="GameEngine-${VERSION}-linux-amd64.deb"
APPIMAGE="$SCRIPT_DIR/GameEngine-linux-x86_64.AppImage"

if [[ ! -f "$APPIMAGE" ]]; then
    echo "Error: AppImage not found at $APPIMAGE" >&2
    echo "       Run a Release build first, or re-run the AppImage build step." >&2
    exit 1
fi

echo "Building .deb package v$VERSION..."

DEB_ROOT="$(mktemp -d)/gameengine-deb"
mkdir -p "$DEB_ROOT/DEBIAN"
mkdir -p "$DEB_ROOT/opt/GameEngine"
mkdir -p "$DEB_ROOT/usr/bin"
mkdir -p "$DEB_ROOT/usr/share/applications"
mkdir -p "$DEB_ROOT/usr/share/icons/hicolor/256x256/apps"

# Binary
cp "$APPIMAGE" "$DEB_ROOT/opt/GameEngine/GameEngine"
chmod +x "$DEB_ROOT/opt/GameEngine/GameEngine"
ln -s /opt/GameEngine/GameEngine "$DEB_ROOT/usr/bin/GameEngine"

# Icon (extract from AppImage if squashfs-tools available, else skip)
if command -v unsquashfs &>/dev/null; then
    TMP_SQ="$(mktemp -d)"
    unsquashfs -q -d "$TMP_SQ/sq" "$APPIMAGE" 'GameEngine.png' 2>/dev/null || true
    ICON="$(find "$TMP_SQ" -name 'GameEngine.png' | head -1)"
    if [[ -n "$ICON" ]]; then
        cp "$ICON" "$DEB_ROOT/usr/share/icons/hicolor/256x256/apps/GameEngine.png"
    fi
    rm -rf "$TMP_SQ"
fi

# Desktop file
cat > "$DEB_ROOT/usr/share/applications/GameEngine.desktop" << DESKTOP
[Desktop Entry]
Name=Game Engine
Comment=Create 2D games and visual novels — no coding required
Exec=/opt/GameEngine/GameEngine
Icon=GameEngine
Type=Application
Categories=Development;Game;
StartupNotify=true
DESKTOP

# Control file
INSTALLED_KB=$(du -sk "$DEB_ROOT/opt" "$DEB_ROOT/usr" | awk '{sum+=$1} END{print sum}')
cat > "$DEB_ROOT/DEBIAN/control" << CONTROL
Package: gameengine
Version: $VERSION
Section: games
Priority: optional
Architecture: amd64
Installed-Size: $INSTALLED_KB
Maintainer: $MAINTAINER
Description: Game Engine — 2D game and visual novel creator
 A beginner-friendly 2D game engine for creating games,
 visual novels, quizzes, and interactive stories.
 No coding knowledge required to get started.
 .
 Features: sprite editor, physics, dialogue system,
 particle effects, Lua scripting, and game export.
CONTROL

# Maintainer scripts
cat > "$DEB_ROOT/DEBIAN/postinst" << 'POSTINST'
#!/bin/sh
set -e
chmod +x /opt/GameEngine/GameEngine 2>/dev/null || true
gtk-update-icon-cache /usr/share/icons/hicolor/ 2>/dev/null || true
update-desktop-database /usr/share/applications/ 2>/dev/null || true
exit 0
POSTINST
chmod 755 "$DEB_ROOT/DEBIAN/postinst"

printf '#!/bin/sh\nexit 0\n' > "$DEB_ROOT/DEBIAN/prerm"
chmod 755 "$DEB_ROOT/DEBIAN/prerm"

# Build
dpkg-deb --build --root-owner-group "$DEB_ROOT" "$SCRIPT_DIR/$DEB_NAME"

SIZE=$(du -sh "$SCRIPT_DIR/$DEB_NAME" | cut -f1)
echo ""
echo "========================================================"
echo "  Done!"
echo "  Package : $DEB_NAME"
echo "  Size    : $SIZE"
echo ""
echo "  Share this file with your users."
echo "  They double-click it, click Install — that's it."
echo "========================================================"
