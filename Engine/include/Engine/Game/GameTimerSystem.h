#pragma once

namespace Engine {

class Scene;

/**
 * @brief Updates all TimerComponent instances each frame.
 *
 * When a countdown timer expires it fires OnTimerExpired() on any Lua
 * script attached to the same entity.
 */
class GameTimerSystem
{
public:
    static void OnUpdate(Scene* scene, float dt);
};

} // namespace Engine
