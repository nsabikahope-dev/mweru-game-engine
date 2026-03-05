#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "Engine/Scripting/LuaScriptEngine.h"
#include "Engine/ECS/Components.h"
#include "Engine/ECS/Entity.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Input/Input.h"
#include "Engine/Audio/AudioSystem.h"

#include <box2d/box2d.h>
#include <AL/al.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <unordered_map>
#include <iostream>

namespace Engine {

// -----------------------------------------------------------------------
// Internal state: one sol::state per entity (keyed by entity uint32_t ID)
// -----------------------------------------------------------------------
static std::unordered_map<uint32_t, std::unique_ptr<sol::state>> s_States;
static GameState s_GameState        = GameState::Playing;
static int       s_PendingLevelIndex = -1; // -1 = no pending level load

// -----------------------------------------------------------------------
// Key name -> KeyCode mapping for student-friendly string-based input
// -----------------------------------------------------------------------
static KeyCode StringToKeyCode(const std::string& key)
{
    static const std::unordered_map<std::string, KeyCode> s_KeyMap = {
        // Alphabet
        {"A", KeyCode::A}, {"B", KeyCode::B}, {"C", KeyCode::C}, {"D", KeyCode::D},
        {"E", KeyCode::E}, {"F", KeyCode::F}, {"G", KeyCode::G}, {"H", KeyCode::H},
        {"I", KeyCode::I}, {"J", KeyCode::J}, {"K", KeyCode::K}, {"L", KeyCode::L},
        {"M", KeyCode::M}, {"N", KeyCode::N}, {"O", KeyCode::O}, {"P", KeyCode::P},
        {"Q", KeyCode::Q}, {"R", KeyCode::R}, {"S", KeyCode::S}, {"T", KeyCode::T},
        {"U", KeyCode::U}, {"V", KeyCode::V}, {"W", KeyCode::W}, {"X", KeyCode::X},
        {"Y", KeyCode::Y}, {"Z", KeyCode::Z},
        // Arrow keys
        {"Up",    KeyCode::Up},    {"Down",  KeyCode::Down},
        {"Left",  KeyCode::Left},  {"Right", KeyCode::Right},
        // Common specials
        {"Space",     KeyCode::Space},
        {"Escape",    KeyCode::Escape},
        {"Enter",     KeyCode::Enter},
        {"Tab",       KeyCode::Tab},
        {"Backspace", KeyCode::Backspace},
        {"Shift",     KeyCode::LeftShift},
        {"Ctrl",      KeyCode::LeftControl},
        {"Alt",       KeyCode::LeftAlt},
        // Number row
        {"0", KeyCode::Num0}, {"1", KeyCode::Num1}, {"2", KeyCode::Num2},
        {"3", KeyCode::Num3}, {"4", KeyCode::Num4}, {"5", KeyCode::Num5},
        {"6", KeyCode::Num6}, {"7", KeyCode::Num7}, {"8", KeyCode::Num8},
        {"9", KeyCode::Num9},
    };

    auto it = s_KeyMap.find(key);
    if (it != s_KeyMap.end())
        return it->second;

    std::cerr << "[LuaScriptEngine] Unknown key name: '" << key << "'\n";
    return static_cast<KeyCode>(0);
}

// -----------------------------------------------------------------------
// Build a fully-configured sol::state for one entity
// -----------------------------------------------------------------------
static void SetupLuaState(sol::state& lua, Scene* scene, Entity entity)
{
    lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table
    );

    // --- Input table ---------------------------------------------------
    sol::table input = lua.create_named_table("Input");

    input.set_function("IsKeyHeld", [](const std::string& key) {
        return Input::IsKeyHeld(StringToKeyCode(key));
    });
    input.set_function("IsKeyPressed", [](const std::string& key) {
        return Input::IsKeyPressed(StringToKeyCode(key));
    });
    input.set_function("IsKeyReleased", [](const std::string& key) {
        return Input::IsKeyReleased(StringToKeyCode(key));
    });
    input.set_function("GetMouseX", []() {
        return Input::GetMousePosition().x;
    });
    input.set_function("GetMouseY", []() {
        return Input::GetMousePosition().y;
    });
    input.set_function("IsMouseButtonHeld", [](int button) {
        return Input::IsMouseButtonHeld(static_cast<MouseButton>(button));
    });

