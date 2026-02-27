#pragma once

#include <string>

namespace Engine {

class Scene;

/**
 * @brief Serializes and deserializes scenes to/from JSON files
 *
 * Saves entire scenes with all entities and components to .scene files.
 * Supports all engine components: Transform, Sprite, Camera, Rigidbody, Colliders, etc.
 *
 * Usage:
 *   // Save scene
 *   SceneSerializer::Serialize(scene, "levels/level1.scene");
 *
 *   // Load scene
 *   auto scene = std::make_unique<Scene>();
 *   SceneSerializer::Deserialize(scene.get(), "levels/level1.scene");
 */
class SceneSerializer
{
public:
    /**
     * @brief Serialize a scene to a JSON file
     * @param scene Scene to serialize
     * @param filepath Output file path
     * @return true if successful, false otherwise
     */
    static bool Serialize(const Scene* scene, const std::string& filepath);

    /**
     * @brief Deserialize a scene from a JSON file
     * @param scene Scene to deserialize into (should be empty)
     * @param filepath Input file path
     * @return true if successful, false otherwise
     */
    static bool Deserialize(Scene* scene, const std::string& filepath);

    /**
     * @brief Serialize a scene to a JSON string
     * @param scene Scene to serialize
     * @return JSON string representation
     */
    static std::string SerializeToString(const Scene* scene);

    /**
     * @brief Deserialize a scene from a JSON string
     * @param scene Scene to deserialize into
     * @param jsonString JSON string
     * @return true if successful, false otherwise
     */
    static bool DeserializeFromString(Scene* scene, const std::string& jsonString);
};

} // namespace Engine
