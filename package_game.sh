#!/usr/bin/env bash
# =============================================================================
# package_game.sh — Export a standalone game package from the engine editor
#
# Usage:
#   ./package_game.sh [OPTIONS]
#
# Options:
#   -n, --name <name>       Game name (default: MyGame)
#   -s, --scene <path>      Path to the .scene file to package (required)
#   -o, --output <dir>      Output directory (default: ./dist)
#   -b, --build-dir <dir>   CMake build directory (default: ./build)
#   --release               Build in Release mode before packaging
#   -h, --help              Show this help message
#
# Example:
#   ./package_game.sh --name Breakout --scene assets/scenes/breakout.scene
#   ./package_game.sh -n Platformer -s my_level.scene --release -o ~/Desktop/game
# =============================================================================

set -euo pipefail

# --------------------------------------------------------------------------
# Defaults
# --------------------------------------------------------------------------
GAME_NAME="MyGame"
SCENE_PATH=""
OUTPUT_DIR="./dist"
BUILD_DIR="./build"
RELEASE_BUILD=false

# --------------------------------------------------------------------------
# Parse arguments
# --------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        -n|--name)    GAME_NAME="$2";   shift 2 ;;
        -s|--scene)   SCENE_PATH="$2";  shift 2 ;;
        -o|--output)  OUTPUT_DIR="$2";  shift 2 ;;
        -b|--build-dir) BUILD_DIR="$2"; shift 2 ;;
        --release)    RELEASE_BUILD=true; shift ;;
        -h|--help)
            sed -n '2,20p' "$0"
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            echo "Run '$0 --help' for usage." >&2
            exit 1
            ;;
    esac
done

# --------------------------------------------------------------------------
# Validate
# --------------------------------------------------------------------------
if [[ -z "$SCENE_PATH" ]]; then
    echo "Error: --scene <path> is required." >&2
    echo "Run '$0 --help' for usage." >&2
    exit 1
fi

if [[ ! -f "$SCENE_PATH" ]]; then
    echo "Error: Scene file not found: $SCENE_PATH" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$(cd "$BUILD_DIR" 2>/dev/null && pwd || { echo "Error: Build directory not found: $BUILD_DIR" >&2; exit 1; })"

echo "========================================================"
echo "  Game Engine — Package Script"
echo "========================================================"
echo "  Game Name  : $GAME_NAME"
echo "  Scene File : $SCENE_PATH"
echo "  Build Dir  : $BUILD_DIR"
echo "  Output Dir : $OUTPUT_DIR"
echo "  Release    : $RELEASE_BUILD"
echo "========================================================"

# --------------------------------------------------------------------------
# Optional: rebuild in Release mode
# --------------------------------------------------------------------------
if $RELEASE_BUILD; then
    echo ""
    echo ">> Building in Release mode..."
    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" --target Runtime -- -j"$(nproc)"
    echo ">> Build complete."
fi

# --------------------------------------------------------------------------
# Locate the Runtime binary (standalone player, no editor UI)
# --------------------------------------------------------------------------
RUNTIME_BIN="$BUILD_DIR/bin/Runtime"
if [[ ! -f "$RUNTIME_BIN" ]]; then
    echo "Error: Runtime binary not found at $RUNTIME_BIN" >&2
    echo "       Run 'cmake --build $BUILD_DIR --target Runtime' first." >&2
    exit 1
fi

# --------------------------------------------------------------------------
# Create package directory structure
# --------------------------------------------------------------------------
PKG_DIR="$OUTPUT_DIR/$GAME_NAME"
echo ""
echo ">> Creating package at: $PKG_DIR"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR"
mkdir -p "$PKG_DIR/assets/scripts"
mkdir -p "$PKG_DIR/assets/textures"
mkdir -p "$PKG_DIR/assets/audio"
mkdir -p "$PKG_DIR/assets/scenes"

