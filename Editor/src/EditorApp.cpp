#include <Engine/Core/Application.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneRenderer.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Physics/PhysicsWorld.h>
#include <Engine/Physics/PhysicsSystem.h>
#include <Engine/ECS/Entity.h>
#include <Engine/ECS/Components.h>
#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer2D.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Scripting/LuaScriptEngine.h>
#include <Engine/Audio/AudioEngine.h>
#include <Engine/Audio/AudioSystem.h>
#include <Engine/Animation/AnimationSystem.h>
#include <Engine/Particles/ParticleSystem.h>
#include <Engine/Game/GameTimerSystem.h>

#include "../include/EditorCamera.h"
#include "../include/GridRenderer.h"
#include "../include/AIHelper.h"
#include "../include/UpdateChecker.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <SDL2/SDL.h>
#include <glad/glad.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cctype>
#include <memory>

using namespace Engine;

class EditorApp : public Application
{
public:
    EditorApp()
        : Application("Kirkiana Games Studio", 1920, 1080)
    {
    }

    void OnInit() override
    {
        std::cout << "==========================================\n";
        std::cout << "  Game Engine Editor\n";
        std::cout << "==========================================\n\n";

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // Setup style
        ImGui::StyleColorsDark();

        // When viewports are enabled, tweak WindowRounding
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(GetWindow().GetNativeWindow(), GetWindow().GetNativeContext());
        ImGui_ImplOpenGL3_Init("#version 330");

        // Create scene
        m_Scene = std::make_unique<Scene>();
        m_Renderer = std::make_unique<SceneRenderer>();
        m_PhysicsWorld = std::make_unique<PhysicsWorld>(glm::vec2(0.0f, -9.8f));

        // Create framebuffer for viewport rendering
        m_Framebuffer = std::make_unique<Framebuffer>(1280, 720);
        m_ViewportSize = glm::vec2(1280, 720);

        // Create editor camera
        m_EditorCamera = std::make_unique<EditorCamera>(1280, 720);

        // Initialize subsystems
        AudioEngine::Init();
        LuaScriptEngine::Init();

        // Create default scene
        CreateDefaultScene();
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());

        std::cout << "Editor initialized successfully!\n\n";

