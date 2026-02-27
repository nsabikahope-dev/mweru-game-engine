#pragma once

#include <string>

namespace Engine {

class Scene;
class Entity;

/**
 * @brief Drives audio for every entity that has an AudioSourceComponent.
 *
 * Lifecycle mirrors PhysicsSystem:
 *   AudioSystem::OnSceneStart(scene)  — load clips, create AL sources
 *   AudioSystem::OnUpdate(scene)      — sync listener position
 *   AudioSystem::OnSceneStop(scene)   — stop & delete AL sources/buffers
 *
 * Per-entity control (also exposed to Lua scripts):
 *   AudioSystem::Play(entity)
 *   AudioSystem::Stop(entity)
 *   AudioSystem::Pause(entity)
 *   AudioSystem::SetVolume(entity, volume)
 */
class AudioSystem
{
public:
    static void OnSceneStart(Scene* scene);
    static void OnSceneStop(Scene* scene);
    static void OnUpdate(Scene* scene);

    // Per-entity helpers (also called from Lua bindings)
    static void Play(Entity entity);
    static void Stop(Entity entity);
    static void Pause(Entity entity);
    static void SetVolume(Entity entity, float volume);
    static float GetVolume(Entity entity);

private:
    // Load audio file → create AL buffer; returns buffer ID (0 on failure)
    static unsigned int LoadBuffer(const std::string& path);

    // Free AL source + buffer for a single entity
    static void FreeSource(Entity entity);
};

} // namespace Engine
