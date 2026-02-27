#pragma once

#include <string>

namespace Engine {

class Scene;
class Entity;

/**
 * @brief Lua scripting engine
 *
 * Manages per-entity Lua states and drives script lifecycle.
 *
 * Each entity with a ScriptComponent gets an isolated sol::state. The engine
 * exposes a simple, student-friendly API into every state:
 *
 *   Input table:
 *     Input.IsKeyHeld("W")       -- bool
 *     Input.IsKeyPressed("Space") -- bool, true only on the press frame
 *     Input.IsKeyReleased("A")   -- bool
 *     Input.GetMouseX()          -- float (pixels)
 *     Input.GetMouseY()          -- float (pixels)
 *
 *   Transform:
 *     GetPosition()  -> x, y, z
 *     SetPosition(x, y, z)
 *     GetScale()     -> x, y, z
 *     SetScale(x, y, z)
 *     GetRotation()  -> z  (radians)
 *     SetRotation(z)
 *
 *   Sprite:
 *     GetColor()  -> r, g, b, a
 *     SetColor(r, g, b, a)
 *
 *   Physics (requires Rigidbody):
 *     GetVelocity()  -> x, y
 *     SetVelocity(x, y)
 *     ApplyForce(x, y)
 *     ApplyImpulse(x, y)
 *
 *   Utility:
 *     Log(message)
 *
 * Scripts define two optional callbacks:
 *   function OnStart()        -- called once when the scene starts playing
 *   function OnUpdate(dt)     -- called every frame, dt = delta time in seconds
 */
class LuaScriptEngine
{
public:
    static void Init();
    static void Shutdown();

    /**
     * @brief Load (or reload) the script file for a single entity.
     * Creates a fresh Lua state, binds the engine API, and executes the file.
     */
    static void LoadScript(Scene* scene, Entity entity, const std::string& scriptPath);

    /**
     * @brief Reload the script currently attached to the entity (uses ScriptComponent path).
     */
    static void ReloadScript(Scene* scene, Entity entity);

    /**
     * @brief Destroy the Lua state for this entity.
     */
    static void UnloadScript(Entity entity);

    /**
     * @brief Called when the scene starts playing.
     * Loads all ScriptComponents and calls OnStart() on each.
     */
    static void OnSceneStart(Scene* scene);

    /**
     * @brief Called when the scene stops playing.
     * Destroys all Lua states for this scene's entities.
     */
    static void OnSceneStop(Scene* scene);

    /**
     * @brief Call OnUpdate(dt) on every enabled, loaded script.
     */
    static void OnUpdate(Scene* scene, float deltaTime);
};

} // namespace Engine
