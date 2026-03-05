/**
 * RuntimeApp — Standalone game player
 *
 * Loads a scene file and runs it without any editor UI.
 * Suitable for distributing finished games.
 *
 * Usage:
 *   ./Runtime                          # loads "game.scene"
 *   ./Runtime path/to/my.scene         # explicit scene file
 *   ./Runtime path/to/my.scene 1280 720 # custom resolution
 *
 * Controls:
 *   ESC  — quit
 *   F11  — toggle fullscreen (future)
 */

#include <Engine/Core/Application.h>
#include <Engine/Core/Timestep.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneRenderer.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Physics/PhysicsWorld.h>
#include <Engine/Physics/PhysicsSystem.h>
#include <Engine/ECS/Entity.h>
#include <Engine/ECS/Components.h>
#include <Engine/Input/Input.h>
#include <Engine/Scripting/LuaScriptEngine.h>
#include <Engine/Audio/AudioEngine.h>
#include <Engine/Audio/AudioSystem.h>
#include <Engine/Animation/AnimationSystem.h>
#include <Engine/Particles/ParticleSystem.h>
#include <Engine/Game/GameTimerSystem.h>
#include <Engine/Rendering/Renderer2D.h>
#include <Engine/Rendering/Framebuffer.h>

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>

using namespace Engine;

// ---------------------------------------------------------------------------
// DialogueAdvance helper — drives DialogueComponent advancement in-engine
// ---------------------------------------------------------------------------
static void UpdateDialogue(Scene* scene, float dt)
{
    auto view = scene->GetRegistry().view<DialogueComponent>();
    for (auto handle : view)
    {
        auto& dlg = view.get<DialogueComponent>(handle);
        if (!dlg.Active || dlg.Lines.empty())
            continue;

        if (dlg.AutoAdvance)
        {
            dlg.AutoAdvanceTimer += dt;
            if (dlg.AutoAdvanceTimer >= dlg.AutoAdvanceTime)
            {
                dlg.AutoAdvanceTimer = 0.0f;
                dlg.CurrentLine++;
                if (dlg.CurrentLine >= (int)dlg.Lines.size())
                {
                    dlg.Active = false;
                    dlg.CurrentLine = 0;
                }
            }
        }
        // Manual advance handled in Lua scripts via engine API
    }
}

// Duration of the Kirkiana splash screen in seconds
static constexpr float SPLASH_DURATION = 2.5f;

// Build a minimal splash scene shown before the game starts
static std::unique_ptr<Scene> BuildSplashScene()
{
    auto scene = std::make_unique<Scene>();

    // Camera
    auto cam = scene->CreateEntity("SplashCam");
    auto& cc = cam.AddComponent<CameraComponent>();
    cc.Primary = true;
    cc.OrthographicSize = 10.0f;

    // Dark background quad
    auto bg = scene->CreateEntity("BG");
    auto& bgt = bg.GetComponent<TransformComponent>();
    bgt.Scale = glm::vec3(40.0f, 25.0f, 1.0f);
    auto& bgs = bg.AddComponent<SpriteRendererComponent>();
    bgs.Color = glm::vec4(0.08f, 0.06f, 0.14f, 1.0f);

    // "KIRKIANA" title
    auto title = scene->CreateEntity("Title");
    auto& tt = title.GetComponent<TransformComponent>();
    tt.Position = glm::vec3(-4.2f, 1.2f, 0.1f);
    auto& tc = title.AddComponent<TextComponent>();
    tc.Text = "KIRKIANA";
    tc.Color = glm::vec4(0.95f, 0.75f, 0.25f, 1.0f);
    tc.FontSize = 1.1f;

    // "GAMES" label
    auto sub = scene->CreateEntity("Sub");
    auto& st = sub.GetComponent<TransformComponent>();
    st.Position = glm::vec3(-1.8f, 0.0f, 0.1f);
    auto& sc2 = sub.AddComponent<TextComponent>();
    sc2.Text = "GAMES";
    sc2.Color = glm::vec4(0.85f, 0.85f, 0.95f, 1.0f);
    sc2.FontSize = 0.55f;

    // Tagline
    auto tag = scene->CreateEntity("Tag");
    auto& tagt = tag.GetComponent<TransformComponent>();
    tagt.Position = glm::vec3(-3.5f, -1.5f, 0.1f);
    auto& tagc = tag.AddComponent<TextComponent>();
    tagc.Text = "Indie games, made by creators like you";
    tagc.Color = glm::vec4(0.55f, 0.55f, 0.7f, 1.0f);
    tagc.FontSize = 0.28f;

    return scene;
}

