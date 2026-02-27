# Vendor Dependencies Setup

This directory contains third-party libraries needed for the game engine.

## Manual Setup Required

Please run the following commands to set up dependencies:

### 1. GLAD (OpenGL Loader)
```bash
cd /home/gillian/game-engine/vendor/glad-repo
python3 -m glad --api="gl:core=4.5" --out-path=../glad c
```

### 2. GLM (Math Library)
```bash
cd /home/gillian/game-engine/vendor
git clone --depth 1 https://github.com/g-truc/glm.git
```

### 3. stb_image (Image Loading)
```bash
cd /home/gillian/game-engine/vendor
mkdir -p stb
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -O stb/stb_image.h
```

## SDL2

SDL2 should be installed system-wide:
```bash
sudo apt-get install libsdl2-dev  # Ubuntu/Debian
# or
sudo dnf install SDL2-devel  # Fedora
# or
brew install sdl2  # macOS
```

After running these commands, the project will be ready to build!