    // --- Utility -------------------------------------------------------
    lua.set_function("Log", [](const std::string& msg) {
        std::cout << "[Lua] " << msg << "\n";
    });

    // --- Transform functions -------------------------------------------
    lua.set_function("GetPosition", [entity]() mutable -> std::tuple<float, float, float> {
        if (entity.HasComponent<TransformComponent>()) {
            auto& t = entity.GetComponent<TransformComponent>();
            return { t.Position.x, t.Position.y, t.Position.z };
        }
        return { 0.0f, 0.0f, 0.0f };
    });

    lua.set_function("SetPosition", [entity](float x, float y, float z) mutable {
        if (entity.HasComponent<TransformComponent>()) {
            auto& t = entity.GetComponent<TransformComponent>();
            t.Position = { x, y, z };
            // Keep physics body in sync for kinematic bodies
            if (entity.HasComponent<RigidbodyComponent>()) {
                auto& rb = entity.GetComponent<RigidbodyComponent>();
                if (rb.RuntimeBody)
                    rb.RuntimeBody->SetTransform({ x, y }, rb.RuntimeBody->GetAngle());
            }
        }
    });

    lua.set_function("GetScale", [entity]() mutable -> std::tuple<float, float, float> {
        if (entity.HasComponent<TransformComponent>()) {
            auto& t = entity.GetComponent<TransformComponent>();
            return { t.Scale.x, t.Scale.y, t.Scale.z };
        }
        return { 1.0f, 1.0f, 1.0f };
    });

    lua.set_function("SetScale", [entity](float x, float y, float z) mutable {
        if (entity.HasComponent<TransformComponent>())
            entity.GetComponent<TransformComponent>().Scale = { x, y, z };
    });

    lua.set_function("GetRotation", [entity]() mutable -> float {
        if (entity.HasComponent<TransformComponent>())
            return entity.GetComponent<TransformComponent>().Rotation.z;
        return 0.0f;
    });

    lua.set_function("SetRotation", [entity](float z) mutable {
        if (entity.HasComponent<TransformComponent>())
            entity.GetComponent<TransformComponent>().Rotation.z = z;
    });

    // --- Sprite color --------------------------------------------------
    lua.set_function("GetColor", [entity]() mutable -> std::tuple<float, float, float, float> {
        if (entity.HasComponent<SpriteRendererComponent>()) {
            auto& s = entity.GetComponent<SpriteRendererComponent>();
            return { s.Color.r, s.Color.g, s.Color.b, s.Color.a };
        }
        return { 1.0f, 1.0f, 1.0f, 1.0f };
    });

    lua.set_function("SetColor", [entity](float r, float g, float b, float a) mutable {
        if (entity.HasComponent<SpriteRendererComponent>())
            entity.GetComponent<SpriteRendererComponent>().Color = { r, g, b, a };
    });

    // --- Physics (only useful when scene is playing) -------------------
    lua.set_function("GetVelocity", [entity]() mutable -> std::tuple<float, float> {
        if (entity.HasComponent<RigidbodyComponent>()) {
            auto& rb = entity.GetComponent<RigidbodyComponent>();
            if (rb.RuntimeBody) {
                b2Vec2 v = rb.RuntimeBody->GetLinearVelocity();
                return { v.x, v.y };
            }
        }
        return { 0.0f, 0.0f };
    });

    lua.set_function("SetVelocity", [entity](float x, float y) mutable {
        if (entity.HasComponent<RigidbodyComponent>()) {
            auto& rb = entity.GetComponent<RigidbodyComponent>();
            if (rb.RuntimeBody)
                rb.RuntimeBody->SetLinearVelocity({ x, y });
        }
    });

    lua.set_function("ApplyForce", [entity](float x, float y) mutable {
        if (entity.HasComponent<RigidbodyComponent>()) {
            auto& rb = entity.GetComponent<RigidbodyComponent>();
            if (rb.RuntimeBody)
                rb.RuntimeBody->ApplyForceToCenter({ x, y }, true);
        }
    });

