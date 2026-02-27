#pragma once

#include <entt/entt.hpp>
#include <string>

namespace Engine {

class Entity; // Forward declaration

/**
 * @brief Scene class that owns the EnTT registry
 *
 * A Scene represents a game level or state. It owns all entities
 * and their components through the EnTT registry.
 *
 * Usage:
 *   Scene scene;
 *   Entity player = scene.CreateEntity("Player");
 *   player.AddComponent<TransformComponent>();
 */
class Scene
{
public:
    Scene();
    ~Scene();

    /**
     * @brief Create a new entity in this scene
     * @param name Optional name for the entity
     * @return Entity handle
     */
    Entity CreateEntity(const std::string& name = "Entity");

    /**
     * @brief Destroy an entity and all its components
     */
    void DestroyEntity(Entity entity);

    /**
     * @brief Update the scene (called every frame)
     */
    void OnUpdate(float deltaTime);

    /**
     * @brief Render the scene
     */
    void OnRender();

    /**
     * @brief Get the EnTT registry (for advanced usage)
     */
    entt::registry& GetRegistry() { return m_Registry; }
    const entt::registry& GetRegistry() const { return m_Registry; }

private:
    entt::registry m_Registry;

    friend class Entity;
};

} // namespace Engine