// Build the overlay scene used for pause/win/lose screens
static std::unique_ptr<Scene> BuildOverlayScene()
{
    auto scene = std::make_unique<Scene>();

    auto addText = [&](const char* tag, const char* text,
                       float x, float y, float size, glm::vec4 color)
    {
        auto e = scene->CreateEntity(tag);
        e.GetComponent<TransformComponent>().Position = glm::vec3(x, y, 0.9f);
        auto& tc = e.AddComponent<TextComponent>();
        tc.Text     = text;
        tc.Color    = color;
        tc.FontSize = size;
    };

    // PAUSED state
    addText("PAUSED",      "PAUSED",                -2.0f,  0.5f, 1.0f, glm::vec4(1, 1, 0, 1));
    addText("PAUSED_hint", "Press P or ESC to resume", -3.2f, -0.5f, 0.3f, glm::vec4(0.8f, 0.8f, 0.8f, 1));

    // WON state
    addText("WIN",         "YOU WIN!",              -2.5f,  0.5f, 1.0f, glm::vec4(0.3f, 1, 0.3f, 1));
    addText("WIN_hint",    "Press ESC to quit",     -2.2f, -0.5f, 0.3f, glm::vec4(0.8f, 0.8f, 0.8f, 1));

    // LOST state
    addText("LOSE",        "GAME OVER",             -3.0f,  0.5f, 1.0f, glm::vec4(1, 0.3f, 0.3f, 1));
    addText("LOSE_hint",   "Press ESC to quit",     -2.2f, -0.5f, 0.3f, glm::vec4(0.8f, 0.8f, 0.8f, 1));

    return scene;
}

// ---------------------------------------------------------------------------
// RuntimeApp
// ---------------------------------------------------------------------------
class RuntimeApp : public Application
{
public:
    RuntimeApp(const std::string& scenePath, uint32_t w, uint32_t h)
        : Application("Kirkiana Games", w, h)
        , m_ScenePath(scenePath)
        , m_WindowWidth(w)
        , m_WindowHeight(h)
    {}

    void OnInit() override
    {
        m_Renderer = std::make_unique<SceneRenderer>();
        AudioEngine::Init();
        LuaScriptEngine::Init();

        // Show splash while game scene is pre-loaded
        m_SplashScene = BuildSplashScene();
        m_SplashTimer = SPLASH_DURATION;
        m_GameLoaded  = false;

        m_Scene        = std::make_unique<Scene>();
        m_PhysicsWorld = std::make_unique<PhysicsWorld>(glm::vec2(0.0f, -9.8f));

        // Build overlay scene (text entities shown when paused/won/lost)
        m_OverlayScene = BuildOverlayScene();

        std::cout << "[Runtime] Loading scene: " << m_ScenePath << "\n";
        if (!SceneSerializer::Deserialize(m_Scene.get(), m_ScenePath))
        {
            std::cerr << "[Runtime] Failed to load scene '" << m_ScenePath << "'.\n";
            std::cerr << "          Starting with empty scene.\n";
        }
    }

    void StartGame()
    {
        m_SplashScene.reset();
        m_GameLoaded = true;
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        AnimationSystem::OnSceneStart(m_Scene.get());
        AudioSystem::OnSceneStart(m_Scene.get());
        LuaScriptEngine::OnSceneStart(m_Scene.get());
        std::cout << "[Runtime] Scene started.\n";
    }

    void OnUpdate(Timestep ts) override
    {
        float dt = ts.GetSeconds();

        if (!m_GameLoaded)
        {
            m_SplashTimer -= dt;
            if (m_SplashTimer <= 0.0f)
                StartGame();
            return;
        }

        auto gs = LuaScriptEngine::GetGameState();

        // Escape: quit if playing; pause/resume toggle
        if (Input::IsKeyPressed(KeyCode::Escape))
        {
            if (gs == GameState::Paused)
                LuaScriptEngine::SetGameState(GameState::Playing);
            else
                Close();
        }

        // P key: toggle pause
        if (Input::IsKeyPressed(KeyCode::P))
        {
            if (gs == GameState::Playing)
                LuaScriptEngine::SetGameState(GameState::Paused);
            else if (gs == GameState::Paused)
                LuaScriptEngine::SetGameState(GameState::Playing);
        }

        // Only update game systems when actively playing
        if (gs == GameState::Playing)
        {
            PhysicsSystem::OnUpdate(m_Scene.get(), m_PhysicsWorld.get(), dt);
            LuaScriptEngine::OnUpdate(m_Scene.get(), dt);
            AudioSystem::OnUpdate(m_Scene.get());
            AnimationSystem::OnUpdate(m_Scene.get(), dt);
            ParticleSystem::OnUpdate(m_Scene.get(), dt);
            GameTimerSystem::OnUpdate(m_Scene.get(), dt);
            UpdateDialogue(m_Scene.get(), dt);
            m_Scene->OnUpdate(dt);
        }

        // Level transitions queued by Game.LoadLevel(n) in Lua
        if (LuaScriptEngine::HasPendingLevel())
            LoadLevel(LuaScriptEngine::GetAndClearPendingLevel());
    }

