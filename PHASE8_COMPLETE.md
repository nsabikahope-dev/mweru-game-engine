# Phase 8: Scene Serialization (JSON) - COMPLETE ✓

## Overview
Successfully implemented JSON-based scene serialization, enabling saving and loading of complete scenes with all entities and components to human-readable .scene files.

## What Was Built

### 1. JSON Library Integration

**nlohmann/json** (v3.11.3)
- Single-header C++ JSON library
- Header-only, no compilation needed
- Modern C++ API with automatic type conversion
- Added to vendor/CMakeLists.txt as interface library

### 2. SceneSerializer Class

**SceneSerializer** ([Engine/include/Engine/Scene/SceneSerializer.h](Engine/include/Engine/Scene/SceneSerializer.h))

```cpp
// Save scene to file
SceneSerializer::Serialize(scene, "level1.scene");

// Load scene from file
SceneSerializer::Deserialize(scene, "level1.scene");

// Save/load to/from string
std::string json = SceneSerializer::SerializeToString(scene);
SceneSerializer::DeserializeFromString(scene, json);
```

**Features:**
- Saves entire scenes to JSON files
- Loads scenes from JSON files
- String-based serialization for debugging
- Pretty-printed JSON output (4-space indentation)
- Error handling with informative messages

### 3. Component Serialization

All engine components are now serializable:

**TagComponent:**
```json
"TagComponent": {
    "Tag": "Player"
}
```

**TransformComponent:**
```json
"TransformComponent": {
    "Position": [0.0, 5.0, 0.0],
    "Rotation": [0.0, 0.0, 0.0],
    "Scale": [1.0, 1.0, 1.0]
}
```

**SpriteRendererComponent:**
```json
"SpriteRendererComponent": {
    "Color": [1.0, 0.5, 0.2, 1.0],
    "TexturePath": "assets/player.png"
}
```

**CameraComponent:**
```json
"CameraComponent": {
    "Primary": true,
    "FixedAspectRatio": false,
    "OrthographicSize": 10.0,
    "OrthographicNear": -1.0,
    "OrthographicFar": 1.0
}
```

**RigidbodyComponent:**
```json
"RigidbodyComponent": {
    "Type": 2,
    "FixedRotation": false,
    "Mass": 1.0,
    "GravityScale": 1.0,
    "LinearDamping": 0.0,
    "AngularDamping": 0.01
}
```

**BoxColliderComponent:**
```json
"BoxColliderComponent": {
    "Size": [1.0, 1.0],
    "Offset": [0.0, 0.0],
    "Density": 1.0,
    "Friction": 0.3,
    "Restitution": 0.2
}
```

**CircleColliderComponent:**
```json
"CircleColliderComponent": {
    "Radius": 0.5,
    "Offset": [0.0, 0.0],
    "Density": 1.0,
    "Friction": 0.3,
    "Restitution": 0.8
}
```

### 4. Scene File Format

Complete `.scene` file example:

```json
{
    "Scene": "Untitled",
    "Entities": [
        {
            "Entity": 1,
            "TagComponent": {
                "Tag": "Ground"
            },
            "TransformComponent": {
                "Position": [0.0, -8.0, 0.0],
                "Rotation": [0.0, 0.0, 0.0],
                "Scale": [20.0, 1.0, 1.0]
            },
            "SpriteRendererComponent": {
                "Color": [0.3, 0.7, 0.3, 1.0],
                "TexturePath": ""
            },
            "RigidbodyComponent": {
                "Type": 0,
                "FixedRotation": false,
                "Mass": 1.0,
                "GravityScale": 1.0,
                "LinearDamping": 0.0,
                "AngularDamping": 0.01
            },
            "BoxColliderComponent": {
                "Size": [1.0, 1.0],
                "Offset": [0.0, 0.0],
                "Density": 1.0,
                "Friction": 0.5,
                "Restitution": 0.0
            }
        },
        {
            "Entity": 2,
            "TagComponent": {
                "Tag": "Box"
            },
            "TransformComponent": {
                "Position": [0.0, 5.0, 0.0],
                "Rotation": [0.0, 0.0, 0.0],
                "Scale": [1.0, 1.0, 1.0]
            },
            "SpriteRendererComponent": {
                "Color": [1.0, 0.3, 0.3, 1.0],
                "TexturePath": ""
            },
            "RigidbodyComponent": {
                "Type": 2,
                "FixedRotation": false,
                "Mass": 1.0,
                "GravityScale": 1.0,
                "LinearDamping": 0.0,
                "AngularDamping": 0.01
            },
            "BoxColliderComponent": {
                "Size": [1.0, 1.0],
                "Offset": [0.0, 0.0],
                "Density": 1.0,
                "Friction": 0.5,
                "Restitution": 0.2
            }
        }
    ]
}
```

### 5. Serialization Demo Application

**SerializationDemo.cpp** provides interactive save/load testing:

**Controls:**
- **S**: Save current scene to `saved_scene.scene`
- **L**: Load scene from `saved_scene.scene`
- **N**: Create new random scene
- **C**: Clear all entities
- **SPACE**: Spawn random box

**Workflow:**
1. Demo starts with default scene (ground, walls, boxes)
2. Press **S** to save scene to file
3. Press **C** to clear the scene entirely
4. Press **L** to restore the scene from file
5. Physics automatically reinitializes after loading

