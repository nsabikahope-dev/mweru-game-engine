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
#include <Engine/Scripting/LuaScriptEngine.h>
#include <Engine/Audio/AudioEngine.h>
#include <Engine/Audio/AudioSystem.h>
#include <Engine/Animation/AnimationSystem.h>
#include <Engine/Particles/ParticleSystem.h>

#include "../include/EditorCamera.h"
#include "../include/GridRenderer.h"
#include "../include/AIHelper.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <SDL2/SDL.h>
#include <glad/glad.h>

#include <iostream>
#include <fstream>
#include <cctype>
#include <memory>

using namespace Engine;

class EditorApp : public Application
{
public:
    EditorApp()
        : Application("Game Engine Editor", 1920, 1080)
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
            PhysicsSystem::OnUpdate(m_Scene.get(), m_PhysicsWorld.get(), ts.GetSeconds());
            LuaScriptEngine::OnUpdate(m_Scene.get(), ts.GetSeconds());
            AudioSystem::OnUpdate(m_Scene.get());
            AnimationSystem::OnUpdate(m_Scene.get(), ts.GetSeconds());
            ParticleSystem::OnUpdate(m_Scene.get(), ts.GetSeconds());
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

            if (ImGui::BeginMenu("Scene"))
            {
                if (ImGui::MenuItem("Play", nullptr, m_ScenePlaying))
                {
                    m_ScenePlaying = !m_ScenePlaying;
                    if (m_ScenePlaying)
                    {
                        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
                        AudioSystem::OnSceneStart(m_Scene.get());
                        AnimationSystem::OnSceneStart(m_Scene.get());
                        LuaScriptEngine::OnSceneStart(m_Scene.get());
                    }
                    else
                    {
                        LuaScriptEngine::OnSceneStop(m_Scene.get());
                        AnimationSystem::OnSceneStop(m_Scene.get());
                        AudioSystem::OnSceneStop(m_Scene.get());
                        ParticleSystem::OnSceneStop(m_Scene.get());
                        PhysicsSystem::OnSceneStop(m_Scene.get());
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

            ImGui::EndMainMenuBar();
        }

        // Hierarchy Panel
        DrawHierarchyPanel();

        // Inspector Panel
        DrawInspectorPanel();

        // Viewport Panel
        DrawViewportPanel();

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

    void DrawHierarchyPanel()
    {
        ImGui::Begin("Hierarchy");

        // Toolbar
        if (ImGui::Button("+ Entity"))
            m_SelectedEntity = m_Scene->CreateEntity("New Entity");
        ImGui::SameLine();
        if (ImGui::Button("- Delete") && m_SelectedEntity)
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
                if (ImGui::MenuItem("Delete Entity"))
                    entityToDelete = e;
                if (ImGui::MenuItem("Duplicate Entity"))
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
        ImGui::Begin("Inspector");

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
                    ImGui::DragFloat3("Position", &transform.Position.x, 0.1f);
                    ImGui::DragFloat3("Rotation", &transform.Rotation.x, 0.1f);
                    ImGui::DragFloat3("Scale", &transform.Scale.x, 0.1f);
                }
            }

            // Sprite Renderer
            if (m_SelectedEntity.HasComponent<SpriteRendererComponent>())
            {
                if (ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& sprite = m_SelectedEntity.GetComponent<SpriteRendererComponent>();
                    ImGui::ColorEdit4("Color", &sprite.Color.r);
                }
            }

            // Camera
            if (m_SelectedEntity.HasComponent<CameraComponent>())
            {
                if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& camera = m_SelectedEntity.GetComponent<CameraComponent>();
                    ImGui::Checkbox("Primary", &camera.Primary);
                    ImGui::DragFloat("Orthographic Size", &camera.OrthographicSize, 0.1f);
                }
            }