# --------------------------------------------------------------------------
# Copy the Runtime binary (renamed to the game name)
# --------------------------------------------------------------------------
echo ">> Copying Runtime binary..."
cp "$RUNTIME_BIN" "$PKG_DIR/$GAME_NAME"
chmod +x "$PKG_DIR/$GAME_NAME"

# --------------------------------------------------------------------------
# Copy the scene file
# --------------------------------------------------------------------------
echo ">> Copying scene file..."
SCENE_BASENAME="$(basename "$SCENE_PATH")"
cp "$SCENE_PATH" "$PKG_DIR/assets/scenes/$SCENE_BASENAME"

# Write a launcher config so the binary knows which scene to open
cat > "$PKG_DIR/game.json" <<JSON
{
    "name": "$GAME_NAME",
    "start_scene": "assets/scenes/$SCENE_BASENAME",
    "version": "1.0.0"
}
JSON

# --------------------------------------------------------------------------
# Copy Lua scripts referenced by the scene
# --------------------------------------------------------------------------
echo ">> Collecting Lua scripts referenced in scene..."
# Extract ScriptPath values from the scene JSON
SCRIPTS=$(grep -o '"ScriptPath"[[:space:]]*:[[:space:]]*"[^"]*"' "$SCENE_PATH" \
          | sed 's/"ScriptPath"[[:space:]]*:[[:space:]]*"//;s/"//' || true)

if [[ -n "$SCRIPTS" ]]; then
    while IFS= read -r script; do
        if [[ -f "$SCRIPT_DIR/$script" ]]; then
            dest_dir="$PKG_DIR/$(dirname "$script")"
            mkdir -p "$dest_dir"
            cp "$SCRIPT_DIR/$script" "$dest_dir/"
            echo "   + $script"
        elif [[ -f "$script" ]]; then
            dest_dir="$PKG_DIR/$(dirname "$script")"
            mkdir -p "$dest_dir"
            cp "$script" "$dest_dir/"
            echo "   + $script"
        else
            echo "   ! Warning: script not found: $script"
        fi
    done <<< "$SCRIPTS"
else
    echo "   (no Lua scripts referenced)"
fi

# --------------------------------------------------------------------------
# Copy textures referenced by the scene
# --------------------------------------------------------------------------
echo ">> Collecting textures referenced in scene..."
TEXTURES=$(grep -o '"TexturePath"[[:space:]]*:[[:space:]]*"[^"]*"' "$SCENE_PATH" \
           | sed 's/"TexturePath"[[:space:]]*:[[:space:]]*"//;s/"//' || true)

if [[ -n "$TEXTURES" ]]; then
    while IFS= read -r tex; do
        if [[ -f "$SCRIPT_DIR/$tex" ]]; then
            dest_dir="$PKG_DIR/$(dirname "$tex")"
            mkdir -p "$dest_dir"
            cp "$SCRIPT_DIR/$tex" "$dest_dir/"
            echo "   + $tex"
        elif [[ -f "$tex" ]]; then
            dest_dir="$PKG_DIR/$(dirname "$tex")"
            mkdir -p "$dest_dir"
            cp "$tex" "$dest_dir/"
            echo "   + $tex"
        else
            echo "   ! Warning: texture not found: $tex"
        fi
    done <<< "$TEXTURES"
else
    echo "   (no textures referenced)"
fi

# --------------------------------------------------------------------------
# Copy audio clips referenced by the scene
# --------------------------------------------------------------------------
echo ">> Collecting audio clips referenced in scene..."
CLIPS=$(grep -o '"ClipPath"[[:space:]]*:[[:space:]]*"[^"]*"' "$SCENE_PATH" \
        | sed 's/"ClipPath"[[:space:]]*:[[:space:]]*"//;s/"//' || true)

