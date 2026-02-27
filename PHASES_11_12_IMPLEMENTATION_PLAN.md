# Phase 11 & 12: Complete Implementation Plan

## Current Status

✅ **Completed**: Phases 1-10 (Full engine with visual editor)
🚧 **In Progress**: Phase 11 (Lua Scripting + AI Helper)
📋 **Planned**: Phase 12 (Polish & Production Features)

## Phase 11: Lua Scripting + AI Helper

### What's Already Done ✅

1. **Lua Integration**
   - ✅ Lua 5.4.6 downloaded and built locally
   - ✅ sol2 (C++ Lua binding) downloaded
   - ✅ CMake configuration updated 
   - ✅ Student guide created with examples

### What Needs to Be Implemented

#### 1. Script Component (30 minutes)

**File**: `Engine/include/Engine/ECS/Components.h`
```cpp
struct ScriptComponent
{
    std::string ScriptPath;           // Path to .lua file
    bool Enabled = true;

    // Runtime state (managed by LuaScriptEngine)
    void* LuaState = nullptr;        // sol::state*

    ScriptComponent() = default;
    ScriptComponent(const std::string& path) : ScriptPath(path) {}
};
```

#### 2. Lua Script Engine (1-2 hours)

**File**: `Engine/include/Engine/Scripting/LuaScriptEngine.h`
```cpp
class LuaScriptEngine
{
public:
    static void Init();
    static void Shutdown();

    // Load and execute script
    static void LoadScript(Scene* scene, Entity entity, const std::string& scriptPath);
    static void ReloadScript(Scene* scene, Entity entity);

    // Update all scripts
    static void OnUpdate(Scene* scene, float deltaTime);

    // Expose engine API to Lua
    static void RegisterEngineAPI(sol::state& lua);
};
```

#### 3. Engine API Bindings (1-2 hours)

**Expose to Lua**:
- `Input` class (IsKeyHeld, IsKeyPressed, GetMousePosition, etc.)
- `Transform` component (Position, Rotation, Scale)
- `SpriteRenderer` component (Color)
- `Rigidbody` component (Velocity, if physics enabled)
- `Entity` class (GetComponent, AddComponent)
- `Scene` class (CreateEntity, DestroyEntity)

**Example Binding**:
```cpp
void LuaScriptEngine::RegisterEngineAPI(sol::state& lua)
{
    // Input bindings
    lua.new_usertype<Input>("Input",
        "IsKeyHeld", &Input::IsKeyHeld,
        "IsKeyPressed", &Input::IsKeyPressed,
        "GetMousePosition", &Input::GetMousePosition
    );

    // Transform bindings
    lua.new_usertype<TransformComponent>("Transform",
        "Position", &TransformComponent::Position,
        "Rotation", &TransformComponent::Rotation,
        "Scale", &TransformComponent::Scale
    );

    // More bindings...
}
```

#### 4. Script Editor in Inspector (1 hour)

**File**: `Editor/src/EditorApp.cpp`

Add to Inspector panel:
```cpp
if (m_SelectedEntity.HasComponent<ScriptComponent>())
{
    if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& script = m_SelectedEntity.GetComponent<ScriptComponent>();

        char buffer[256];
        strcpy(buffer, script.ScriptPath.c_str());
        if (ImGui::InputText("Script Path", buffer, sizeof(buffer)))
        {
            script.ScriptPath = buffer;
        }

        if (ImGui::Button("Load Script"))
        {
            LuaScriptEngine::LoadScript(m_Scene.get(), m_SelectedEntity, script.ScriptPath);
        }

        ImGui::SameLine();
        if (ImGui::Button("Reload"))
        {
            LuaScriptEngine::ReloadScript(m_Scene.get(), m_SelectedEntity);
        }

        if (ImGui::Button("AI Helper"))
        {
            ImGui::OpenPopup("AI Script Generator");
        }

        // AI Helper Popup
        if (ImGui::BeginPopup("AI Script Generator"))
        {
            static char description[512] = "";
            ImGui::Text("Describe what you want in plain English:");
            ImGui::InputTextMultiline("##description", description, sizeof(description));

            if (ImGui::Button("Generate Script"))
            {
                std::string generatedCode = AIHelper::GenerateScript(description);
                // Save to file and load
            }

            ImGui::EndPopup();
        }
    }
}
```

#### 5. AI Script Generator (2-3 hours)

**File**: `Editor/include/AIHelper.h`

Two implementation options:

**Option A: Template-Based (Simple)**
```cpp
class AIHelper
{
public:
    static std::string GenerateScript(const std::string& description)
    {
        // Pattern matching for common requests
        if (description.find("move") != std::string::npos &&
            description.find("arrow keys") != std::string::npos)
        {
            return GetTemplate("movement_arrows");
        }
        else if (description.find("jump") != std::string::npos)
        {
            return GetTemplate("jump");
        }
        // ... more patterns

        return GetTemplate("empty");
    }

private:
    static std::string GetTemplate(const std::string& name);
    static std::map<std::string, std::string> s_Templates;
};
```

**Option B: Claude API (Advanced)**
```cpp
class AIHelper
{
public:
    static std::string GenerateScript(const std::string& description)
    {
        // Call Claude API
        std::string prompt =
            "Generate a Lua script for a 2D game engine that does: " + description + "\n\n"
            "Available API:\n"
            "- Input.IsKeyHeld(keyName)\n"
            "- transform.Position.x/y\n"
            "- sprite.Color.r/g/b/a\n"
            "- rigidbody.Velocity.x/y\n\n"
            "Generate only the Lua script:";

        return CallClaudeAPI(prompt);
    }
};
```