    lua.set_function("ApplyImpulse", [entity](float x, float y) mutable {
        if (entity.HasComponent<RigidbodyComponent>()) {
            auto& rb = entity.GetComponent<RigidbodyComponent>();
            if (rb.RuntimeBody)
                rb.RuntimeBody->ApplyLinearImpulseToCenter({ x, y }, true);
        }
    });

    // --- Particle functions --------------------------------------------
    lua.set_function("SetEmitting", [entity](bool active) mutable {
        if (entity.HasComponent<ParticleEmitterComponent>())
            entity.GetComponent<ParticleEmitterComponent>().Emitting = active;
    });
    lua.set_function("IsEmitting", [entity]() mutable -> bool {
        if (entity.HasComponent<ParticleEmitterComponent>())
            return entity.GetComponent<ParticleEmitterComponent>().Emitting;
        return false;
    });

    // --- Audio functions -----------------------------------------------
    lua.set_function("PlayAudio",  [entity]() mutable { AudioSystem::Play(entity);  });
    lua.set_function("StopAudio",  [entity]() mutable { AudioSystem::Stop(entity);  });
    lua.set_function("PauseAudio", [entity]() mutable { AudioSystem::Pause(entity); });
    lua.set_function("SetVolume",  [entity](float v) mutable { AudioSystem::SetVolume(entity, v); });
    lua.set_function("GetVolume",  [entity]() mutable -> float { return AudioSystem::GetVolume(entity); });

    // --- Text functions ------------------------------------------------
    lua.set_function("SetText", [entity](const std::string& text) mutable {
        if (entity.HasComponent<TextComponent>())
            entity.GetComponent<TextComponent>().Text = text;
    });
    lua.set_function("GetText", [entity]() mutable -> std::string {
        if (entity.HasComponent<TextComponent>())
            return entity.GetComponent<TextComponent>().Text;
        return "";
    });
    lua.set_function("SetTextColor", [entity](float r, float g, float b, float a) mutable {
        if (entity.HasComponent<TextComponent>())
            entity.GetComponent<TextComponent>().Color = { r, g, b, a };
    });
    lua.set_function("SetTextVisible", [entity](bool v) mutable {
        if (entity.HasComponent<TextComponent>())
            entity.GetComponent<TextComponent>().Visible = v;
    });

    // --- Dialogue functions --------------------------------------------
    lua.set_function("StartDialogue", [entity]() mutable {
        if (entity.HasComponent<DialogueComponent>()) {
            auto& d = entity.GetComponent<DialogueComponent>();
            d.CurrentLine = 0;
            d.Active = true;
            d.AutoAdvanceTimer = 0.0f;
        }
    });
    lua.set_function("AdvanceDialogue", [entity]() mutable {
        if (entity.HasComponent<DialogueComponent>()) {
            auto& d = entity.GetComponent<DialogueComponent>();
            if (!d.Active) return;
            d.CurrentLine++;
            if (d.CurrentLine >= (int)d.Lines.size()) {
                d.Active = false;
                d.CurrentLine = 0;
            }
        }
    });
    lua.set_function("IsDialogueActive", [entity]() mutable -> bool {
        if (entity.HasComponent<DialogueComponent>())
            return entity.GetComponent<DialogueComponent>().Active;
        return false;
    });
    lua.set_function("GetDialogueSpeaker", [entity]() mutable -> std::string {
        if (entity.HasComponent<DialogueComponent>()) {
            auto& d = entity.GetComponent<DialogueComponent>();
            if (d.Active && d.CurrentLine < (int)d.Lines.size())
                return d.Lines[(size_t)d.CurrentLine].Speaker;
        }
        return "";
    });
    lua.set_function("GetDialogueText", [entity]() mutable -> std::string {
        if (entity.HasComponent<DialogueComponent>()) {
            auto& d = entity.GetComponent<DialogueComponent>();
            if (d.Active && d.CurrentLine < (int)d.Lines.size())
                return d.Lines[(size_t)d.CurrentLine].Text;
        }
        return "";
    });

