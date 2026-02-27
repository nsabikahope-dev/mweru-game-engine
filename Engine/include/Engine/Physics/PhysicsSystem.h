#pragma once

#include "Engine/Physics/PhysicsWorld.h"
#include <entt/entt.hpp>

namespace Engine {

class Scene;

/**
 * @brief Physics system for syncing ECS with Box2D
 *
 * Responsibilities:
 * - Create Box2D bodies from Rigidbody components
 * - Create Box2D fixtures from Collider components
 * - Sync Transform <-> Box2D body positions/rotations
 * - Clean up Box2D objects when entities are destroyed
 *
 * Usage:
 *   PhysicsSystem::OnSceneStart(scene, physicsWorld);  // Initialize bodies
 *   PhysicsSystem::OnUpdate(scene, physicsWorld);       // Sync transforms
 *   PhysicsSystem::OnSceneStop(scene);                  // Cleanup
 */
class PhysicsSystem
{
public:
    /**
     * @brief Initialize physics bodies for all entities with Rigidbody components
     * Call this when scene starts
     */
    static void OnSceneStart(Scene* scene, PhysicsWorld* physicsWorld);

    /**
     * @brief Update physics simulation and sync transforms
     * Call this every frame before rendering
     */
    static void OnUpdate(Scene* scene, PhysicsWorld* physicsWorld, float deltaTime);

    /**
     * @brief Clean up all physics bodies
     * Call this when scene stops
     */
    static void OnSceneStop(Scene* scene);

private:
    static void CreateBody(Scene* scene, entt::entity entity, PhysicsWorld* physicsWorld);
    static void DestroyBody(Scene* scene, entt::entity entity);
    static void SyncTransformToPhysics(Scene* scene);
    static void SyncPhysicsToTransform(Scene* scene);
};

} // namespace Engine