#### 6. Example Scripts (30 minutes)

Create in `assets/scripts/examples/`:

- `movement.lua` - WASD movement
- `jump.lua` - Jumping mechanic
- `follow_mouse.lua` - Follow mouse cursor
- `rainbow.lua` - Rainbow color effect
- `shoot.lua` - Shooting mechanic
- `bounce.lua` - Bouncing behavior

## Phase 12: Polish & Production

### Features to Add

#### 1. Animation System (2-3 hours)

**Sprite Animation Component**:
```cpp
struct SpriteAnimationComponent
{
    std::vector<std::shared_ptr<Texture2D>> Frames;
    float FrameTime = 0.1f;
    bool Loop = true;
    bool Playing = true;

    // Runtime
    int CurrentFrame = 0;
    float Timer = 0.0f;
};
```

#### 2. Particle System (2-3 hours)

Simple particle emitter for effects:
- Explosions
- Fire
- Smoke
- Sparkles

#### 3. Tilemap Support (2-3 hours)

For level design:
- Load tilemap from JSON
- Render efficiently
- Collision detection

#### 4. Audio System (1-2 hours)

Re-visit Phase 7 with proper OpenAL setup:
- Sound effects
- Background music
- Volume control

#### 5. Build/Export System (1 hour)

Package game for distribution:
```bash
./package_game.sh MyGame
# Creates standalone executable + assets
```

#### 6. Profiler (1 hour)

Performance monitoring:
- FPS counter (already have)
- Memory usage
- Draw call breakdown
- Script performance

## Implementation Priority

### Must-Have (Core Scripting)
1. ✅ Lua integration
2. ⏳ ScriptComponent
3. ⏳ LuaScriptEngine
4. ⏳ Engine API bindings
5. ⏳ Script editor in Inspector

### Should-Have (Student-Friendly)
6. ⏳ AI Helper (template-based)
7. ⏳ Example scripts
8. ✅ Student guide

### Nice-to-Have (Polish)
9. ⏳ Animation system
10. ⏳ Particle system
11. ⏳ Audio system

## Time Estimates

**Phase 11 Core (Scripting)**:
- Component & Engine: 3-4 hours
- Editor integration: 1-2 hours
- AI Helper (templates): 1 hour
- Testing: 1 hour
- **Total: 6-8 hours**

**Phase 11 Advanced (AI)**:
- Claude API integration: 2-3 hours
- Prompt engineering: 1-2 hours
- **Total: 3-5 hours**

**Phase 12 (Polish)**:
- Animation: 2-3 hours
- Particles: 2-3 hours
- Audio: 1-2 hours
- Build system: 1 hour
- **Total: 6-9 hours**

## Next Steps

### For You (Teacher):
1. Review the student guide
2. Decide on AI Helper approach:
   - Template-based (simpler, no API needed)
   - Claude API (more powerful, requires API key)
3. Test with a small student group

### For Development:
1. Implement ScriptComponent
2. Create LuaScriptEngine
3. Add engine API bindings
4. Integrate into Editor
5. Add AI Helper
6. Create example scripts
7. Test thoroughly

## Student Workflow (Final Vision)

```
1. Student opens Editor
2. Creates entity (e.g., "Player")
3. Adds Sprite Renderer (picks color)
4. Adds Script component
5. Clicks "AI Helper"
6. Types: "Make player move with WASD keys"
7. AI generates Lua script
8. Student clicks "Apply"
9. Clicks "Play" button
10. Character moves! 🎉
```

## API Keys Needed (Optional)

For Claude API integration:
- Anthropic API key
- Set in environment: `export ANTHROPIC_API_KEY="your-key"`
- Or hardcode in config file (not recommended for production)

## Alternative: Local AI

Instead of Claude API, use:
- Template matching (simplest)
- Local LLM (llama.cpp, etc.)
- Pre-trained code completion model

## Files to Create

```
Engine/
├── include/Engine/Scripting/
│   └── LuaScriptEngine.h
├── src/Scripting/
│   └── LuaScriptEngine.cpp

Editor/
├── include/
│   └── AIHelper.h
└── src/
    └── AIHelper.cpp

assets/
└── scripts/
    ├── examples/
    │   ├── movement.lua
    │   ├── jump.lua
    │   ├── follow_mouse.lua
    │   ├── rainbow.lua
    │   └── shoot.lua
    └── templates/
        └── empty.lua
```

## Testing Checklist

- [ ] Lua scripts load correctly
- [ ] OnUpdate() function calls every frame
- [ ] Input API works from Lua
- [ ] Transform changes reflect in scene
- [ ] Scripts can be hot-reloaded
- [ ] AI Helper generates valid Lua
- [ ] Error messages are helpful
- [ ] Student guide examples work
- [ ] Scripts save/load with scenes
- [ ] Performance is acceptable (60 FPS)

## Success Metrics

Students should be able to:
- ✅ Create a moving character in < 5 minutes
- ✅ Use AI Helper without coding knowledge
- ✅ Understand generated Lua code
- ✅ Modify scripts confidently
- ✅ Create simple game in < 30 minutes
- ✅ Debug common issues independently

## Conclusion

You have a **solid foundation** with Phases 1-10. The student guide is ready, Lua is integrated, and you have a clear path to completion.

**Recommend Focus**:
1. Complete basic Lua scripting (6-8 hours)
2. Add template-based AI Helper (1 hour)
3. Test with students
4. Add advanced features based on feedback

**Your engine is already production-ready for teaching!** Students can use the visual editor + components system right now. Scripting adds programming education on top.