    void LoadLevel(int levelIndex)
    {
        std::string levelPath = "levels/level" + std::to_string(levelIndex) + ".scene";
        std::cout << "[Runtime] Loading level " << levelIndex << ": " << levelPath << "\n";

        LuaScriptEngine::OnSceneStop(m_Scene.get());
        AnimationSystem::OnSceneStop(m_Scene.get());
        AudioSystem::OnSceneStop(m_Scene.get());
        ParticleSystem::OnSceneStop(m_Scene.get());
        PhysicsSystem::OnSceneStop(m_Scene.get());

        m_Scene = std::make_unique<Scene>();
        if (!SceneSerializer::Deserialize(m_Scene.get(), levelPath))
            std::cerr << "[Runtime] Failed to load level from " << levelPath << "\n";

        LuaScriptEngine::ResetGameState();
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        AnimationSystem::OnSceneStart(m_Scene.get());
        AudioSystem::OnSceneStart(m_Scene.get());
        LuaScriptEngine::OnSceneStart(m_Scene.get());
    }

    glm::mat4 ComputeGameCameraVP() const
    {
        auto camView = m_Scene->GetRegistry().view<TransformComponent, CameraComponent>();
        for (auto e : camView)
        {
            auto [tf, cam] = camView.get<TransformComponent, CameraComponent>(e);
            if (cam.Primary)
            {
                float ar = (float)m_WindowWidth / (float)m_WindowHeight;
                return cam.GetProjection(ar) * glm::inverse(tf.GetTransform());
            }
        }
        // Fallback orthographic
        float ar = (float)m_WindowWidth / (float)m_WindowHeight;
        float s  = 10.0f;
        return glm::ortho(-s * ar * 0.5f, s * ar * 0.5f, -s * 0.5f, s * 0.5f, -1.f, 1.f);
    }

    void OnRender() override
    {
        glViewport(0, 0, (int)m_WindowWidth, (int)m_WindowHeight);

        if (!m_GameLoaded && m_SplashScene)
        {
            // Render splash scene
            glClearColor(0.08f, 0.06f, 0.14f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            float ar  = (float)m_WindowWidth / (float)m_WindowHeight;
            float s   = 10.0f;
            glm::mat4 vp = glm::ortho(-s * ar * 0.5f, s * ar * 0.5f,
                                      -s * 0.5f, s * 0.5f, -1.f, 1.f);
            m_Renderer->RenderScene(m_SplashScene.get(), vp);
            return;
        }

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 vp = ComputeGameCameraVP();
        m_Renderer->RenderScene(m_Scene.get(), vp);
        ParticleSystem::OnRender(m_Scene.get(), vp);

        // Game state overlay — fixed screen-space ortho projection
        auto gs = LuaScriptEngine::GetGameState();
        if (gs == GameState::Paused || gs == GameState::Won || gs == GameState::Lost)
        {
            float ar  = (float)m_WindowWidth / (float)m_WindowHeight;
            float s   = 10.0f;
            glm::mat4 ovp = glm::ortho(-s * ar * 0.5f, s * ar * 0.5f,
                                       -s * 0.5f, s * 0.5f, -1.f, 1.f);

            // Semi-transparent dark overlay quad
            Renderer2D::BeginScene(ovp);
            Renderer2D::DrawQuad(glm::vec3(0, 0, 0.5f), glm::vec2(s * ar, s),
                                 glm::vec4(0, 0, 0, 0.55f));
            Renderer2D::EndScene();

            // Show the right text entities
            const char* prefix = (gs == GameState::Paused) ? "PAUSED"
                               : (gs == GameState::Won)    ? "WIN"
                               :                             "LOSE";
            m_OverlayScene->GetRegistry().view<TagComponent, TextComponent>().each(
                [&](auto, auto& tag, auto& tc) {
                    tc.Visible = (tag.Tag.rfind(prefix, 0) == 0);
                });

            m_Renderer->RenderScene(m_OverlayScene.get(), ovp);
        }
    }

    void OnShutdown() override
    {
        LuaScriptEngine::OnSceneStop(m_Scene.get());
        AnimationSystem::OnSceneStop(m_Scene.get());
        AudioSystem::OnSceneStop(m_Scene.get());
        ParticleSystem::OnSceneStop(m_Scene.get());
        PhysicsSystem::OnSceneStop(m_Scene.get());
        LuaScriptEngine::Shutdown();
        AudioEngine::Shutdown();

        std::cout << "[Runtime] Shutdown complete.\n";
    }

private:
    std::string                    m_ScenePath;
    uint32_t                       m_WindowWidth;
    uint32_t                       m_WindowHeight;

    std::unique_ptr<Scene>         m_Scene;
    std::unique_ptr<SceneRenderer> m_Renderer;
    std::unique_ptr<PhysicsWorld>  m_PhysicsWorld;

    // Splash screen
    std::unique_ptr<Scene>         m_SplashScene;
    float                          m_SplashTimer = 0.0f;
    bool                           m_GameLoaded  = false;

    // Game state overlay (pause/win/lose)
    std::unique_ptr<Scene>         m_OverlayScene;
};

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    std::string scenePath = "game.scene";
    uint32_t    width     = 1280;
    uint32_t    height    = 720;

    if (argc >= 2)
        scenePath = argv[1];
    if (argc >= 3)
        width = static_cast<uint32_t>(std::stoi(argv[2]));
    if (argc >= 4)
        height = static_cast<uint32_t>(std::stoi(argv[3]));

    RuntimeApp app(scenePath, width, height);
    app.Run();
    return 0;
}
