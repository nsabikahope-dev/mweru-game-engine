#!/bin/bash

# Game Engine Setup Script

echo "========================================="
echo "Game Engine - Setup Script"
echo "========================================="
echo ""

# Check for SDL2
echo "[1/4] Checking for SDL2..."
if pkg-config --exists sdl2; then
    echo "✓ SDL2 found"
else
    echo "✗ SDL2 not found. Please install:"
    echo "  Ubuntu/Debian: sudo apt-get install libsdl2-dev"
    echo "  Fedora: sudo dnf install SDL2-devel"
    echo "  macOS: brew install sdl2"
    exit 1
fi

# Setup GLAD
echo ""
echo "[2/4] Setting up GLAD (OpenGL loader)..."
if [ ! -d "vendor/glad/include" ]; then
    cd vendor/glad-repo
    python3 -m glad --api="gl:core=4.5" --out-path=../glad c
    if [ $? -eq 0 ]; then
        echo "✓ GLAD generated successfully"
    else
        echo "✗ GLAD generation failed"
        exit 1
    fi
    cd ../..
else
    echo "✓ GLAD already exists"
fi

# Setup GLM
echo ""
echo "[3/4] Setting up GLM (math library)..."
if [ ! -d "vendor/glm" ]; then
    cd vendor
    git clone --depth 1 https://github.com/g-truc/glm.git
    if [ $? -eq 0 ]; then
        echo "✓ GLM cloned successfully"
    else
        echo "✗ GLM clone failed"
        exit 1
    fi
    cd ..
else
    echo "✓ GLM already exists"
fi

# Setup stb_image
echo ""
echo "[4/4] Setting up stb_image (image loading)..."
if [ ! -f "vendor/stb/stb_image.h" ]; then
    mkdir -p vendor/stb
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -O vendor/stb/stb_image.h
    if [ $? -eq 0 ]; then
        echo "✓ stb_image downloaded successfully"
    else
        echo "✗ stb_image download failed"
        exit 1
    fi
else
    echo "✓ stb_image already exists"
fi

echo ""
echo "========================================="
echo "✓ Setup complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  cmake --build ."
echo "  ./bin/Sandbox"
echo ""
