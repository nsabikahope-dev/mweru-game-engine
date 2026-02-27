#pragma once

#include <entt/entt.hpp>
#include <cassert>

// Include Scene.h for template method implementations
// This is safe because Scene.h only forward declares Entity
#include "Engine/Scene/Scene.h"

namespace Engine {

/**
 * @brief Type-safe wrapper around entt::entity
 *
 * Entity is a lightweight handle to an entity in a Scene.
 * It provides a convenient API for adding, removing, and getting components.
 *
 * Usage:
 *   Entity entity = scene.CreateEntity("Player");
 *   entity.AddComponent<TransformComponent>(glm::vec3(0.0f));
 *   auto& transform = entity.GetComponent<TransformComponent>();
 */
class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);
    Entity(const Entity& other) = default;

    /**
     * @brief Add a component to this entity
     * @tparam T Component type
     * @tparam Args Constructor arguments
     * @return Reference to the added component
     */
    template<typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        assert(!HasComponent<T>() && "Entity already has component!");
        return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    /**
     * @brief Get a component from this entity
     * @tparam T Component type
     * @return Reference to the component
     */
    template<typename T>
    T& GetComponent()
    {
        assert(HasComponent<T>() && "Entity does not have component!");
        return m_Scene->m_Registry.get<T>(m_EntityHandle);
    }

    /**
     * @brief Check if this entity has a component
     * @tparam T Component type
     * @return True if the entity has the component
     */
    template<typename T>
    bool HasComponent()
    {
        return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
    }

    /**
     * @brief Remove a component from this entity
     * @tparam T Component type
     */
    template<typename T>
    void RemoveComponent()
    {
        assert(HasComponent<T>() && "Entity does not have component!");
        m_Scene->m_Registry.remove<T>(m_EntityHandle);
    }

    /**
     * @brief Check if this entity is valid
     */
    operator bool() const { return m_EntityHandle != entt::null && m_Scene != nullptr; }

    /**
     * @brief Get the raw entt::entity handle
     */
    operator entt::entity() const { return m_EntityHandle; }

    /**
     * @brief Get the entity ID (for debugging)
     */
    uint32_t GetID() const { return static_cast<uint32_t>(m_EntityHandle); }

    /**
     * @brief Equality operators
     */
    bool operator==(const Entity& other) const
    {
        return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
    }

    bool operator!=(const Entity& other) const
    {
        return !(*this == other);
    }

private:
    entt::entity m_EntityHandle{ entt::null };
    Scene* m_Scene = nullptr;
};

} // namespace Engine