            // Rigidbody
            if (m_SelectedEntity.HasComponent<RigidbodyComponent>())
            {
                if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& rb = m_SelectedEntity.GetComponent<RigidbodyComponent>();

                    const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
                    int currentType = static_cast<int>(rb.Type);
                    if (ImGui::Combo("Type", &currentType, bodyTypes, 3))
                    {
                        rb.Type = static_cast<RigidbodyComponent::BodyType>(currentType);
                    }

                    ImGui::Checkbox("Fixed Rotation", &rb.FixedRotation);
                    ImGui::DragFloat("Mass", &rb.Mass, 0.01f);
                    ImGui::DragFloat("Gravity Scale", &rb.GravityScale, 0.01f);
                }
            }

            // Script
            if (m_SelectedEntity.HasComponent<ScriptComponent>())
            {
                if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& sc = m_SelectedEntity.GetComponent<ScriptComponent>();

                    char pathBuf[512];
                    strncpy(pathBuf, sc.ScriptPath.c_str(), sizeof(pathBuf) - 1);
                    pathBuf[sizeof(pathBuf) - 1] = '\0';
                    if (ImGui::InputText("Script Path", pathBuf, sizeof(pathBuf)))
                        sc.ScriptPath = pathBuf;

                    ImGui::Checkbox("Enabled", &sc.Enabled);

                    if (ImGui::Button("Load Script"))
                        LuaScriptEngine::LoadScript(m_Scene.get(), m_SelectedEntity, sc.ScriptPath);
                    ImGui::SameLine();
                    if (ImGui::Button("Reload"))
                        LuaScriptEngine::ReloadScript(m_Scene.get(), m_SelectedEntity);
                    ImGui::SameLine();
                    if (ImGui::Button("AI Helper..."))
                        ImGui::OpenPopup("AI Script Generator");

                    ImGui::TextDisabled("Scripts run when scene is playing (Scene > Play)");

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
                if (ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& src = m_SelectedEntity.GetComponent<AudioSourceComponent>();

                    char clipBuf[512];
                    strncpy(clipBuf, src.ClipPath.c_str(), sizeof(clipBuf) - 1);
                    clipBuf[sizeof(clipBuf) - 1] = '\0';
                    if (ImGui::InputText("Clip Path", clipBuf, sizeof(clipBuf)))
                        src.ClipPath = clipBuf;

                    ImGui::SliderFloat("Volume", &src.Volume, 0.0f, 1.0f);
                    ImGui::SliderFloat("Pitch",  &src.Pitch,  0.1f, 3.0f);
                    ImGui::Checkbox("Loop",         &src.Loop);
                    ImGui::Checkbox("Play On Start", &src.PlayOnStart);
                    ImGui::Checkbox("Spatial (3D)",  &src.Spatial);

                    ImGui::Spacing();
                    if (ImGui::Button("Play"))  AudioSystem::Play(m_SelectedEntity);
                    ImGui::SameLine();
                    if (ImGui::Button("Pause")) AudioSystem::Pause(m_SelectedEntity);
                    ImGui::SameLine();
                    if (ImGui::Button("Stop"))  AudioSystem::Stop(m_SelectedEntity);
                    ImGui::TextDisabled("Playback controls work during scene play.");
                }
            }

            // Audio Listener
            if (m_SelectedEntity.HasComponent<AudioListenerComponent>())
            {
                if (ImGui::CollapsingHeader("Audio Listener", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::TextDisabled("This entity acts as the audio listener.");
                    ImGui::TextDisabled("Position is taken from its Transform.");
                }
            }

            // Sprite Animation
            if (m_SelectedEntity.HasComponent<SpriteAnimationComponent>())
            {
                if (ImGui::CollapsingHeader("Sprite Animation", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& anim = m_SelectedEntity.GetComponent<SpriteAnimationComponent>();

                    char baseBuf[512];
                    strncpy(baseBuf, anim.BasePath.c_str(), sizeof(baseBuf) - 1);
                    baseBuf[sizeof(baseBuf) - 1] = '\0';
                    if (ImGui::InputText("Base Path", baseBuf, sizeof(baseBuf)))
                        anim.BasePath = baseBuf;

                    ImGui::DragInt("Frame Count",  &anim.FrameCount, 1, 1, 256);
                    ImGui::DragFloat("Frame Time (s)", &anim.FrameTime, 0.01f, 0.01f, 2.0f);
                    ImGui::Checkbox("Loop",    &anim.Loop);
                    ImGui::Checkbox("Playing", &anim.Playing);

                    ImGui::TextDisabled("Frames: %s_0.png … %s_%d.png",
                        anim.BasePath.c_str(), anim.BasePath.c_str(), anim.FrameCount - 1);
                    ImGui::TextDisabled("Or set FramePaths explicitly in code.");
                    if (anim.Playing)
                        ImGui::Text("Current frame: %d / %d",
                                    anim.CurrentFrame, (int)anim.Frames.size() - 1);
                }
            }

            // Particle Emitter
            if (m_SelectedEntity.HasComponent<ParticleEmitterComponent>())
            {
                if (ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& e = m_SelectedEntity.GetComponent<ParticleEmitterComponent>();

                    ImGui::DragFloat2("Velocity",          &e.Velocity.x,          0.1f);
                    ImGui::DragFloat2("Velocity Variation",&e.VelocityVariation.x,  0.05f);
                    ImGui::ColorEdit4("Color Begin",       &e.ColorBegin.r);
                    ImGui::ColorEdit4("Color End",         &e.ColorEnd.r);
                    ImGui::DragFloat("Size Begin",  &e.SizeBegin,     0.01f, 0.0f, 5.0f);
                    ImGui::DragFloat("Size End",    &e.SizeEnd,       0.01f, 0.0f, 5.0f);
                    ImGui::DragFloat("Life Time",   &e.LifeTime,      0.05f, 0.1f, 10.0f);
                    ImGui::DragFloat("Emission Rate", &e.EmissionRate, 1.0f, 1.0f, 500.0f);
                    ImGui::Checkbox("Emitting", &e.Emitting);
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

            // Circle Collider
            if (m_SelectedEntity.HasComponent<CircleColliderComponent>())
            {
                if (ImGui::CollapsingHeader("Circle Collider", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& cc = m_SelectedEntity.GetComponent<CircleColliderComponent>();
                    ImGui::DragFloat("Radius",      &cc.Radius,      0.01f, 0.01f, 10.0f);
                    ImGui::DragFloat2("Offset##cc", &cc.Offset.x,    0.01f);
                    ImGui::DragFloat("Density##cc", &cc.Density,     0.01f);
                    ImGui::DragFloat("Friction##cc",&cc.Friction,    0.01f);
                    ImGui::DragFloat("Restitution##cc",&cc.Restitution,0.01f);
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
                    if (ImGui::MenuItem("Sprite Renderer"))
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
                    if (ImGui::MenuItem("Rigidbody"))
                    {
                        m_SelectedEntity.AddComponent<RigidbodyComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<BoxColliderComponent>())
                {
                    if (ImGui::MenuItem("Box Collider"))
                    {
                        m_SelectedEntity.AddComponent<BoxColliderComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<CircleColliderComponent>())
                {
                    if (ImGui::MenuItem("Circle Collider"))
                    {
                        m_SelectedEntity.AddComponent<CircleColliderComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<ScriptComponent>())
                {
                    if (ImGui::MenuItem("Script"))
                    {
                        m_SelectedEntity.AddComponent<ScriptComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<AudioSourceComponent>())
                {
                    if (ImGui::MenuItem("Audio Source"))
                    {
                        m_SelectedEntity.AddComponent<AudioSourceComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<AudioListenerComponent>())
                {
                    if (ImGui::MenuItem("Audio Listener"))
                    {
                        m_SelectedEntity.AddComponent<AudioListenerComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<SpriteAnimationComponent>())
                {
                    if (ImGui::MenuItem("Sprite Animation"))
                    {
                        m_SelectedEntity.AddComponent<SpriteAnimationComponent>();
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (!m_SelectedEntity.HasComponent<ParticleEmitterComponent>())
                {
                    if (ImGui::MenuItem("Particle Emitter"))
                    {
                        m_SelectedEntity.AddComponent<ParticleEmitterComponent>();
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
            ImGui::Text("No entity selected");
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
