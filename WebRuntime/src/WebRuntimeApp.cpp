/**
 * WebRuntimeApp — Kirkiana Games universal WebAssembly player
 *
 * A single pre-compiled WASM binary that can play any Kirkiana game scene.
 * The game to load is determined at startup from the URL query parameter:
 *
 *   play.html?game=<gameId>
 *
 * The scene file is fetched from /api/games/<gameId>/scene at runtime.
 * Game assets (sprites, etc.) are fetched on-demand using the same base URL.
 *
 * Build with Emscripten — see WebRuntime/CMakeLists.txt for instructions.
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

#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <cstdio>

using namespace Engine;

// ---------------------------------------------------------------------------
// Helpers — read URL query parameters from the browser
// ---------------------------------------------------------------------------
static std::string GetURLParam(const char* key)
{
    char buf[512] = {};
    EM_ASM({
        var p   = new URLSearchParams(window.location.search);
        var val = p.get(UTF8ToString($0)); val = val ? val : "";
        stringToUTF8(val, $1, 512);
    }, key, buf);
    return buf;
}

// ---------------------------------------------------------------------------
// Scene-fetch state (populated by emscripten_fetch callbacks)
// ---------------------------------------------------------------------------
static bool        g_SceneReady    = false;
static bool        g_SceneFetched  = false;  // true even on error (proceed with empty)
static const char* SCENE_VPATH     = "/tmp/game.scene";

static void OnSceneFetchSuccess(emscripten_fetch_t* fetch)
{
    FILE* f = fopen(SCENE_VPATH, "wb");
    if (f)
    {
        fwrite(fetch->data, 1, static_cast<size_t>(fetch->numBytes), f);
        fclose(f);
        g_SceneFetched = true;
        g_SceneReady   = true;
        std::cout << "[WebRuntime] Scene fetched (" << fetch->numBytes << " bytes)\n";
    }
    else
    {
        std::cerr << "[WebRuntime] Could not write scene to virtual FS\n";
        g_SceneReady = true;
    }
    emscripten_fetch_close(fetch);
}

static void OnSceneFetchFailed(emscripten_fetch_t* fetch)
{
    std::cerr << "[WebRuntime] Failed to fetch scene from: " << fetch->url << "\n";
    g_SceneReady = true;  // proceed with empty scene
    emscripten_fetch_close(fetch);
}

static void FetchScene(const std::string& url)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess  = OnSceneFetchSuccess;
    attr.onerror    = OnSceneFetchFailed;
    emscripten_fetch(&attr, url.c_str());
}

// ---------------------------------------------------------------------------
// Level-fetch state — used when Game.LoadLevel(n) is called from Lua
// ---------------------------------------------------------------------------
static bool        g_LevelReady    = false;
static bool        g_LevelFetched  = false;
static const char* LEVEL_VPATH     = "/tmp/level.scene";

static void OnLevelFetchSuccess(emscripten_fetch_t* fetch)
{
    FILE* f = fopen(LEVEL_VPATH, "wb");
    if (f)
    {
        fwrite(fetch->data, 1, static_cast<size_t>(fetch->numBytes), f);
        fclose(f);
        g_LevelFetched = true;
        std::cout << "[WebRuntime] Level fetched (" << fetch->numBytes << " bytes)\n";
    }
    else
        std::cerr << "[WebRuntime] Could not write level to virtual FS\n";
    g_LevelReady = true;
    emscripten_fetch_close(fetch);
}

static void OnLevelFetchFailed(emscripten_fetch_t* fetch)
{
    std::cerr << "[WebRuntime] Failed to fetch level from: " << fetch->url << "\n";
    g_LevelReady = true;
    emscripten_fetch_close(fetch);
}

static void FetchLevel(const std::string& url)
{
    g_LevelReady  = false;
    g_LevelFetched = false;
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess  = OnLevelFetchSuccess;
    attr.onerror    = OnLevelFetchFailed;
    emscripten_fetch(&attr, url.c_str());
}

// ---------------------------------------------------------------------------
// Dialogue auto-advance (same helper as RuntimeApp)
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
                    dlg.Active      = false;
                    dlg.CurrentLine = 0;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Splash scene builder (shown while scene is being fetched)
// ---------------------------------------------------------------------------
static constexpr float SPLASH_DURATION = 2.0f;

static std::unique_ptr<Scene> BuildSplashScene()
{
    auto scene = std::make_unique<Scene>();

    auto cam  = scene->CreateEntity("SplashCam");
    auto& cc  = cam.AddComponent<CameraComponent>();
    cc.Primary         = true;
    cc.OrthographicSize = 10.0f;

    auto bg  = scene->CreateEntity("BG");
    auto& bt = bg.GetComponent<TransformComponent>();
    bt.Scale = glm::vec3(40.0f, 25.0f, 1.0f);
    auto& bs = bg.AddComponent<SpriteRendererComponent>();
    bs.Color = glm::vec4(0.08f, 0.06f, 0.14f, 1.0f);

    auto title  = scene->CreateEntity("Title");
    auto& tt    = title.GetComponent<TransformComponent>();
    tt.Position = glm::vec3(-4.2f, 1.2f, 0.1f);
    auto& tc    = title.AddComponent<TextComponent>();
    tc.Text     = "KIRKIANA";
    tc.Color    = glm::vec4(0.95f, 0.75f, 0.25f, 1.0f);
    tc.FontSize = 1.1f;

    auto sub  = scene->CreateEntity("Sub");
    auto& st  = sub.GetComponent<TransformComponent>();
    st.Position = glm::vec3(-1.8f, 0.0f, 0.1f);
    auto& sc  = sub.AddComponent<TextComponent>();
    sc.Text     = "GAMES";
    sc.Color    = glm::vec4(0.85f, 0.85f, 0.95f, 1.0f);
    sc.FontSize = 0.55f;

    auto tag  = scene->CreateEntity("Tag");
    auto& tat = tag.GetComponent<TransformComponent>();
    tat.Position = glm::vec3(-3.5f, -1.5f, 0.1f);
    auto& tagc = tag.AddComponent<TextComponent>();
    tagc.Text     = "Loading game...";
    tagc.Color    = glm::vec4(0.55f, 0.55f, 0.7f, 1.0f);
    tagc.FontSize = 0.28f;

    return scene;
}

// ---------------------------------------------------------------------------
// WebRuntimeApp
// ---------------------------------------------------------------------------
class WebRuntimeApp : public Application
{
public:
    WebRuntimeApp(const std::string& gameId, uint32_t w, uint32_t h)
        : Application("Kirkiana Games", w, h)
        , m_GameId(gameId)
        , m_WindowWidth(w)
        , m_WindowHeight(h)
    {}

    void OnInit() override
    {
        m_Renderer     = std::make_unique<SceneRenderer>();
        m_SplashScene  = BuildSplashScene();
        m_SplashTimer  = SPLASH_DURATION;
        m_GameLoaded   = false;

        m_Scene        = std::make_unique<Scene>();
        m_PhysicsWorld = std::make_unique<PhysicsWorld>(glm::vec2(0.0f, -9.8f));
        m_OverlayScene = BuildOverlayScene();

        AudioEngine::Init();
        LuaScriptEngine::Init();

        // Start fetching the scene file asynchronously
        if (!m_GameId.empty())
        {
            std::string url = "/api/games/" + m_GameId + "/scene";
            std::cout << "[WebRuntime] Fetching scene: " << url << "\n";
            FetchScene(url);
        }
        else
        {
            // No game ID — skip fetch, show splash then empty scene
            g_SceneReady = true;
        }
    }

    void StartGame()
    {
        m_SplashScene.reset();
        m_GameLoaded = true;

        if (g_SceneFetched)
        {
            if (!SceneSerializer::Deserialize(m_Scene.get(), SCENE_VPATH))
                std::cerr << "[WebRuntime] Failed to deserialize scene\n";
        }

        LuaScriptEngine::ResetGameState();
        SetupNetworkLua();
        SetupVideoLua();
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        AnimationSystem::OnSceneStart(m_Scene.get());
        AudioSystem::OnSceneStart(m_Scene.get());
        LuaScriptEngine::OnSceneStart(m_Scene.get());
        AutoPlayVideos();
        std::cout << "[WebRuntime] Game started.\n";
    }

    // Hot-swap the running scene with a freshly-loaded level
    void ApplyLevelScene()
    {
        LuaScriptEngine::OnSceneStop(m_Scene.get());
        AnimationSystem::OnSceneStop(m_Scene.get());
        AudioSystem::OnSceneStop(m_Scene.get());
        ParticleSystem::OnSceneStop(m_Scene.get());
        PhysicsSystem::OnSceneStop(m_Scene.get());

        m_Scene = std::make_unique<Scene>();
        if (g_LevelFetched)
        {
            if (!SceneSerializer::Deserialize(m_Scene.get(), LEVEL_VPATH))
                std::cerr << "[WebRuntime] Failed to deserialize level scene\n";
        }

        LuaScriptEngine::ResetGameState();
        SetupNetworkLua();
        SetupVideoLua();
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        AnimationSystem::OnSceneStart(m_Scene.get());
        AudioSystem::OnSceneStart(m_Scene.get());
        LuaScriptEngine::OnSceneStart(m_Scene.get());
        AutoPlayVideos();
        std::cout << "[WebRuntime] Level loaded.\n";
    }

    // Inject Network.* and Game.SubmitScore into every Lua state after scene start
    // (called after LuaScriptEngine::OnSceneStart sets up the states)
    void SetupNetworkLua()
    {
        // We expose JS-bridge functions globally so Lua scripts can call:
        //   Network.Send(key, value)
        //   Network.Receive()  -> {from, key, value} or nil
        //   Network.IsConnected() -> bool
        //   Network.GetPlayerCount() -> int
        //   Game.SubmitScore(score)
        //
        // The JS side (multiplayer.js) exposes:
        //   window._netSend(key, val)
        //   window._netDequeue() -> JSON string or ""
        //   window._netConnected() -> bool (0/1)
        //   window._netPlayerCount() -> int
        //   window._submitScore(gameId, score)
        //
        // We inject these via a Lua chunk executed in every entity state.
        // LuaScriptEngine exposes no iteration, so we piggyback on a
        // post-start hook by calling the EM_ASM setup once; the JS side
        // then intercepts Lua calls through the EM_ASM bridge below.
        //
        // Simpler approach: add a global script entity that calls the bridge.
        // Actual bridge is in multiplayer.js (window._net*).

        std::string gameId = m_GameId;

        // EM_ASM block: define JS-side helpers that Lua will call via ccall
        EM_ASM({
            // Ensure queue exists (multiplayer.js may not be loaded)
            if (!window._netQueue)   window._netQueue   = [];
            if (!window._netSend)    window._netSend    = function() {};
            if (!window._netDequeue) window._netDequeue = function() { return ""; };
            if (!window._netConnected)    window._netConnected    = function() { return 0; };
            if (!window._netPlayerCount)  window._netPlayerCount  = function() { return 1; };
            if (!window._submitScore)     window._submitScore     = function() {};
        });
    }

    void SetupVideoLua()
    {
        // Define JS-side video bridge stubs so Lua calls don't fail if
        // multiplayer.js / game-specific video JS is not yet loaded.
        EM_ASM({
            if (!window._videoPlay)      window._videoPlay      = function(url) {
                var v = document.getElementById('kirkiana-video');
                if (!v) {
                    v = document.createElement('video');
                    v.id = 'kirkiana-video';
                    v.style.cssText = 'position:absolute;top:0;left:0;width:100%;height:100%;z-index:10;background:#000';
                    document.body.appendChild(v);
                }
                v.src = url; v.style.display = 'block'; v.play();
            };
            if (!window._videoPause)     window._videoPause     = function() {
                var v = document.getElementById('kirkiana-video'); if (v) v.pause();
            };
            if (!window._videoStop)      window._videoStop      = function() {
                var v = document.getElementById('kirkiana-video');
                if (v) { v.pause(); v.src = ''; v.style.display = 'none'; }
            };
            if (!window._videoIsPlaying) window._videoIsPlaying = function() {
                var v = document.getElementById('kirkiana-video');
                return v && !v.paused && !v.ended ? 1 : 0;
            };
            if (!window._videoSetLoop)   window._videoSetLoop   = function(loop) {
                var v = document.getElementById('kirkiana-video'); if (v) v.loop = !!loop;
            };
        });
    }

    // Auto-play VideoComponents that have AutoPlay = true
    void AutoPlayVideos()
    {
        auto view = m_Scene->GetRegistry().view<VideoComponent>();
        for (auto h : view)
        {
            auto& vc = view.get<VideoComponent>(h);
            if (vc.AutoPlay && !vc.VideoUrl.empty())
            {
                std::string url  = vc.VideoUrl;
                int         loop = vc.Loop ? 1 : 0;
                EM_ASM({
                    if (window._videoSetLoop) window._videoSetLoop($1);
                    if (window._videoPlay)    window._videoPlay(UTF8ToString($0));
                }, url.c_str(), loop);
            }
        }
    }

    void OnUpdate(Timestep ts) override
    {
        float dt = ts.GetSeconds();

        if (!m_GameLoaded)
        {
            // Wait until both the splash timer expires AND the scene has been fetched
            m_SplashTimer -= dt;
            if (m_SplashTimer <= 0.0f && g_SceneReady)
                StartGame();
            return;
        }

        // Level transition: wait for async fetch, then swap scene
        if (m_LevelLoading && g_LevelReady)
        {
            m_LevelLoading = false;
            ApplyLevelScene();
        }

        // Queue a level fetch when Lua calls Game.LoadLevel(n)
        if (!m_LevelLoading && LuaScriptEngine::HasPendingLevel())
        {
            int levelIdx = LuaScriptEngine::GetAndClearPendingLevel();
            std::string url = "/api/games/" + m_GameId + "/scenes/" + std::to_string(levelIdx);
            std::cout << "[WebRuntime] Fetching level " << levelIdx << ": " << url << "\n";
            m_LevelLoading = true;
            FetchLevel(url);
        }

        auto gs = LuaScriptEngine::GetGameState();

        // P key: toggle pause
        if (Input::IsKeyPressed(KeyCode::P))
        {
            if (gs == GameState::Playing)
                LuaScriptEngine::SetGameState(GameState::Paused);
            else if (gs == GameState::Paused)
                LuaScriptEngine::SetGameState(GameState::Playing);
        }

        // Only update systems when playing (not paused/won/lost)
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
        float ar = (float)m_WindowWidth / (float)m_WindowHeight;
        float s  = 10.0f;
        return glm::ortho(-s * ar * 0.5f, s * ar * 0.5f, -s * 0.5f, s * 0.5f, -1.f, 1.f);
    }

    void OnRender() override
    {
        glViewport(0, 0, (int)m_WindowWidth, (int)m_WindowHeight);

        if (!m_GameLoaded && m_SplashScene)
        {
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

        // Game state overlay
        auto gs = LuaScriptEngine::GetGameState();
        if (gs == GameState::Paused || gs == GameState::Won || gs == GameState::Lost)
        {
            float ar  = (float)m_WindowWidth / (float)m_WindowHeight;
            float s   = 10.0f;
            glm::mat4 ovp = glm::ortho(-s * ar * 0.5f, s * ar * 0.5f,
                                       -s * 0.5f, s * 0.5f, -1.f, 1.f);
            Renderer2D::BeginScene(ovp);
            Renderer2D::DrawQuad(glm::vec3(0, 0, 0.5f), glm::vec2(s * ar, s),
                                 glm::vec4(0, 0, 0, 0.55f));
            Renderer2D::EndScene();

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
        if (m_GameLoaded)
        {
            LuaScriptEngine::OnSceneStop(m_Scene.get());
            AnimationSystem::OnSceneStop(m_Scene.get());
            AudioSystem::OnSceneStop(m_Scene.get());
            ParticleSystem::OnSceneStop(m_Scene.get());
            PhysicsSystem::OnSceneStop(m_Scene.get());
        }
        LuaScriptEngine::Shutdown();
        AudioEngine::Shutdown();
    }

private:
    std::string                    m_GameId;
    uint32_t                       m_WindowWidth;
    uint32_t                       m_WindowHeight;

    std::unique_ptr<Scene>         m_Scene;
    std::unique_ptr<SceneRenderer> m_Renderer;
    std::unique_ptr<PhysicsWorld>  m_PhysicsWorld;

    std::unique_ptr<Scene>         m_SplashScene;
    float                          m_SplashTimer = 0.0f;
    bool                           m_GameLoaded  = false;

    std::unique_ptr<Scene>         m_OverlayScene;

    bool                           m_LevelLoading = false; // true while async level fetch is in flight

    static std::unique_ptr<Scene> BuildOverlayScene()
    {
        auto scene = std::make_unique<Scene>();
        auto addText = [&](const char* tag, const char* text,
                           float x, float y, float size, glm::vec4 color)
        {
            auto e = scene->CreateEntity(tag);
            e.GetComponent<TransformComponent>().Position = glm::vec3(x, y, 0.9f);
            auto& tc = e.AddComponent<TextComponent>();
            tc.Text = text; tc.Color = color; tc.FontSize = size;
        };
        addText("PAUSED",      "PAUSED",                  -2.0f,  0.5f, 1.0f, glm::vec4(1, 1, 0, 1));
        addText("PAUSED_hint", "Press P to resume",        -2.5f, -0.5f, 0.3f, glm::vec4(0.8f, 0.8f, 0.8f, 1));
        addText("WIN",         "YOU WIN!",                 -2.5f,  0.5f, 1.0f, glm::vec4(0.3f, 1, 0.3f, 1));
        addText("WIN_hint",    "Thanks for playing!",      -2.8f, -0.5f, 0.3f, glm::vec4(0.8f, 0.8f, 0.8f, 1));
        addText("LOSE",        "GAME OVER",                -3.0f,  0.5f, 1.0f, glm::vec4(1, 0.3f, 0.3f, 1));
        addText("LOSE_hint",   "Better luck next time!",   -3.0f, -0.5f, 0.3f, glm::vec4(0.8f, 0.8f, 0.8f, 1));
        return scene;
    }
};

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main()
{
    // Read game ID from URL: play.html?game=123
    std::string gameId = GetURLParam("game");
    std::cout << "[WebRuntime] Game ID: " << (gameId.empty() ? "(none)" : gameId) << "\n";

    // Default canvas size — JS can resize after load
    constexpr uint32_t W = 1280;
    constexpr uint32_t H = 720;

    WebRuntimeApp app(gameId, W, H);
    app.Run();
    return 0;
}