**Run with:**
```bash
./bin/SerializationDemo
```

## Usage Examples

### Save a Scene

```cpp
// Create scene with entities
auto scene = std::make_unique<Scene>();
auto player = scene->CreateEntity("Player");
player.AddComponent<TransformComponent>().Position = glm::vec3(0.0f, 5.0f, 0.0f);
player.AddComponent<SpriteRendererComponent>().Color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

// Save to file
SceneSerializer::Serialize(scene.get(), "levels/level1.scene");
```

### Load a Scene

```cpp
// Create empty scene
auto scene = std::make_unique<Scene>();

// Load from file
if (SceneSerializer::Deserialize(scene.get(), "levels/level1.scene"))
{
    // Reinitialize physics if needed
    PhysicsSystem::OnSceneStart(scene.get(), physicsWorld.get());

    // Find camera
    auto view = scene->GetRegistry().view<CameraComponent>();
    if (!view.empty())
    {
        camera = Entity(*view.begin(), scene.get());
    }
}
```

### Debug: Print Scene as JSON

```cpp
std::string json = SceneSerializer::SerializeToString(scene.get());
std::cout << json << "\n";
```

## Architecture Details

### Serialization Process

1. **Iterate Entities**: Query all entities with TagComponent
2. **Serialize Components**: For each entity, check which components it has
3. **Write JSON**: Convert component data to JSON objects
4. **Save File**: Write formatted JSON to .scene file

### Deserialization Process

1. **Parse JSON**: Load and parse .scene file
2. **Create Entities**: Create entities with names from TagComponent
3. **Add Components**: For each component in JSON, add to entity
4. **Set Properties**: Copy all property values from JSON

### Type Conversions

The serializer handles:
- **Vectors**: `glm::vec3` → `[x, y, z]`
- **Colors**: `glm::vec4` → `[r, g, b, a]`
- **Enums**: `RigidbodyComponent::BodyType` → `int`
- **Bools**: Direct JSON boolean
- **Floats**: Direct JSON number
- **Strings**: Direct JSON string

### Runtime vs Serialized Data

**Runtime data is NOT serialized:**
- `b2Body* RuntimeBody` in RigidbodyComponent
- `void* RuntimeFixture` in Collider components
- Physics bodies must be recreated after loading

**Why?**
- Runtime pointers are memory addresses, not meaningful data
- Physics state changes as simulation runs
- Scene files should represent initial state, not runtime state

## Integration with Existing Systems

### Physics Integration

After loading a scene with physics components:

```cpp
// Load scene
SceneSerializer::Deserialize(scene.get(), "scene.scene");

// Reinitialize physics (creates Box2D bodies from components)
PhysicsSystem::OnSceneStart(scene.get(), physicsWorld.get());
```

### Asset Integration

Texture paths are saved but not automatically loaded:

```json
"SpriteRendererComponent": {
    "TexturePath": "assets/player.png"
}
```

**Future enhancement:**
```cpp
if (!texturePath.empty())
{
    sprite.Texture = AssetManager::Load<Texture2D>(texturePath);
}
```

## Files Created/Modified

### New Files
- `Engine/include/Engine/Scene/SceneSerializer.h` - Serializer API
- `Engine/src/Scene/SceneSerializer.cpp` - Implementation
- `Sandbox/SerializationDemo.cpp` - Interactive demo
- `vendor/json.hpp` - nlohmann/json library

### Modified Files
- `vendor/CMakeLists.txt` - Added nlohmann_json interface library
- `Engine/CMakeLists.txt` - Linked nlohmann_json
- `Engine/include/Engine/Scene/Scene.h` - Added const GetRegistry()
- `Sandbox/CMakeLists.txt` - Added SerializationDemo executable

## Benefits

1. **Level Editing**: Save and load game levels
2. **Prefabs**: Save entity configurations as templates
3. **Version Control**: .scene files are text-based, git-friendly
4. **Debugging**: Human-readable format for inspection
5. **Collaboration**: Artists can edit scene files directly
6. **Hot Reload**: Reload scenes without restarting game
7. **Undo/Redo**: Save scene states for editor undo system

## Limitations & Future Work

**Current Limitations:**
- Texture paths saved but not auto-loaded (needs AssetManager integration)
- No UUID/GUID for entities (uses transient EnTT handles)
- No entity parent/child relationships
- No asset references (textures, materials, etc.)
- No custom component serialization

**Future Enhancements:**
- UUIDs for stable entity references
- Entity hierarchy (parent/child)
- Asset reference system
- Custom component serialization registration
- Binary serialization for faster loading
- Scene versioning for migration
- Partial scene updates (only changed entities)
- Scene templates/prefabs

## Verification

✓ nlohmann/json library integrated
✓ SceneSerializer saves scenes to JSON
✓ SceneSerializer loads scenes from JSON
✓ All components serialize correctly
✓ Physics components preserve properties
✓ Transforms, sprites, cameras restore properly
✓ SerializationDemo runs successfully
✓ Save→Clear→Load workflow works
✓ JSON format is human-readable
✓ Error handling functions correctly

## Next Phase

Ready to proceed to **Phase 9: ImGui Editor** when requested!

Phase 9 will be a BIG one - a visual editor with:
- Scene hierarchy panel
- Inspector for editing components
- Viewport with gizmos
- Asset browser
- Drag-and-drop workflow
- This is where the engine becomes truly usable!