        // Kick off background update check
        m_UpdateChecker.CheckAsync();
    }

    void CreateDefaultScene()
    {
        // Camera
        auto camera = m_Scene->CreateEntity("Main Camera");
        auto& cameraComp = camera.AddComponent<CameraComponent>();
        cameraComp.Primary = true;
        cameraComp.OrthographicSize = 10.0f;
        m_SelectedEntity = camera;

        // Example entity
        auto box = m_Scene->CreateEntity("Box");
        auto& transform = box.GetComponent<TransformComponent>();
        transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);

        auto& sprite = box.AddComponent<SpriteRendererComponent>();
        sprite.Color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);
    }

    void OnUpdate(Timestep ts) override
    {
        // Keyboard shortcuts
        bool ctrl = Input::IsKeyHeld(KeyCode::LeftControl) || Input::IsKeyHeld(KeyCode::RightControl);
        if (ctrl && Input::IsKeyPressed(KeyCode::S))
            SaveScene(m_CurrentScenePath);
        if (ctrl && Input::IsKeyPressed(KeyCode::O))
            LoadScene(m_CurrentScenePath);
        if (ctrl && Input::IsKeyPressed(KeyCode::N))
            NewScene();

        // Delete selected entity
        if (m_SelectedEntity && Input::IsKeyPressed(KeyCode::Delete))
        {
            m_Scene->DestroyEntity(m_SelectedEntity);
            m_SelectedEntity = {};
        }

        // Record frame time sample for profiler
        float dt = ts.GetSeconds();
        m_FrameTimes[m_FrameTimeIdx] = dt * 1000.0f;  // store as ms
        m_FrameTimeIdx = (m_FrameTimeIdx + 1) % k_ProfilerSamples;
        m_FrameTimeAccum += dt;
        m_FrameCount++;

        // Update editor camera
        m_EditorCamera->OnUpdate(ts.GetSeconds());

        // Update physics, scripts, audio, animation and particles if playing
        if (m_ScenePlaying)
        {
            // P key toggles pause
            if (Input::IsKeyPressed(KeyCode::P))
            {
                if (LuaScriptEngine::GetGameState() == GameState::Playing)
                    LuaScriptEngine::SetGameState(GameState::Paused);
                else if (LuaScriptEngine::GetGameState() == GameState::Paused)
                    LuaScriptEngine::SetGameState(GameState::Playing);
            }

            // Only run systems when the game is actually playing (not paused/won/lost)
            if (LuaScriptEngine::GetGameState() == GameState::Playing)
            {
                PhysicsSystem::OnUpdate(m_Scene.get(), m_PhysicsWorld.get(), ts.GetSeconds());
                LuaScriptEngine::OnUpdate(m_Scene.get(), ts.GetSeconds());
                AudioSystem::OnUpdate(m_Scene.get());
                AnimationSystem::OnUpdate(m_Scene.get(), ts.GetSeconds());
                ParticleSystem::OnUpdate(m_Scene.get(), ts.GetSeconds());
                GameTimerSystem::OnUpdate(m_Scene.get(), ts.GetSeconds());
            }

            // Level transitions queued by Game.LoadLevel(n) in Lua
            if (LuaScriptEngine::HasPendingLevel())
            {
                int levelIdx = LuaScriptEngine::GetAndClearPendingLevel();
                std::string levelPath = "levels/level" + std::to_string(levelIdx) + ".scene";
                std::cout << "[Editor] Level transition to: " << levelPath << "\n";

                LuaScriptEngine::OnSceneStop(m_Scene.get());
                AnimationSystem::OnSceneStop(m_Scene.get());
                AudioSystem::OnSceneStop(m_Scene.get());
                ParticleSystem::OnSceneStop(m_Scene.get());
                PhysicsSystem::OnSceneStop(m_Scene.get());

                m_Scene = std::make_unique<Scene>();
                if (!SceneSerializer::Deserialize(m_Scene.get(), levelPath))
                    std::cerr << "[Editor] Failed to load level " << levelPath << "\n";
                m_SelectedEntity = {};

                LuaScriptEngine::ResetGameState();
                PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
                AnimationSystem::OnSceneStart(m_Scene.get());
                AudioSystem::OnSceneStart(m_Scene.get());
                LuaScriptEngine::OnSceneStart(m_Scene.get());
            }
        }

        m_Scene->OnUpdate(ts.GetSeconds());
    }

    void OnRender() override
    {
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Dockspace
        ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        // Build default panel layout on first frame (overrides any saved imgui.ini)
        static bool s_LayoutReady = false;
        if (!s_LayoutReady)
        {
            s_LayoutReady = true;

            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

            // Split: left strip for Objects
            ImGuiID center = dockspace_id;
            ImGuiID left   = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.18f, nullptr, &center);

            // Split: right strip for Properties + Profiler
            ImGuiID right  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);

            // Split the right strip vertically: Properties on top, Profiler below
            ImGuiID right_bottom;
            ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.35f, &right_bottom, &right);

            ImGui::DockBuilderDockWindow("Objects",    left);
            ImGui::DockBuilderDockWindow("Viewport",   center);
            ImGui::DockBuilderDockWindow("Properties", right);
            ImGui::DockBuilderDockWindow("Profiler",   right_bottom);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        // Menu Bar
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene"))
                {
                    NewScene();
                }
                if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                {
                    LoadScene(m_CurrentScenePath);
                }
                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                {
                    SaveScene(m_CurrentScenePath);
                }
                if (ImGui::MenuItem("Save Scene As..."))
                {
                    SaveScene(m_CurrentScenePath);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    // Exit application
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Game"))
            {
                if (ImGui::MenuItem(m_ScenePlaying ? "Stop" : "Play", nullptr, m_ScenePlaying))
                {
                    m_ScenePlaying = !m_ScenePlaying;
                    if (m_ScenePlaying)
                    {
                        LuaScriptEngine::ResetGameState();
                        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
                        AudioSystem::OnSceneStart(m_Scene.get());
                        AnimationSystem::OnSceneStart(m_Scene.get());
                        LuaScriptEngine::OnSceneStart(m_Scene.get());
                    }
                    else
                    {
                        LuaScriptEngine::ResetGameState();
                        LuaScriptEngine::OnSceneStop(m_Scene.get());
                        AnimationSystem::OnSceneStop(m_Scene.get());
                        AudioSystem::OnSceneStop(m_Scene.get());
                        ParticleSystem::OnSceneStop(m_Scene.get());
                        PhysicsSystem::OnSceneStop(m_Scene.get());
                    }
                }
                // Show Pause option while playing
                if (m_ScenePlaying)
                {
                    auto gs = LuaScriptEngine::GetGameState();
                    const char* pauseLabel = (gs == GameState::Paused) ? "Resume" : "Pause";
                    if (ImGui::MenuItem(pauseLabel))
                    {
                        if (gs == GameState::Paused)
                            LuaScriptEngine::SetGameState(GameState::Playing);
                        else
                            LuaScriptEngine::SetGameState(GameState::Paused);
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Show Grid", nullptr, m_ShowGrid))
                {
                    m_ShowGrid = !m_ShowGrid;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Getting Started"))
                    m_ShowWelcome = true;
                ImGui::Separator();
                if (ImGui::MenuItem("Check for Updates"))
                {
                    m_UpdateToastDismissed = false;
                    m_ShowUpdateModal = true;
                    m_UpdateChecker.CheckAsync();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // Hierarchy Panel
        DrawHierarchyPanel();

        // Inspector Panel
        DrawInspectorPanel();

        // Viewport Panel
        DrawViewportPanel();

        // Game state overlay (shown in viewport when playing and paused/won/lost)
        if (m_ScenePlaying)
        {
            auto gs = LuaScriptEngine::GetGameState();
            if (gs == GameState::Paused || gs == GameState::Won || gs == GameState::Lost)
            {
                ImGuiIO& gio = ImGui::GetIO();
                ImGui::SetNextWindowPos(ImVec2(gio.DisplaySize.x * 0.5f, gio.DisplaySize.y * 0.5f),
                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowBgAlpha(0.88f);
                ImGui::SetNextWindowSize(ImVec2(320, 0));
                ImGui::Begin("##GameOverlay", nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_AlwaysAutoResize);

                if (gs == GameState::Paused)
                {
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("PAUSED").x) * 0.5f);
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "PAUSED");
                    ImGui::Separator();
                    ImGui::TextDisabled("Press P or Game > Resume to continue");
                }
                else if (gs == GameState::Won)
                {
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("YOU WIN!").x) * 0.5f);
                    ImGui::TextColored(ImVec4(0.3f, 1, 0.3f, 1), "YOU WIN!");
                    ImGui::Separator();
                    if (ImGui::Button("Play Again", ImVec2(-1, 0)))
                    {
                        LuaScriptEngine::ResetGameState();
                        LuaScriptEngine::OnSceneStop(m_Scene.get());
                        LuaScriptEngine::OnSceneStart(m_Scene.get());
                    }
                }
                else if (gs == GameState::Lost)
                {
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("GAME OVER").x) * 0.5f);
                    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "GAME OVER");
                    ImGui::Separator();
                    if (ImGui::Button("Try Again", ImVec2(-1, 0)))
                    {
                        LuaScriptEngine::ResetGameState();
                        LuaScriptEngine::OnSceneStop(m_Scene.get());
                        LuaScriptEngine::OnSceneStart(m_Scene.get());
                    }
                }
                ImGui::End();
            }
        }

        // Update available toast (top-right corner, non-blocking)
        if (m_UpdateChecker.IsUpdateAvailable() && !m_UpdateToastDismissed)
        {
            ImGuiIO& uio = ImGui::GetIO();
            ImGui::SetNextWindowPos(
                ImVec2(uio.DisplaySize.x - 10.0f, 30.0f),
                ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            ImGui::SetNextWindowBgAlpha(0.9f);
            ImGui::Begin("##UpdateToast", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoMove);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f),
                "Update available: v%s", m_UpdateChecker.GetLatestVersion().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Download"))
                m_UpdateChecker.OpenReleasePage();
            ImGui::SameLine();
            if (ImGui::SmallButton("X"))
                m_UpdateToastDismissed = true;
            ImGui::End();
        }

        // Update check result modal (triggered by Help -> Check for Updates)
        if (m_ShowUpdateModal)
        {
            ImGui::OpenPopup("Check for Updates");
            m_ShowUpdateModal = false;
        }
        if (ImGui::BeginPopupModal("Check for Updates", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            auto st = m_UpdateChecker.GetState();
            if (st == UpdateChecker::State::Checking)
            {
                ImGui::Text("Checking for updates...");
                // Keep modal open: re-queue OpenPopup next frame
                m_ShowUpdateModal = true;
            }
            else if (st == UpdateChecker::State::UpdateAvailable)
            {
                ImGui::Text("Update available: v%s",
                    m_UpdateChecker.GetLatestVersion().c_str());
                ImGui::Text("You have v" ENGINE_VERSION);
                if (ImGui::Button("Download"))
                {
                    m_UpdateChecker.OpenReleasePage();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Later"))
                    ImGui::CloseCurrentPopup();
            }
            else if (st == UpdateChecker::State::UpToDate)
            {
                ImGui::Text("You're up to date! (v" ENGINE_VERSION ")");
                if (ImGui::Button("OK"))
                    ImGui::CloseCurrentPopup();
            }
            else
            {
                ImGui::Text("Could not check for updates.");
                ImGui::Text("Check your internet connection.");
                if (ImGui::Button("OK"))
                    ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Welcome screen
        DrawWelcomeScreen();

        // Render ImGui
        ImGui::Render();

        // Render scene to framebuffer using editor camera
        m_Framebuffer->Bind();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer2D::ResetStats();

        // Render grid first
        Renderer2D::BeginScene(m_EditorCamera->GetViewProjectionMatrix());
        if (m_ShowGrid)
        {
            GridRenderer::DrawGrid(m_EditorCamera->GetViewProjectionMatrix(), m_GridSize, 40);
        }
        Renderer2D::EndScene();

        // Then render scene entities
        m_Renderer->RenderScene(m_Scene.get(), m_EditorCamera->GetViewProjectionMatrix());

        // Render particles on top (only during play)
        if (m_ScenePlaying)
            ParticleSystem::OnRender(m_Scene.get(), m_EditorCamera->GetViewProjectionMatrix());

        m_Framebuffer->Unbind();

        // Render ImGui to main window
        glViewport(0, 0, GetWindow().GetWidth(), GetWindow().GetHeight());
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }
    }

    void DrawWelcomeScreen()
    {
        if (!m_ShowWelcome) return;

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(580, 0), ImGuiCond_Always);
        ImGui::OpenPopup("##welcome");

        if (ImGui::BeginPopupModal("##welcome", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            // Title
            ImGui::Spacing();
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Kirkiana Games Studio").x) * 0.5f);
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Kirkiana Games Studio");
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() -
                ImGui::CalcTextSize("Create games, stories, and interactive experiences").x) * 0.5f);
            ImGui::TextDisabled("Create games, stories, and interactive experiences");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 3-step guide
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Getting started in 3 steps:");
            ImGui::Spacing();

            ImGui::BulletText("Step 1 — Create an object");
            ImGui::Indent();
            ImGui::TextDisabled("Click '+ New Object' in the Objects panel (left side)");
            ImGui::Unindent();
            ImGui::Spacing();

            ImGui::BulletText("Step 2 — Give it abilities");
            ImGui::Indent();
            ImGui::TextDisabled("Select it, then click 'Add Component' in the Properties panel (right side)");
            ImGui::TextDisabled("e.g. add Appearance to make it visible, or Sound to give it audio");
            ImGui::Unindent();
            ImGui::Spacing();

            ImGui::BulletText("Step 3 — Test your creation");
            ImGui::Indent();
            ImGui::TextDisabled("Click Game > Play to run your scene. Press it again to stop.");
            ImGui::Unindent();
            ImGui::Spacing();

            ImGui::Separator();
            ImGui::Spacing();

            // Tips
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Useful tips:");
            ImGui::BulletText("Right-click an object in the Objects panel to duplicate or delete it");
            ImGui::BulletText("Scroll to zoom, middle-click drag to pan in the Scene View");
            ImGui::BulletText("Use the AI Helper in a Behaviour (Script) component to write code for you");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Footer
            float btnWidth = 140.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btnWidth) * 0.5f);
            if (ImGui::Button("Get Started!", ImVec2(btnWidth, 32)))
            {
                m_ShowWelcome = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Spacing();
            bool showAgain = m_ShowWelcome;
            if (ImGui::Checkbox("Show this screen next time I open the editor", &showAgain))
                m_ShowWelcome = showAgain;
            ImGui::Spacing();

            ImGui::EndPopup();
        }
    }

    void DrawHierarchyPanel()
    {
        ImGui::Begin("Objects");

        // Toolbar
        if (ImGui::Button("+ New Object"))
        {
            m_SelectedEntity = m_Scene->CreateEntity("New Object");
            // Place the new object at the centre of the editor camera view
            auto& tf = m_SelectedEntity.GetComponent<TransformComponent>();
            auto camPos = m_EditorCamera->GetPosition();
            tf.Position = glm::vec3(camPos.x, camPos.y, 0.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Object") && m_SelectedEntity)
        {
            m_Scene->DestroyEntity(m_SelectedEntity);
            m_SelectedEntity = {};
        }

        ImGui::Separator();

        // Entity list
        Entity entityToDelete;
        auto view = m_Scene->GetRegistry().view<TagComponent>();
        for (auto entity : view)
        {
            Entity e(entity, m_Scene.get());
            auto& tag = e.GetComponent<TagComponent>();

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                                     | ImGuiTreeNodeFlags_SpanAvailWidth
                                     | ImGuiTreeNodeFlags_Leaf;
            if (m_SelectedEntity == e)
                flags |= ImGuiTreeNodeFlags_Selected;

            bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags,
                                            "%s", tag.Tag.c_str());
            if (ImGui::IsItemClicked())
                m_SelectedEntity = e;

            // Right-click context menu
            if (ImGui::BeginPopupContextItem())
            {
                m_SelectedEntity = e;
                if (ImGui::MenuItem("Delete Object"))
                    entityToDelete = e;
                if (ImGui::MenuItem("Duplicate Object"))
                {
                    // Simple duplicate: create a new entity with the same name
                    m_SelectedEntity = m_Scene->CreateEntity(tag.Tag + " (copy)");
                }
                ImGui::EndPopup();
            }

            if (opened)
                ImGui::TreePop();
        }

        // Deferred deletion (safe to do outside the loop)
        if (entityToDelete)
        {
            if (m_SelectedEntity == entityToDelete)
                m_SelectedEntity = {};
            m_Scene->DestroyEntity(entityToDelete);
        }

        ImGui::End();
    }

    void DrawInspectorPanel()
    {
        ImGui::Begin("Properties");

        if (m_SelectedEntity)
        {
            // Tag
            if (m_SelectedEntity.HasComponent<TagComponent>())
            {
                auto& tag = m_SelectedEntity.GetComponent<TagComponent>();
                char buffer[256];
                strcpy(buffer, tag.Tag.c_str());
                if (ImGui::InputText("Name", buffer, sizeof(buffer)))
                {
                    tag.Tag = std::string(buffer);
                }
            }

            ImGui::Separator();

            // Transform
            if (m_SelectedEntity.HasComponent<TransformComponent>())
            {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
                    ImGui::DragFloat3("Start Position", &transform.Position.x, 0.1f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Where this object appears when the game starts.");
                    ImGui::DragFloat3("Rotation", &transform.Rotation.x, 0.1f);
                    ImGui::DragFloat3("Scale", &transform.Scale.x, 0.1f);
                }
            }

            // Sprite Renderer
            if (m_SelectedEntity.HasComponent<SpriteRendererComponent>())
            {
                if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& sprite = m_SelectedEntity.GetComponent<SpriteRendererComponent>();
                    ImGui::ColorEdit4("Color", &sprite.Color.r);

                    // Texture / image picker
                    static char texBuf[256] = "";
                    std::string currentPath = sprite.Texture ? sprite.Texture->GetPath() : "";
                    if (currentPath.size() < sizeof(texBuf))
                        std::copy(currentPath.begin(), currentPath.end(), texBuf),
                        texBuf[currentPath.size()] = '\0';

                    ImGui::SetNextItemWidth(-80.0f);
                    if (ImGui::InputText("##texpath", texBuf, sizeof(texBuf)))
                    {
                        // live-update as user types a valid file
                        if (std::filesystem::exists(texBuf))
                            sprite.Texture = Engine::Texture2D::Create(texBuf);
                        else
                            sprite.Texture = nullptr;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Load Image"))
                    {
                        std::string p(texBuf);
                        if (!p.empty() && std::filesystem::exists(p))
                            sprite.Texture = Engine::Texture2D::Create(p);
                        else if (p.empty())
                            sprite.Texture = nullptr;
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("(?)");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Type a path to a .png or .jpg file, then click Load Image.\nExample: assets/images/player.png");
                    ImGui::TextDisabled("Image file (leave empty for solid color)");
                }
            }

            // Camera
            if (m_SelectedEntity.HasComponent<CameraComponent>())
            {
                if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& camera = m_SelectedEntity.GetComponent<CameraComponent>();
                    ImGui::Checkbox("Primary", &camera.Primary);
                    ImGui::DragFloat("Camera Zoom", &camera.OrthographicSize, 0.1f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("How much of the scene is visible. Larger = zoomed out, smaller = zoomed in.");
                }
            }

            // Rigidbody
            if (m_SelectedEntity.HasComponent<RigidbodyComponent>())
            {
                if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& rb = m_SelectedEntity.GetComponent<RigidbodyComponent>();

                    const char* bodyTypes[] = { "Fixed (doesn't move)", "Scripted (moves by code)", "Physics (gravity + forces)" };
                    int currentType = static_cast<int>(rb.Type);
                    if (ImGui::Combo("Type", &currentType, bodyTypes, 3))
                    {
                        rb.Type = static_cast<RigidbodyComponent::BodyType>(currentType);
                    }

                    ImGui::Checkbox("Lock Rotation", &rb.FixedRotation);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stops the object from spinning when hit by physics");
                    ImGui::DragFloat("Mass", &rb.Mass, 0.01f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("How heavy the object is. Heavier objects are harder to push.");
                    ImGui::DragFloat("Gravity Strength", &rb.GravityScale, 0.01f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("How much gravity affects this object. 0 = floats, 1 = normal, 2 = heavy");
                }
            }

            // Script
            if (m_SelectedEntity.HasComponent<ScriptComponent>())
            {
                if (ImGui::CollapsingHeader("Behaviour (Script)", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& sc = m_SelectedEntity.GetComponent<ScriptComponent>();

                    char pathBuf[512];
                    strncpy(pathBuf, sc.ScriptPath.c_str(), sizeof(pathBuf) - 1);
                    pathBuf[sizeof(pathBuf) - 1] = '\0';
                    if (ImGui::InputText("Script File", pathBuf, sizeof(pathBuf)))
                        sc.ScriptPath = pathBuf;

                    ImGui::Checkbox("Enabled", &sc.Enabled);

                    if (ImGui::Button("Load Script"))
                        LuaScriptEngine::LoadScript(m_Scene.get(), m_SelectedEntity, sc.ScriptPath);
                    ImGui::SameLine();
                    if (ImGui::Button("Reload"))
                        LuaScriptEngine::ReloadScript(m_Scene.get(), m_SelectedEntity);
                    ImGui::SameLine();
                    if (ImGui::Button("Generate with AI..."))
                        ImGui::OpenPopup("AI Script Generator");

                    ImGui::TextDisabled("Behaviours only run when you press Play (Game > Play)");

                    // ---------------------------------------------------------
                    // AI Helper popup
                    // ---------------------------------------------------------
                    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    ImGui::SetNextWindowSize(ImVec2(640, 520), ImGuiCond_Appearing);

                    if (ImGui::BeginPopupModal("AI Script Generator", nullptr,
                                               ImGuiWindowFlags_NoResize))
                    {
                        // API key status banner
                        if (AIHelper::IsClaudeAvailable())
                            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f),
                                "Claude API active (ANTHROPIC_API_KEY is set)");
                        else
                            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f),
                                "Template mode (set ANTHROPIC_API_KEY for Claude AI)");

                        ImGui::Separator();
                        ImGui::Text("Describe what you want in plain English:");
                        ImGui::InputTextMultiline("##desc", m_AIDescription,
                                                  sizeof(m_AIDescription),
                                                  ImVec2(-1, 80));

                        if (ImGui::Button("Generate", ImVec2(120, 0)))
                        {
                            m_AIGenerating = true;
                            m_AIStatusMsg  = "Generating...";
                            m_AIGeneratedCode = AIHelper::GenerateScript(m_AIDescription);
                            m_AIGenerating = false;

                            if (m_AIGeneratedCode.empty())
                                m_AIStatusMsg = "Generation failed.";
                            else
                                m_AIStatusMsg = "Generated via " + AIHelper::GetLastMethod() + ".";
                        }

                        ImGui::SameLine();
                        if (m_AIGenerating)
                            ImGui::TextColored(ImVec4(0.9f,0.9f,0.2f,1), "Working...");
                        else if (!m_AIStatusMsg.empty())
                            ImGui::TextColored(ImVec4(0.5f,0.9f,0.5f,1), "%s", m_AIStatusMsg.c_str());

                        ImGui::Separator();
                        ImGui::Text("Generated script:");
                        ImGui::InputTextMultiline("##code", m_AIGeneratedCode.data(),
                                                  m_AIGeneratedCode.size() + 1,
                                                  ImVec2(-1, 280),
                                                  ImGuiInputTextFlags_ReadOnly);

                        ImGui::Separator();

                        bool canApply = !m_AIGeneratedCode.empty();
                        if (!canApply) ImGui::BeginDisabled();

                        if (ImGui::Button("Apply to Entity", ImVec2(160, 0)))
                        {
                            // Derive a save path from the entity tag
                            std::string tag = "script";
                            if (m_SelectedEntity.HasComponent<Engine::TagComponent>())
                                tag = m_SelectedEntity.GetComponent<Engine::TagComponent>().Tag;

                            // Sanitise for use as a filename
                            for (char& c : tag)
                                if (!std::isalnum(c) && c != '_') c = '_';
                            std::string savePath = "assets/scripts/" + tag + ".lua";

                            std::filesystem::create_directories("assets/scripts");
                            std::ofstream f(savePath);
                            if (f.is_open()) {
                                f << m_AIGeneratedCode;
                                f.close();

                                sc.ScriptPath = savePath;
                                LuaScriptEngine::LoadScript(m_Scene.get(),
                                                            m_SelectedEntity, savePath);
                                m_AIStatusMsg = "Saved and loaded: " + savePath;
                                ImGui::CloseCurrentPopup();
                            } else {
                                m_AIStatusMsg = "Error: could not save " + savePath;
                            }
                        }

                        if (!canApply) ImGui::EndDisabled();

                        ImGui::SameLine();
                        if (ImGui::Button("Cancel", ImVec2(80, 0)))
                        {
                            m_AIGeneratedCode.clear();
                            m_AIStatusMsg.clear();
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }
                }
            }

            // Audio Source
            if (m_SelectedEntity.HasComponent<AudioSourceComponent>())
            {
                if (ImGui::CollapsingHeader("Sound", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& src = m_SelectedEntity.GetComponent<AudioSourceComponent>();

                    char clipBuf[512];
                    strncpy(clipBuf, src.ClipPath.c_str(), sizeof(clipBuf) - 1);
                    clipBuf[sizeof(clipBuf) - 1] = '\0';
                    if (ImGui::InputText("Sound File", clipBuf, sizeof(clipBuf)))
                        src.ClipPath = clipBuf;

                    ImGui::SliderFloat("Volume", &src.Volume, 0.0f, 1.0f);
                    ImGui::SliderFloat("Pitch",  &src.Pitch,  0.1f, 3.0f);
                    ImGui::Checkbox("Loop",         &src.Loop);
                    ImGui::Checkbox("Play On Start", &src.PlayOnStart);
                    ImGui::Checkbox("3D Sound",      &src.Spatial);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Makes the sound get quieter as the listener moves away from this object.");

                    ImGui::Spacing();
                    if (ImGui::Button("Play"))  AudioSystem::Play(m_SelectedEntity);
                    ImGui::SameLine();
                    if (ImGui::Button("Pause")) AudioSystem::Pause(m_SelectedEntity);
                    ImGui::SameLine();
                    if (ImGui::Button("Stop"))  AudioSystem::Stop(m_SelectedEntity);
                    ImGui::TextDisabled("Use these buttons to preview your sound.");
                }
            }

            // Audio Listener
            if (m_SelectedEntity.HasComponent<AudioListenerComponent>())
            {
                if (ImGui::CollapsingHeader("Sound Listener", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::TextDisabled("This object acts as the ears of the scene.");
                    ImGui::TextDisabled("Place it on your main character or camera.");
                }
            }

            // Sprite Animation
            if (m_SelectedEntity.HasComponent<SpriteAnimationComponent>())
            {
                if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& anim = m_SelectedEntity.GetComponent<SpriteAnimationComponent>();

                    char baseBuf[512];
                    strncpy(baseBuf, anim.BasePath.c_str(), sizeof(baseBuf) - 1);
                    baseBuf[sizeof(baseBuf) - 1] = '\0';
                    if (ImGui::InputText("Image Folder", baseBuf, sizeof(baseBuf)))
                        anim.BasePath = baseBuf;

                    ImGui::DragInt("Frame Count",  &anim.FrameCount, 1, 1, 256);
                    ImGui::DragFloat("Speed (s/frame)", &anim.FrameTime, 0.01f, 0.01f, 2.0f);
                    ImGui::Checkbox("Loop",    &anim.Loop);
                    ImGui::Checkbox("Playing", &anim.Playing);

                    ImGui::TextDisabled("Images: %s_0.png to %s_%d.png",
                        anim.BasePath.c_str(), anim.BasePath.c_str(), anim.FrameCount - 1);
                    if (anim.Playing)
                        ImGui::Text("Current frame: %d / %d",
                                    anim.CurrentFrame, (int)anim.Frames.size() - 1);
                }
            }

            // Particle Emitter
            if (m_SelectedEntity.HasComponent<ParticleEmitterComponent>())
            {
                if (ImGui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& e = m_SelectedEntity.GetComponent<ParticleEmitterComponent>();

                    ImGui::DragFloat2("Direction",         &e.Velocity.x,          0.1f);
                    ImGui::DragFloat2("Direction Spread",  &e.VelocityVariation.x,  0.05f);
                    ImGui::ColorEdit4("Start Color",       &e.ColorBegin.r);
                    ImGui::ColorEdit4("End Color",         &e.ColorEnd.r);
                    ImGui::DragFloat("Start Size",   &e.SizeBegin,     0.01f, 0.0f, 5.0f);
                    ImGui::DragFloat("End Size",     &e.SizeEnd,       0.01f, 0.0f, 5.0f);
                    ImGui::DragFloat("Lifetime",     &e.LifeTime,      0.05f, 0.1f, 10.0f);
                    ImGui::DragFloat("Spawn Rate",   &e.EmissionRate,  1.0f, 1.0f, 500.0f);
                    ImGui::Checkbox("Active on Play", &e.Emitting);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Tick to emit particles as soon as Play is pressed.\nUntick and use SetEmitting(true) in a script to trigger at a specific moment.");
                }
            }

            // ---- Visual Novel / Comics ----------------------------------------

            // Text
            if (m_SelectedEntity.HasComponent<TextComponent>())
            {
                if (ImGui::CollapsingHeader("Text", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& tc = m_SelectedEntity.GetComponent<TextComponent>();

                    char textBuf[1024];
                    strncpy(textBuf, tc.Text.c_str(), sizeof(textBuf) - 1);
                    textBuf[sizeof(textBuf) - 1] = '\0';
                    ImGui::TextDisabled("Use \\n for newlines");
                    if (ImGui::InputTextMultiline("Text##tc", textBuf, sizeof(textBuf), ImVec2(-1, 80)))
                        tc.Text = textBuf;

                    ImGui::ColorEdit4("Color##tc", &tc.Color.r);
                    ImGui::DragFloat("Font Size",    &tc.FontSize,    0.01f, 0.05f, 5.0f);
                    ImGui::DragFloat("Line Spacing", &tc.LineSpacing, 0.05f, 0.5f,  3.0f);
                    ImGui::Checkbox("Visible##tc", &tc.Visible);
                }
            }

            // Dialogue Box
            if (m_SelectedEntity.HasComponent<DialogueComponent>())
            {
                if (ImGui::CollapsingHeader("Dialogue Box", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& dlg = m_SelectedEntity.GetComponent<DialogueComponent>();

                    ImGui::Text("Lines: %d  |  Current: %d", (int)dlg.Lines.size(), dlg.CurrentLine);
                    ImGui::Checkbox("Active",       &dlg.Active);
                    ImGui::Checkbox("Auto Advance", &dlg.AutoAdvance);
                    if (dlg.AutoAdvance)
                        ImGui::DragFloat("Auto Time (s)", &dlg.AutoAdvanceTime, 0.1f, 0.5f, 10.0f);

                    char keyBuf[64];
                    strncpy(keyBuf, dlg.AdvanceKey.c_str(), sizeof(keyBuf) - 1);
                    keyBuf[sizeof(keyBuf) - 1] = '\0';
                    if (ImGui::InputText("Advance Key", keyBuf, sizeof(keyBuf)))
                        dlg.AdvanceKey = keyBuf;

                    ImGui::ColorEdit4("Box Color",     &dlg.BoxColor.r);
                    ImGui::ColorEdit4("Text Color",    &dlg.TextColor.r);
                    ImGui::ColorEdit4("Speaker Color", &dlg.SpeakerColor.r);
                    ImGui::DragFloat("Box Height", &dlg.BoxHeight, 0.1f, 0.5f, 10.0f);
                    ImGui::DragFloat("Font Size##dlg", &dlg.FontSize, 0.01f, 0.05f, 1.0f);

                    ImGui::Separator();
                    ImGui::Text("Dialogue Lines:");

                    for (int i = 0; i < (int)dlg.Lines.size(); i++)
                    {
                        ImGui::PushID(i);
                        auto& ln = dlg.Lines[i];

                        char spkBuf[128];
                        strncpy(spkBuf, ln.Speaker.c_str(), sizeof(spkBuf) - 1);
                        spkBuf[sizeof(spkBuf) - 1] = '\0';
                        if (ImGui::InputText("Speaker", spkBuf, sizeof(spkBuf)))
                            ln.Speaker = spkBuf;

                        char txtBuf[512];
                        strncpy(txtBuf, ln.Text.c_str(), sizeof(txtBuf) - 1);
                        txtBuf[sizeof(txtBuf) - 1] = '\0';
                        if (ImGui::InputTextMultiline("##lntxt", txtBuf, sizeof(txtBuf), ImVec2(-1, 60)))
                            ln.Text = txtBuf;

                        if (ImGui::Button("Remove"))
                            dlg.Lines.erase(dlg.Lines.begin() + i);

                        ImGui::Separator();
                        ImGui::PopID();
                    }

                    if (ImGui::Button("Add Line"))
                        dlg.Lines.emplace_back("Speaker", "Dialogue text here...");

                    if (ImGui::Button("Reset to Line 0"))
                    {
                        dlg.CurrentLine = 0;
                        dlg.Active = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Advance"))
                    {
                        dlg.CurrentLine++;
                        if (dlg.CurrentLine >= (int)dlg.Lines.size())
                        {
                            dlg.Active = false;
                            dlg.CurrentLine = 0;
                        }
                    }
                }
            }

            // Comic Panel
            if (m_SelectedEntity.HasComponent<PanelComponent>())
            {
                if (ImGui::CollapsingHeader("Comic Panel", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& panel = m_SelectedEntity.GetComponent<PanelComponent>();
                    ImGui::Checkbox("Show Background", &panel.ShowBackground);
                    if (panel.ShowBackground)
                        ImGui::ColorEdit4("Background##panel", &panel.BackgroundColor.r);
                    ImGui::Checkbox("Show Border", &panel.ShowBorder);
                    if (panel.ShowBorder)
                    {
                        ImGui::ColorEdit4("Border Color##panel", &panel.BorderColor.r);
                        ImGui::DragFloat("Border Width", &panel.BorderWidth, 0.005f, 0.0f, 1.0f);
                    }
                }
            }

            // Timer
            if (m_SelectedEntity.HasComponent<TimerComponent>())
            {
                if (ImGui::CollapsingHeader("Timer", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& t = m_SelectedEntity.GetComponent<TimerComponent>();
                    ImGui::DragFloat("Duration (s)", &t.Duration, 0.5f, 0.0f, 3600.0f);
                    ImGui::Checkbox("Count Down", &t.CountDown);
                    ImGui::Checkbox("Loop", &t.Loop);
                    ImGui::Checkbox("Active", &t.Active);
                    ImGui::Text("Elapsed : %.2f s", t.Elapsed);
                    if (t.CountDown && t.Duration > 0.0f)
                        ImGui::Text("Remaining: %.2f s", t.GetRemaining());
                    if (t.Expired)
                        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "EXPIRED");
                    if (ImGui::Button("Reset Timer"))
                    {
                        t.Elapsed  = 0.0f;
                        t.Expired  = false;
                        t.Active   = true;
                    }
                }
            }

            // Video
            if (m_SelectedEntity.HasComponent<VideoComponent>())
            {
                if (ImGui::CollapsingHeader("Video", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& vc = m_SelectedEntity.GetComponent<VideoComponent>();

                    char urlBuf[512];
                    strncpy(urlBuf, vc.VideoUrl.c_str(), sizeof(urlBuf) - 1);
                    urlBuf[sizeof(urlBuf) - 1] = '\0';
                    if (ImGui::InputText("Video URL", urlBuf, sizeof(urlBuf)))
                        vc.VideoUrl = urlBuf;

                    ImGui::Checkbox("Auto Play",   &vc.AutoPlay);
                    ImGui::Checkbox("Loop##vc",    &vc.Loop);
                    ImGui::Checkbox("Visible##vc", &vc.Visible);
                    ImGui::TextDisabled("In the web player, an HTML5 video overlay is shown.");
                    ImGui::TextDisabled("Use PlayVideo(url) in a Behaviour to trigger at any time.");
                }
            }

            // Circle Collider
            if (m_SelectedEntity.HasComponent<CircleColliderComponent>())
            {
                if (ImGui::CollapsingHeader("Circle Hitbox", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& cc = m_SelectedEntity.GetComponent<CircleColliderComponent>();
                    ImGui::DragFloat("Radius",        &cc.Radius,      0.01f, 0.01f, 10.0f);
                    ImGui::DragFloat2("Offset##cc",   &cc.Offset.x,    0.01f);
                    ImGui::DragFloat("Density##cc",   &cc.Density,     0.01f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("How heavy the material feels. Higher = heavier per unit area.");
                    ImGui::DragFloat("Friction##cc",  &cc.Friction,    0.01f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("How much objects slide. 0 = ice (slippery), 1 = rubber (grippy).");
                    ImGui::DragFloat("Bounciness##cc",&cc.Restitution,  0.01f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("How much this object bounces. 0 = no bounce, 1 = bounces forever.");
                }
            }

            ImGui::Separator();

            // Add Component button
            if (ImGui::Button("Add Component"))
            {
                ImGui::OpenPopup("AddComponent");
            }

            if (ImGui::BeginPopup("AddComponent"))
            {
                if (!m_SelectedEntity.HasComponent<SpriteRendererComponent>())
                {
                    if (ImGui::MenuItem("Appearance (Color / Sprite)"))
                    {
                        m_SelectedEntity.AddComponent<SpriteRendererComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<CameraComponent>())
                {
                    if (ImGui::MenuItem("Camera"))
                    {
                        m_SelectedEntity.AddComponent<CameraComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<RigidbodyComponent>())
                {
                    if (ImGui::MenuItem("Physics Body (gravity, forces)"))
                    {
                        m_SelectedEntity.AddComponent<RigidbodyComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<BoxColliderComponent>())
                {
                    if (ImGui::MenuItem("Box Hitbox (rectangle collision)"))
                    {
                        m_SelectedEntity.AddComponent<BoxColliderComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<CircleColliderComponent>())
                {
                    if (ImGui::MenuItem("Circle Hitbox (round collision)"))
                    {
                        m_SelectedEntity.AddComponent<CircleColliderComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<ScriptComponent>())
                {
                    if (ImGui::MenuItem("Behaviour (Script / AI)"))
                    {
                        m_SelectedEntity.AddComponent<ScriptComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<AudioSourceComponent>())
                {
                    if (ImGui::MenuItem("Sound (plays audio)"))
                    {
                        m_SelectedEntity.AddComponent<AudioSourceComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<AudioListenerComponent>())
                {
                    if (ImGui::MenuItem("Sound Listener (ears of the scene)"))
                    {
                        m_SelectedEntity.AddComponent<AudioListenerComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<SpriteAnimationComponent>())
                {
                    if (ImGui::MenuItem("Animation (flipbook frames)"))
                    {
                        m_SelectedEntity.AddComponent<SpriteAnimationComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<ParticleEmitterComponent>())
                {
                    if (ImGui::MenuItem("Particles (sparks, fire, dust...)"))
                    {
                        m_SelectedEntity.AddComponent<ParticleEmitterComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<TimerComponent>())
                {
                    if (ImGui::MenuItem("Timer (countdown / stopwatch)"))
                    {
                        m_SelectedEntity.AddComponent<TimerComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<VideoComponent>())
                {
                    if (ImGui::MenuItem("Video (web HTML5 video overlay)"))
                    {
                        m_SelectedEntity.AddComponent<VideoComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::Separator();
                ImGui::TextDisabled("Visual Novel / Comics");

                if (!m_SelectedEntity.HasComponent<TextComponent>())
                {
                    if (ImGui::MenuItem("Text"))
                    {
                        m_SelectedEntity.AddComponent<TextComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<DialogueComponent>())
                {
                    if (ImGui::MenuItem("Dialogue Box"))
                    {
                        m_SelectedEntity.AddComponent<DialogueComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<PanelComponent>())
                {
                    if (ImGui::MenuItem("Comic Panel"))
                    {
                        m_SelectedEntity.AddComponent<PanelComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::TextDisabled("No object selected.");
            ImGui::TextDisabled("Click an object in the Objects panel to see its properties.");
        }

        ImGui::End();
    }

    void DrawViewportPanel()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");

        // Check if viewport is hovered and focused
        m_ViewportHovered = ImGui::IsWindowHovered();
        m_ViewportFocused = ImGui::IsWindowFocused();

        // Update editor camera with hover/focus state
        m_EditorCamera->SetViewportHovered(m_ViewportHovered);
        m_EditorCamera->SetViewportFocused(m_ViewportFocused);

        // Get available content region size
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

        // Resize framebuffer and editor camera if needed
        if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0 &&
            (viewportPanelSize.x != m_ViewportSize.x || viewportPanelSize.y != m_ViewportSize.y))
        {
            m_ViewportSize = glm::vec2(viewportPanelSize.x, viewportPanelSize.y);
            m_Framebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
            m_EditorCamera->OnResize(viewportPanelSize.x, viewportPanelSize.y);
        }

        // Display framebuffer texture
        uint64_t textureID = m_Framebuffer->GetColorAttachment();
        ImGui::Image((void*)textureID, viewportPanelSize, ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End();
        ImGui::PopStyleVar();

        // Stats + Profiler window
        ImGui::Begin("Profiler");

        // ---- Renderer stats ----
        if (ImGui::CollapsingHeader("Renderer Stats", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto stats = Renderer2D::GetStats();
            ImGui::Text("Draw Calls : %d", stats.DrawCalls);
            ImGui::Text("Quad Count : %d", stats.QuadCount);
            ImGui::Text("Viewport   : %.0fx%.0f", m_ViewportSize.x, m_ViewportSize.y);
        }

        // ---- Camera info ----
        if (ImGui::CollapsingHeader("Camera"))
        {
            ImGui::Text("Zoom  : %.2f", m_EditorCamera->GetZoomLevel());
            ImGui::Text("Pos   : (%.1f, %.1f, %.1f)",
                m_EditorCamera->GetPosition().x,
                m_EditorCamera->GetPosition().y,
                m_EditorCamera->GetPosition().z);
        }

        // ---- Frame-time profiler ----
        if (ImGui::CollapsingHeader("Frame Time", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Compute stats over the ring buffer
            float minMs = m_FrameTimes[0], maxMs = m_FrameTimes[0], sumMs = 0.0f;
            for (int i = 0; i < k_ProfilerSamples; ++i)
            {
                float v = m_FrameTimes[i];
                if (v < minMs) minMs = v;
                if (v > maxMs) maxMs = v;
                sumMs += v;
            }
            float avgMs  = sumMs / k_ProfilerSamples;
            float avgFPS = (avgMs > 0.0f) ? (1000.0f / avgMs) : 0.0f;

            ImGui::Text("Avg : %.2f ms  (%.0f FPS)", avgMs, avgFPS);
            ImGui::Text("Min : %.2f ms   Max : %.2f ms", minMs, maxMs);

            // Scrolling graph — overlay shows current avg
            char overlay[32];
            snprintf(overlay, sizeof(overlay), "%.1f ms", avgMs);
            ImGui::PlotLines("##ft", m_FrameTimes, k_ProfilerSamples,
                             m_FrameTimeIdx,          // offset (newest sample)
                             overlay,
                             0.0f, maxMs * 1.5f + 1.0f,
                             ImVec2(0.0f, 60.0f));

            // Accumulated FPS since scene start
            if (m_ScenePlaying && m_FrameCount > 0)
            {
                float overallFPS = static_cast<float>(m_FrameCount) / m_FrameTimeAccum;
                ImGui::Text("Session FPS avg : %.0f  (%d frames)", overallFPS, m_FrameCount);
            }

            if (ImGui::Button("Reset"))
            {
                memset(m_FrameTimes, 0, sizeof(m_FrameTimes));
                m_FrameTimeIdx   = 0;
                m_FrameTimeAccum = 0.0f;
                m_FrameCount     = 0;
            }
        }

        ImGui::End();
    }

    void NewScene()
    {
        LuaScriptEngine::OnSceneStop(m_Scene.get());
        AnimationSystem::OnSceneStop(m_Scene.get());
        AudioSystem::OnSceneStop(m_Scene.get());
        ParticleSystem::OnSceneStop(m_Scene.get());
        PhysicsSystem::OnSceneStop(m_Scene.get());
        m_Scene = std::make_unique<Scene>();
        CreateDefaultScene();
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        m_ScenePlaying = false;
    }

    void SaveScene(const std::string& filepath)
    {
        SceneSerializer::Serialize(m_Scene.get(), filepath);
        m_CurrentScenePath = filepath;
        std::cout << "Scene saved to: " << filepath << "\n";
    }

    void LoadScene(const std::string& filepath)
    {
        LuaScriptEngine::OnSceneStop(m_Scene.get());
        AnimationSystem::OnSceneStop(m_Scene.get());
        AudioSystem::OnSceneStop(m_Scene.get());
        ParticleSystem::OnSceneStop(m_Scene.get());
        PhysicsSystem::OnSceneStop(m_Scene.get());
        m_Scene = std::make_unique<Scene>();

        if (SceneSerializer::Deserialize(m_Scene.get(), filepath))
        {
            PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
            m_CurrentScenePath = filepath;
            std::cout << "Scene loaded from: " << filepath << "\n";

            // Select first entity
            auto view = m_Scene->GetRegistry().view<TagComponent>();
            if (!view.empty())
            {
                m_SelectedEntity = Entity(*view.begin(), m_Scene.get());
            }
        }

        m_ScenePlaying = false;
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

        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        std::cout << "Editor shutdown!\n";
    }

    // Override to handle ImGui events
    void HandleEvent(SDL_Event* event)
    {
        ImGui_ImplSDL2_ProcessEvent(event);
    }

private:
    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<SceneRenderer> m_Renderer;
    std::unique_ptr<PhysicsWorld> m_PhysicsWorld;
    std::unique_ptr<Framebuffer> m_Framebuffer;
    std::unique_ptr<EditorCamera> m_EditorCamera;

    Entity m_SelectedEntity;
    bool m_ScenePlaying = false;
    std::string m_CurrentScenePath = "saved_scene.scene";
    glm::vec2 m_ViewportSize = glm::vec2(1280, 720);
    bool m_ViewportHovered = false;
    bool m_ViewportFocused = false;

    // Grid settings
    bool m_ShowGrid = true;
    float m_GridSize = 1.0f;

    // AI Helper state
    char        m_AIDescription[512]  = {};
    std::string m_AIGeneratedCode;
    std::string m_AIStatusMsg;
    bool        m_AIGenerating        = false;

    // Update checker
    UpdateChecker m_UpdateChecker;
    bool          m_ShowUpdateModal      = false;
    bool          m_UpdateToastDismissed = false;

    // Welcome screen
    bool m_ShowWelcome = true;

    // Profiler ring buffer (last 120 frame times)
    static constexpr int k_ProfilerSamples = 120;
    float m_FrameTimes[k_ProfilerSamples]  = {};
    int   m_FrameTimeIdx                   = 0;
    float m_FrameTimeAccum                 = 0.0f;
    int   m_FrameCount                     = 0;
};

int main(int argc, char** argv)
{
    EditorApp app;
    app.Run();
    return 0;
}
