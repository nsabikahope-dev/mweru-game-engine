# Quick Start Guide

## Running the Engine

```bash
cd /home/gillian/game-engine/build
./bin/Sandbox
```

You should see a window with a purple square that you can move with the arrow keys!

## Controls

- **Arrow Keys** - Move the square around
- **Close Window** or **Escape** - Exit the application

## What's Working

✅ **Phase 1 Complete!**
- SDL2 window with OpenGL 3.3+ context
- Shader compilation and linking
- Texture loading (stb_image)
- Basic rendering (VAO/VBO/EBO)
- Game loop with delta time
- Keyboard input

## Project Structure

```
game-engine/
├── Engine/           # Core engine library
│   ├── src/
│   │   ├── Core/            # Application, Window
│   │   └── Rendering/       # Shader, Texture
│   └── include/Engine/
├── Sandbox/          # Test application (the purple square demo)
├── build/            # Build output
│   └── bin/Sandbox   # Executable
└── vendor/           # Third-party libraries
```

## Making Changes

After modifying code, rebuild:

```bash
cd build
make -j4
./bin/Sandbox
```

## Common Experiments

### Change the square color
Edit `Sandbox/src/main.cpp`, line ~137:
```cpp
m_Shader->SetUniformFloat4("u_Color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red
```

### Change movement speed
Edit `Sandbox/src/main.cpp`, line ~114:
```cpp
float speed = 5.0f * ts.GetSeconds(); // Faster movement
```

### Change window size
Edit `Sandbox/src/main.cpp`, line ~17:
```cpp
: Engine::Application("Sandbox - Game Engine", 1920, 1080)
```

## Next Phase

When you're ready, we'll implement **Phase 2: ECS Architecture** with:
- EnTT integration
- Entity/Component system
- Scene management
- Multiple sprites with components

## Troubleshooting

**Black screen?** Check the console for OpenGL errors.

**Crash on startup?** Make sure SDL2 is installed: `sudo apt-get install libsdl2-dev`

**Can't find Sandbox?** Make sure you're in the `build` directory.
