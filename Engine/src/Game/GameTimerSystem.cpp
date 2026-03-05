#include <Engine/Game/GameTimerSystem.h>
#include <Engine/Scene/Scene.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Entity.h>
#include <Engine/Scripting/LuaScriptEngine.h>

namespace Engine {

void GameTimerSystem::OnUpdate(Scene* scene, float dt)
{
    auto view = scene->GetRegistry().view<TimerComponent>();
    for (auto handle : view)
    {
        auto& t = view.get<TimerComponent>(handle);
        if (!t.Active || t.Expired)
            continue;

        t.Elapsed += dt;

        // Check expiry for countdown timers (duration > 0)
        if (t.CountDown && t.Duration > 0.0f && t.Elapsed >= t.Duration)
        {
            t.Elapsed  = t.Loop ? 0.0f : t.Duration;
            t.Expired  = !t.Loop;
            t.Active   = t.Loop;

            // Fire Lua callback on the same entity
            Entity entity(handle, scene);
            LuaScriptEngine::FireEvent(scene, entity, "OnTimerExpired");
        }
    }
}

} // namespace Engine
