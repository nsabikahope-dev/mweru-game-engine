#pragma once

namespace Engine {

class Scene;

/**
 * @brief Drives SpriteAnimationComponent frame playback.
 *
 * Lifecycle:
 *   AnimationSystem::OnSceneStart(scene) — load frame textures
 *   AnimationSystem::OnUpdate(scene, dt) — advance frames, swap textures
 *   AnimationSystem::OnSceneStop(scene)  — release runtime textures
 */
class AnimationSystem
{
public:
    static void OnSceneStart(Scene* scene);
    static void OnSceneStop(Scene* scene);
    static void OnUpdate(Scene* scene, float deltaTime);
};

} // namespace Engine
