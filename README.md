# 2D Game Engine

A production-ready 2D game engine built from scratch in C++ using modern OpenGL, ECS architecture, and ImGui.
### Prerequisites

1. **Install SDL2**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libsdl2-dev

   # Fedora
   sudo dnf install SDL2-devel

   # macOS
   brew install sdl2
   ```

2. **Setup Vendor Dependencies**

   Run these commands from the project root:

   ```bash
   # Generate GLAD (OpenGL loader)
   cd vendor/glad-repo
   python3 -m glad --api="gl:core=4.5" --out-path=../glad c
   cd ../..

   # Clone GLM (math library)
   cd vendor
   git clone --depth 1 https://github.com/g-truc/glm.git
   cd ..

   # Download stb_image (image loading)
   mkdir -p vendor/stb
   wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -O vendor/stb/stb_image.h
   ```

### Build with CMake

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .

# Run the sandbox
./bin/Sandbox
```

## Project Structure

```
game-engine/
├── Engine/          # Core engine library
│   ├── src/
│   │   ├── Core/          # Application, Window, Time
│   │   └── Rendering/     # Shader, Texture, Renderer
│   └── include/Engine/
├── Sandbox/         # Test application
├── vendor/          # Third-party dependencies
└── CMakeLists.txt

### Core Systems

- **Application**: Main game loop, owns window
- **Window**: SDL2 wrapper, OpenGL context management
- **Timestep**: Delta time management
- **Shader**: GLSL shader compilation and uniforms
- **Texture2D**: Image loading and OpenGL texture management

### Technologies

- **Language**: C++17
- **Build System**: CMake
- **Windowing**: SDL2
- **Graphics**: OpenGL 4.5 Core
- **Math**: GLM
- **Image Loading**: stb_image

## Resources

- [Learn OpenGL](https://learnopengl.com/) - Excellent OpenGL tutorials
- [The Cherno's Game Engine Series](https://www.youtube.com/playlist?list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT) - Game engine development
- [EnTT Documentation](https://github.com/skypjack/entt) - ECS library we'll use in Phase 2

---