    // --- Scene table: control OTHER entities by name ----------------------
    // Usage in Lua:
    //   Scene.SetText("ScoreLabel", "Score: 10")
    //   local x, y, z = Scene.GetPosition("Enemy")
    //   Scene.SetPosition("Bullet", x, y + 1, 0)
    //   Scene.SetVisible("Overlay", false)
    //   Scene.SetColor("Button", 0, 1, 0, 1)
    //   if Scene.Exists("Boss") then ... end
    sol::table sceneApi = lua.create_named_table("Scene");

    sceneApi.set_function("GetPosition",
        [scene](const std::string& name) -> std::tuple<float, float, float>
    {
        auto view = scene->GetRegistry().view<TagComponent, TransformComponent>();
        for (auto h : view) {
            if (view.get<TagComponent>(h).Tag == name) {
                auto& tf = view.get<TransformComponent>(h);
                return { tf.Position.x, tf.Position.y, tf.Position.z };
            }
        }
        return { 0.f, 0.f, 0.f };
    });

    sceneApi.set_function("SetPosition",
        [scene](const std::string& name, float x, float y, float z)
    {
        auto view = scene->GetRegistry().view<TagComponent, TransformComponent>();
        for (auto h : view) {
            if (view.get<TagComponent>(h).Tag == name) {
                auto& tf = view.get<TransformComponent>(h);
                tf.Position = { x, y, z };
                // Sync kinematic/dynamic physics body if present
                auto& reg = scene->GetRegistry();
                if (reg.all_of<RigidbodyComponent>(h)) {
                    auto& rb = reg.get<RigidbodyComponent>(h);
                    if (rb.RuntimeBody)
                        rb.RuntimeBody->SetTransform({ x, y }, rb.RuntimeBody->GetAngle());
                }
                return;
            }
        }
    });

    sceneApi.set_function("SetVisible",
        [scene](const std::string& name, bool visible)
    {
        auto& reg = scene->GetRegistry();
        auto view  = reg.view<TagComponent>();
        for (auto h : view) {
            if (reg.get<TagComponent>(h).Tag != name) continue;
            if (reg.all_of<SpriteRendererComponent>(h))
                reg.get<SpriteRendererComponent>(h).Color.a = visible ? 1.0f : 0.0f;
            if (reg.all_of<TextComponent>(h))
                reg.get<TextComponent>(h).Visible = visible;
            return;
        }
    });

    sceneApi.set_function("SetColor",
        [scene](const std::string& name, float r, float g, float b, float a)
    {
        auto view = scene->GetRegistry().view<TagComponent, SpriteRendererComponent>();
        for (auto h : view) {
            if (view.get<TagComponent>(h).Tag == name) {
                view.get<SpriteRendererComponent>(h).Color = { r, g, b, a };
                return;
            }
        }
    });

    sceneApi.set_function("SetText",
        [scene](const std::string& name, const std::string& text)
    {
        auto view = scene->GetRegistry().view<TagComponent, TextComponent>();
        for (auto h : view) {
            if (view.get<TagComponent>(h).Tag == name) {
                view.get<TextComponent>(h).Text = text;
                return;
            }
        }
    });

    sceneApi.set_function("GetText",
        [scene](const std::string& name) -> std::string
    {
        auto view = scene->GetRegistry().view<TagComponent, TextComponent>();
        for (auto h : view) {
            if (view.get<TagComponent>(h).Tag == name)
                return view.get<TextComponent>(h).Text;
        }
        return "";
    });

    sceneApi.set_function("Exists",
        [scene](const std::string& name) -> bool
    {
        auto view = scene->GetRegistry().view<TagComponent>();
        for (auto h : view) {
            if (view.get<TagComponent>(h).Tag == name) return true;
        }
        return false;
    });

    sceneApi.set_function("SetEmitting",
        [scene](const std::string& name, bool active)
    {
        auto& reg = scene->GetRegistry();
        auto view  = reg.view<TagComponent, ParticleEmitterComponent>();
        for (auto h : view) {
            if (view.get<TagComponent>(h).Tag == name) {
                view.get<ParticleEmitterComponent>(h).Emitting = active;
                return;
            }
        }
    });

