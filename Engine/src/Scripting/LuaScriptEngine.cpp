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

#include <unordered_map>
#include <iostream>

namespace Engine {

// -----------------------------------------------------------------------
// Internal state: one sol::state per entity (keyed by entity uint32_t ID)
// -----------------------------------------------------------------------
static std::unordered_map<uint32_t, std::unique_ptr<sol::state>> s_States;

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

} // namespace Engine
