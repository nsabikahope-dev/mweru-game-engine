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
#include <Engine/ECS/Components.h>
#include <Engine/Input/Input.h>
#include <Engine/Scripting/LuaScriptEngine.h>
#include <Engine/Audio/AudioEngine.h>
#include <Engine/Audio/AudioSystem.h>
#include <Engine/Animation/AnimationSystem.h>
#include <Engine/Particles/ParticleSystem.h>
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

// ---------------------------------------------------------------------------
// RuntimeApp
// ---------------------------------------------------------------------------
class RuntimeApp : public Application
{
public:
    RuntimeApp(const std::string& scenePath, uint32_t w, uint32_t h)
        : Application("Game", w, h)
        , m_ScenePath(scenePath)
        , m_WindowWidth(w)
        , m_WindowHeight(h)
    {}

    void OnInit() override
    {
        std::cout << "[Runtime] Loading scene: " << m_ScenePath << "\n";

        m_Scene        = std::make_unique<Scene>();
        m_Renderer     = std::make_unique<SceneRenderer>();
        m_PhysicsWorld = std::make_unique<PhysicsWorld>(glm::vec2(0.0f, -9.8f));

        AudioEngine::Init();
        LuaScriptEngine::Init();

        if (!SceneSerializer::Deserialize(m_Scene.get(), m_ScenePath))
        {
            std::cerr << "[Runtime] Failed to load scene '" << m_ScenePath << "'.\n";
            std::cerr << "          Starting with empty scene.\n";
        }

        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        AnimationSystem::OnSceneStart(m_Scene.get());
        AudioSystem::OnSceneStart(m_Scene.get());
        LuaScriptEngine::OnSceneStart(m_Scene.get());

        std::cout << "[Runtime] Scene started.\n";
    }

    void OnUpdate(Timestep ts) override
    {
        float dt = ts.GetSeconds();

        if (Input::IsKeyPressed(KeyCode::Escape))
            Close();

        PhysicsSystem::OnUpdate(m_Scene.get(), m_PhysicsWorld.get(), dt);
        LuaScriptEngine::OnUpdate(m_Scene.get(), dt);
        AudioSystem::OnUpdate(m_Scene.get());
        AnimationSystem::OnUpdate(m_Scene.get(), dt);
        ParticleSystem::OnUpdate(m_Scene.get(), dt);
        UpdateDialogue(m_Scene.get(), dt);
        m_Scene->OnUpdate(dt);
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
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 vp = ComputeGameCameraVP();
        m_Renderer->RenderScene(m_Scene.get(), vp);
        ParticleSystem::OnRender(m_Scene.get(), vp);
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