if [[ -n "$CLIPS" ]]; then
    while IFS= read -r clip; do
        if [[ -f "$SCRIPT_DIR/$clip" ]]; then
            dest_dir="$PKG_DIR/$(dirname "$clip")"
            mkdir -p "$dest_dir"
            cp "$SCRIPT_DIR/$clip" "$dest_dir/"
            echo "   + $clip"
        elif [[ -f "$clip" ]]; then
            dest_dir="$PKG_DIR/$(dirname "$clip")"
            mkdir -p "$dest_dir"
            cp "$clip" "$dest_dir/"
            echo "   + $clip"
        else
            echo "   ! Warning: audio clip not found: $clip"
        fi
    done <<< "$CLIPS"
else
    echo "   (no audio clips referenced)"
fi

# --------------------------------------------------------------------------
# Copy animation frame sets referenced by the scene
# --------------------------------------------------------------------------
echo ">> Collecting animation frames referenced in scene..."
ANIM_PATHS=$(grep -o '"FramePaths"[[:space:]]*:[[:space:]]*\[[^]]*\]' "$SCENE_PATH" \
             | grep -o '"assets/[^"]*"' | tr -d '"' || true)

if [[ -n "$ANIM_PATHS" ]]; then
    while IFS= read -r frame; do
        if [[ -f "$SCRIPT_DIR/$frame" ]]; then
            dest_dir="$PKG_DIR/$(dirname "$frame")"
            mkdir -p "$dest_dir"
            cp "$SCRIPT_DIR/$frame" "$dest_dir/"
            echo "   + $frame"
        fi
    done <<< "$ANIM_PATHS"
else
    echo "   (no animation frame assets referenced)"
fi

# --------------------------------------------------------------------------
# Bundle shared libraries needed at runtime
# --------------------------------------------------------------------------
echo ">> Collecting shared library dependencies..."
LIBS_DIR="$PKG_DIR/lib"
mkdir -p "$LIBS_DIR"

# Key runtime libraries that are not universally available
RUNTIME_LIBS=(
    "libopenal.so.1"
    "libsndfile.so.1"
    "libSDL2-2.0.so.0"
)

for libname in "${RUNTIME_LIBS[@]}"; do
    libpath=$(ldconfig -p 2>/dev/null | grep "^\s*${libname}\b" | awk '{print $NF}' | head -1 || true)
    if [[ -n "$libpath" && -f "$libpath" ]]; then
        cp "$libpath" "$LIBS_DIR/"
        echo "   + $libname"
    else
        echo "   ! Warning: $libname not found via ldconfig (may be a system default)"
    fi
done

# --------------------------------------------------------------------------
# Write a launch wrapper script
# --------------------------------------------------------------------------
cat > "$PKG_DIR/run.sh" <<'LAUNCHER'
#!/usr/bin/env bash
# Launch the game with bundled libraries
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:${LD_LIBRARY_PATH:-}"
# Pass the packaged scene as the first argument; extra args (e.g. width height) are forwarded
exec "$SCRIPT_DIR/GAME_NAME_PLACEHOLDER" "assets/scenes/SCENE_BASENAME_PLACEHOLDER" "$@"
LAUNCHER

# Replace placeholders with actual values
sed -i "s/GAME_NAME_PLACEHOLDER/$GAME_NAME/g" "$PKG_DIR/run.sh"
sed -i "s/SCENE_BASENAME_PLACEHOLDER/$SCENE_BASENAME/g" "$PKG_DIR/run.sh"
chmod +x "$PKG_DIR/run.sh"

# --------------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------------
TOTAL_SIZE=$(du -sh "$PKG_DIR" 2>/dev/null | cut -f1)

echo ""
echo "========================================================"
echo "  Package complete!"
echo "  Location : $PKG_DIR"
echo "  Size     : $TOTAL_SIZE"
echo "  Run with : $PKG_DIR/run.sh"
echo "========================================================"
echo ""
echo "  To distribute, archive the package:"
echo "    tar -czf ${GAME_NAME}.tar.gz -C \"$(dirname "$PKG_DIR")\" \"$GAME_NAME\""
echo ""
