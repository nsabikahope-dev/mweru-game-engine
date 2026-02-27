#pragma once

#include <glm/glm.hpp>
#include <memory>

// Forward declarations to avoid exposing Box2D in header
class b2World;
class b2Body;

namespace Engine {

/**
 * @brief Physics world wrapper for Box2D
 *
 * Manages the Box2D physics simulation world, including:
 * - Gravity settings
 * - Fixed timestep simulation
 * - Debug rendering (future)
 *
 * Usage:
 *   PhysicsWorld physics(glm::vec2(0.0f, -9.8f));  // Gravity down
 *   physics.Step(deltaTime);
 */
class PhysicsWorld
{
public:
    PhysicsWorld(const glm::vec2& gravity = glm::vec2(0.0f, -9.8f));
    ~PhysicsWorld();

    /**
     * @brief Step the physics simulation
     * Uses fixed timestep accumulator for stability
     * @param deltaTime Time since last frame in seconds
     */
    void Step(float deltaTime);

    /**
     * @brief Set gravity
     */
    void SetGravity(const glm::vec2& gravity);

    /**
     * @brief Get gravity
     */
    glm::vec2 GetGravity() const;

    /**
     * @brief Get the Box2D world (for advanced usage)
     */
    b2World* GetNativeWorld() { return m_World.get(); }

private:
    std::unique_ptr<b2World> m_World;

    // Fixed timestep accumulator
    static constexpr float s_FixedTimestep = 1.0f / 60.0f;  // 60 FPS physics
    float m_Accumulator = 0.0f;

    // Box2D simulation parameters
    static constexpr int s_VelocityIterations = 8;
    static constexpr int s_PositionIterations = 3;
};

} // namespace Engine