    // --- Timer functions (operate on this entity's TimerComponent) --------
    lua.set_function("StartTimer", [entity]() mutable {
        if (entity.HasComponent<TimerComponent>()) {
            auto& t = entity.GetComponent<TimerComponent>();
            t.Elapsed  = 0.0f;
            t.Expired  = false;
            t.Active   = true;
        }
    });
    lua.set_function("StopTimer", [entity]() mutable {
        if (entity.HasComponent<TimerComponent>())
            entity.GetComponent<TimerComponent>().Active = false;
    });
    lua.set_function("ResetTimer", [entity]() mutable {
        if (entity.HasComponent<TimerComponent>()) {
            auto& t = entity.GetComponent<TimerComponent>();
            t.Elapsed  = 0.0f;
            t.Expired  = false;
        }
    });
    lua.set_function("GetTimerElapsed", [entity]() mutable -> float {
        if (entity.HasComponent<TimerComponent>())
            return entity.GetComponent<TimerComponent>().Elapsed;
        return 0.0f;
    });
    lua.set_function("GetTimerRemaining", [entity]() mutable -> float {
        if (entity.HasComponent<TimerComponent>())
            return entity.GetComponent<TimerComponent>().GetRemaining();
        return 0.0f;
    });
    lua.set_function("IsTimerExpired", [entity]() mutable -> bool {
        if (entity.HasComponent<TimerComponent>())
            return entity.GetComponent<TimerComponent>().Expired;
        return false;
    });

    // --- Game state table (global, shared across all entity scripts) ------
    sol::table game = lua.create_named_table("Game");

    game.set_function("Pause",    []() { s_GameState = GameState::Paused;  });
    game.set_function("Resume",   []() { s_GameState = GameState::Playing; });
    game.set_function("Win",      []() { s_GameState = GameState::Won;     });
    game.set_function("Lose",     []() { s_GameState = GameState::Lost;    });
    game.set_function("IsPaused", []() -> bool { return s_GameState == GameState::Paused; });
    game.set_function("IsWon",    []() -> bool { return s_GameState == GameState::Won;    });
    game.set_function("IsLost",   []() -> bool { return s_GameState == GameState::Lost;   });
    game.set_function("GetState", []() -> std::string {
        switch (s_GameState) {
            case GameState::Playing: return "playing";
            case GameState::Paused:  return "paused";
            case GameState::Won:     return "won";
            case GameState::Lost:    return "lost";
        }
        return "playing";
    });
    // Game.SubmitScore — no-op in native; uses fetch() in WASM build
    game.set_function("SubmitScore", [](int score) {
#ifdef __EMSCRIPTEN__
        EM_ASM({ if (window._submitScore) window._submitScore($0); }, score);
#else
        (void)score;
#endif
    });

    // Game.LoadLevel(n) — queue a level transition; polled by the runtime each frame
    // Desktop: loads "levels/levelN.scene"
    // Web:     fetches "/api/games/<gameId>/scenes/N"
    game.set_function("LoadLevel", [](int n) {
        s_PendingLevelIndex = n;
    });

    // --- Video (HTML5 overlay on web; console log on desktop) ---------------
    lua.set_function("PlayVideo", [entity](const std::string& url) mutable {
        if (entity.HasComponent<VideoComponent>())
            entity.GetComponent<VideoComponent>().VideoUrl = url;
#ifdef __EMSCRIPTEN__
        EM_ASM({
            if (window._videoPlay) window._videoPlay(UTF8ToString($0));
        }, url.c_str());
#else
        std::cout << "[Video] PlayVideo: " << url << "\n";
#endif
    });

    lua.set_function("PauseVideo", []() {
#ifdef __EMSCRIPTEN__
        EM_ASM({ if (window._videoPause) window._videoPause(); });
#else
        std::cout << "[Video] PauseVideo\n";
#endif
    });

    lua.set_function("StopVideo", [entity]() mutable {
        if (entity.HasComponent<VideoComponent>())
            entity.GetComponent<VideoComponent>().Visible = false;
#ifdef __EMSCRIPTEN__
        EM_ASM({ if (window._videoStop) window._videoStop(); });
#else
        std::cout << "[Video] StopVideo\n";
#endif
    });

    lua.set_function("IsVideoPlaying", []() -> bool {
#ifdef __EMSCRIPTEN__
        return EM_ASM_INT({
            return (window._videoIsPlaying && window._videoIsPlaying()) ? 1 : 0;
        }) != 0;
#else
        return false;
#endif
    });

