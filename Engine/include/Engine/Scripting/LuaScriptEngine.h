#pragma once

#include <string>

namespace Engine {

class Scene;
class Entity;

enum class GameState { Playing, Paused, Won, Lost };

/**
 * @brief Lua scripting engine
 *
 * Manages per-entity Lua states and drives script lifecycle.
 *
 * Each entity with a ScriptComponent gets an isolated sol::state. The engine
 * exposes a simple, student-friendly API into every state:
 *
 *   Input table:
 *     Input.IsKeyHeld("W")        -- bool
 *     Input.IsKeyPressed("Space") -- bool, true only on the press frame
 *     Input.IsKeyReleased("A")    -- bool
 *     Input.GetMouseX()           -- float (pixels)
 *     Input.GetMouseY()           -- float (pixels)
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
 *   Timer (requires TimerComponent):
 *     StartTimer()          -- reset and activate
 *     StopTimer()           -- pause the timer
 *     ResetTimer()          -- set elapsed to 0
 *     GetTimerElapsed()     -> float
 *     GetTimerRemaining()   -> float
 *     IsTimerExpired()      -> bool
 *
 *   Game state (global):
 *     Game.Pause()          -- freeze game updates
 *     Game.Resume()         -- resume from pause
 *     Game.Win()            -- trigger win screen
 *     Game.Lose()           -- trigger lose screen
 *     Game.IsPaused()       -> bool
 *     Game.IsWon()          -> bool
 *     Game.IsLost()         -> bool
 *     Game.GetState()       -> "playing"|"paused"|"won"|"lost"
 *     Game.LoadLevel(n)     -- transition to level n
 *                              (loads levels/levelN.scene; web fetches /api/games/<id>/scenes/N)
 *     Game.SubmitScore(n)   -> (web only) POST score to backend
 *
 *   Video (web: HTML5 <video> overlay; desktop: no-op):
 *     PlayVideo(url)        -- load and play a video
 *     PauseVideo()          -- pause playback
 *     StopVideo()           -- stop and hide video
 *     IsVideoPlaying()      -> bool
 *     SetVideoLoop(bool)    -- toggle looping
 *
 *   Utility:
 *     Log(message)
 *
 * Scripts define optional callbacks:
 *   function OnStart()           -- called once when the scene starts playing
 *   function OnUpdate(dt)        -- called every frame, dt = delta time in seconds
 *   function OnCollision(name)   -- called when this entity's physics body touches another
 *   function OnTimerExpired()    -- called when this entity's TimerComponent expires
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

    /**
     * @brief Called by PhysicsSystem when two bodies begin touching.
     * Calls OnCollision(otherName) in the Lua script attached to entityID.
     */
    static void FireCollision(Scene* scene, uint32_t entityID, const std::string& otherName);

    /**
     * @brief Fire a named Lua callback on a specific entity (e.g. "OnTimerExpired").
     * Silent if the function doesn't exist in that entity's script.
     */
    static void FireEvent(Scene* scene, Entity entity, const std::string& fnName);

    // -------------------------------------------------------------------------
    // Global game state — readable/writable by Lua scripts via the Game table
    // -------------------------------------------------------------------------
    static GameState GetGameState();
    static void      SetGameState(GameState s);
    static void      ResetGameState();   // -> Playing

    // -------------------------------------------------------------------------
    // Level management — polled each frame by RuntimeApp / WebRuntimeApp
    // -------------------------------------------------------------------------
    static bool HasPendingLevel();
    static int  GetAndClearPendingLevel(); // returns -1 if none pending
};

} // namespace Engine
