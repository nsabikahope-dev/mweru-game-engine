#pragma once

#include <glm/glm.hpp>

namespace Engine {

class Scene;

/**
 * @brief CPU particle system using the Renderer2D batch renderer.
 *
 * Lifecycle:
 *   ParticleSystem::OnUpdate(scene, dt)   — emit + simulate
 *   ParticleSystem::OnRender(scene, vp)   — draw all active particles
 *   ParticleSystem::OnSceneStop(scene)    — reset all pools
 */
class ParticleSystem
{
public:
    static void OnUpdate(Scene* scene, float deltaTime);
    static void OnRender(Scene* scene, const glm::mat4& viewProjection);
    static void OnSceneStop(Scene* scene);
};

} // namespace Engine