    lua.set_function("SetVideoLoop", [](bool loop) {
#ifdef __EMSCRIPTEN__
        EM_ASM({ if (window._videoSetLoop) window._videoSetLoop($0); }, loop ? 1 : 0);
#else
        (void)loop;
#endif
    });

#ifdef __EMSCRIPTEN__
    // --- Network table (web / multiplayer via socket.io + WebRTC) ------------
    // JS side (multiplayer.js) exposes:
    //   window._netSend(key, val)
    //   window._netDequeue()        -> JSON string or ""
    //   window._netConnected()      -> 0 or 1
    //   window._netPlayerCount()    -> int
    sol::table net = lua.create_named_table("Network");

    net.set_function("Send", [](const std::string& key, const std::string& val) {
        EM_ASM({
            if (window._netSend)
                window._netSend(UTF8ToString($0), UTF8ToString($1));
        }, key.c_str(), val.c_str());
    });

    net.set_function("IsConnected", []() -> bool {
        return EM_ASM_INT({
            return (window._netConnected && window._netConnected()) ? 1 : 0;
        }) != 0;
    });

    net.set_function("GetPlayerCount", []() -> int {
        return EM_ASM_INT({
            return window._netPlayerCount ? window._netPlayerCount() : 1;
        });
    });

    net.set_function("Receive", [](sol::this_state s) -> sol::object {
        char buf[2048] = {};
        int got = EM_ASM_INT({
            if (!window._netDequeue) { return 0; }
            var msg = window._netDequeue();
            if (!msg || msg.length === 0) { return 0; }
            var enc = new TextEncoder();
            var bytes = enc.encode(msg);
            var len = Math.min(bytes.length, 2047);
            HEAPU8.set(bytes.subarray(0, len), $0);
            HEAPU8[$0 + len] = 0;
            return 1;
        }, buf);
        if (!got) return sol::nil;

        // buf contains JSON: {"from":"x","key":"y","value":"z"}
        // Simple extraction (no json dep needed here)
        sol::state_view lv(s);
        sol::table t = lv.create_table();
        std::string j(buf);
        auto extract = [&](const std::string& field) -> std::string {
            std::string search = "\"" + field + "\":\"";
            auto pos = j.find(search);
            if (pos == std::string::npos) return "";
            pos += search.size();
            auto end = j.find('"', pos);
            return j.substr(pos, end - pos);
        };
        t["from"]  = extract("from");
        t["key"]   = extract("key");
        t["value"] = extract("value");
        return t;
    });
#endif
}

