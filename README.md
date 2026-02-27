# 2D Game Engine

A production-ready 2D game engine built from scratch in C++ using modern OpenGL, ECS architecture, and ImGui.

## Features (Phase 1 Complete!)

- ✅ Window management with SDL2
- ✅ OpenGL 4.5 Core rendering context
- ✅ Shader compilation and management
- ✅ Texture loading (stb_image)
- ✅ Game loop with delta time
- ✅ Application framework

## Coming Soon

- Phase 2: ECS Architecture (EnTT)
- Phase 3: Batch Renderer & Camera
- Phase 4: Asset Management
- Phase 5: Input System
- Phase 6: Physics (Box2D)
- Phase 7: Audio (OpenAL)
- Phase 8: Scene Serialization
- Phase 9-10: ImGui Editor
- Phase 11: Scripting (Lua)
- Phase 12: Polish & Production Features

## Building the Project

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
```

## Usage

The Sandbox application demonstrates basic usage:

1. Opens a window with OpenGL context
2. Renders a colored square
3. Use arrow keys to move the square around

### Creating Your Own Application

```cpp
#include <Engine/Core/Application.h>

class MyGame : public Engine::Application
{
public:
    MyGame() : Application("My Game", 1920, 1080) {}

    void OnInit() override {
        // Load resources, create game objects
    }

    void OnUpdate(Engine::Timestep ts) override {
        // Update game logic
    }

    void OnRender() override {
        // Render your game
    }
};

int main() {
    MyGame game;
    game.Run();
    return 0;
}
```

## Architecture

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

## Next Steps

Once you have the Sandbox running, we'll move to Phase 2: ECS Architecture!

This will include:
- EnTT integration
- Entity/Component system
- Scene management
- Sprite rendering system

## Troubleshooting

### GLAD generation fails
Make sure you're in the `vendor/glad-repo` directory and try:
```bash
python3 -m glad --api="gl:core=4.5" --out-path=../glad c
```

### CMake can't find SDL2
Install SDL2 development files for your platform (see Prerequisites).

### OpenGL version not supported
This engine requires OpenGL 4.5+. Update your graphics drivers or use a different machine.

## License

This project is for educational purposes.

## Resources

- [Learn OpenGL](https://learnopengl.com/) - Excellent OpenGL tutorials
- [The Cherno's Game Engine Series](https://www.youtube.com/playlist?list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT) - Game engine development
- [EnTT Documentation](https://github.com/skypjack/entt) - ECS library we'll use in Phase 2

---

**Current Phase**: 1/12 (Foundation) ✅
**Status**: Ready to build and test!