// -----------------------------------------------------------------------
// Helper: call a named void function in a Lua state (no args besides dt)
// -----------------------------------------------------------------------
static void CallLuaFunction(sol::state& lua, const std::string& name,
                            const std::string& scriptPath, float dt = -1.0f)
{
    sol::protected_function fn = lua[name];
    if (!fn.valid())
        return;

    sol::protected_function_result res;
    if (dt >= 0.0f)
        res = fn(dt);
    else
        res = fn();

    if (!res.valid()) {
        sol::error err = res;
        std::cerr << "[LuaScriptEngine] " << name << " error in '"
                  << scriptPath << "': " << err.what() << "\n";
    }
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void LuaScriptEngine::Init()
{
    std::cout << "[LuaScriptEngine] Initialized\n";
}

void LuaScriptEngine::Shutdown()
{
    s_States.clear();
    std::cout << "[LuaScriptEngine] Shutdown\n";
}

void LuaScriptEngine::LoadScript(Scene* scene, Entity entity, const std::string& scriptPath)
{
    if (scriptPath.empty()) {
        std::cerr << "[LuaScriptEngine] Empty script path on entity " << entity.GetID() << "\n";
        return;
    }

    uint32_t id = entity.GetID();
    s_States.erase(id);

    auto lua = std::make_unique<sol::state>();
    SetupLuaState(*lua, scene, entity);

    // Load the file (compile only)
    auto loadResult = lua->load_file(scriptPath);
    if (!loadResult.valid()) {
        sol::error err = loadResult;
        std::cerr << "[LuaScriptEngine] Compile error in '" << scriptPath
                  << "': " << err.what() << "\n";
        return;
    }

    // Execute to define OnStart / OnUpdate
    sol::protected_function_result execResult = loadResult();
    if (!execResult.valid()) {
        sol::error err = execResult;
        std::cerr << "[LuaScriptEngine] Runtime error in '" << scriptPath
                  << "': " << err.what() << "\n";
        return;
    }

    std::cout << "[LuaScriptEngine] Loaded: " << scriptPath << "\n";
    s_States[id] = std::move(lua);
}

void LuaScriptEngine::ReloadScript(Scene* scene, Entity entity)
{
    if (!entity.HasComponent<ScriptComponent>())
        return;
    auto& sc = entity.GetComponent<ScriptComponent>();
    LoadScript(scene, entity, sc.ScriptPath);
}

void LuaScriptEngine::UnloadScript(Entity entity)
{
    s_States.erase(entity.GetID());
}

void LuaScriptEngine::OnSceneStart(Scene* scene)
{
    auto view = scene->GetRegistry().view<ScriptComponent>();
    for (auto handle : view) {
        Entity entity(handle, scene);
        auto& sc = entity.GetComponent<ScriptComponent>();

        if (!sc.Enabled || sc.ScriptPath.empty())
            continue;

        LoadScript(scene, entity, sc.ScriptPath);

        auto it = s_States.find(entity.GetID());
        if (it != s_States.end())
            CallLuaFunction(*it->second, "OnStart", sc.ScriptPath);
    }
}

void LuaScriptEngine::OnSceneStop(Scene* scene)
{
    auto view = scene->GetRegistry().view<ScriptComponent>();
    for (auto handle : view)
        s_States.erase(Entity(handle, scene).GetID());
}

void LuaScriptEngine::OnUpdate(Scene* scene, float deltaTime)
{
    auto view = scene->GetRegistry().view<ScriptComponent>();
    for (auto handle : view) {
        Entity entity(handle, scene);
        auto& sc = entity.GetComponent<ScriptComponent>();

        if (!sc.Enabled)
            continue;

        auto it = s_States.find(entity.GetID());
        if (it == s_States.end())
            continue;

        // Call OnUpdate(dt); disable script on error to prevent console spam
        sol::protected_function fn = (*it->second)["OnUpdate"];
        if (!fn.valid())
            continue;

        sol::protected_function_result res = fn(deltaTime);
        if (!res.valid()) {
            sol::error err = res;
            std::cerr << "[LuaScriptEngine] OnUpdate error in '"
                      << sc.ScriptPath << "': " << err.what()
                      << "\n  (script disabled to suppress further errors)\n";
            sc.Enabled = false;
        }
    }
}

void LuaScriptEngine::FireCollision(Scene* /*scene*/, uint32_t entityID,
                                    const std::string& otherName)
{
    auto it = s_States.find(entityID);
    if (it == s_States.end()) return;

    sol::protected_function fn = (*it->second)["OnCollision"];
    if (!fn.valid()) return;

    sol::protected_function_result res = fn(otherName);
    if (!res.valid()) {
        sol::error err = res;
        std::cerr << "[LuaScriptEngine] OnCollision error: " << err.what() << "\n";
    }
}

void LuaScriptEngine::FireEvent(Scene* /*scene*/, Entity entity, const std::string& fnName)
{
    auto it = s_States.find(entity.GetID());
    if (it == s_States.end()) return;

    sol::protected_function fn = (*it->second)[fnName];
    if (!fn.valid()) return;

    sol::protected_function_result res = fn();
    if (!res.valid()) {
        sol::error err = res;
        std::cerr << "[LuaScriptEngine] " << fnName << " error: " << err.what() << "\n";
    }
}

GameState LuaScriptEngine::GetGameState()  { return s_GameState; }
void      LuaScriptEngine::SetGameState(GameState s) { s_GameState = s; }
void      LuaScriptEngine::ResetGameState() { s_GameState = GameState::Playing; }

bool LuaScriptEngine::HasPendingLevel() { return s_PendingLevelIndex >= 0; }
int  LuaScriptEngine::GetAndClearPendingLevel()
{
    int idx = s_PendingLevelIndex;
    s_PendingLevelIndex = -1;
    return idx;
}

} // namespace Engine
