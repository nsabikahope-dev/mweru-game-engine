/**
 * Breakout — full-featured demo built with the custom engine.
 *
 * Features demonstrated:
 *   - ECS (entities, components)
 *   - 2-D batch renderer  (Renderer2D / SceneRenderer)
 *   - Built-in bitmap font (TextComponent HUD)
 *   - Box2D physics        (kinematic paddle, dynamic ball, static bricks/walls)
 *   - Particle system      (color-matched burst on every brick hit)
 *   - 3 unique levels      (classic grid → diamond → checkerboard)
 *   - Score, lives, level  (real-time HUD via TextComponent)
 *   - Ball speed ramp      (ball + tint change as it accelerates)
 *
 * Controls:
 *   LEFT / RIGHT  – move paddle
 *   SPACE         – launch ball  /  advance to next level  /  restart after death
 */

#include <Engine/Core/Application.h>
#include <Engine/ECS/Entity.h>
#include <Engine/ECS/Components.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneRenderer.h>
#include <Engine/Rendering/Renderer2D.h>
#include <Engine/Input/Input.h>
#include <Engine/Physics/PhysicsWorld.h>
#include <Engine/Physics/PhysicsSystem.h>
#include <Engine/Particles/ParticleSystem.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <box2d/b2_body.h>

#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>

using namespace Engine;

// ---------------------------------------------------------------------------
// World-space constants
// Camera OrthographicSize = 20  →  half-height = 10.0
// Aspect 800/600 = 1.333        →  half-width  ≈ 13.3
// ---------------------------------------------------------------------------
static constexpr float HALF_H = 10.0f;
static constexpr float HALF_W = 13.3f;

class BreakoutGame : public Application
{
public:
    BreakoutGame()
        : Application("Breakout — Made with Custom Engine!", 800, 600)
    {}

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------
    void OnInit() override
    {
        m_Scene        = std::make_unique<Scene>();
        m_Renderer     = std::make_unique<SceneRenderer>();
        m_PhysicsWorld = std::make_unique<PhysicsWorld>(glm::vec2(0.0f, 0.0f));

        // Camera — lives forever
        auto cam = m_Scene->CreateEntity("Camera");
        auto& camComp = cam.AddComponent<CameraComponent>();
        camComp.Primary          = true;
        camComp.OrthographicSize = 20.0f;

        CreateHUD();
        StartLevel();
    }

    void OnUpdate(Timestep ts) override
    {
        float dt = ts.GetSeconds();

        if (m_State == State::Playing)
        {
            MovePaddle(dt);
            LaunchBall();
            CheckBallLost();
            CheckBrickCollisions();
            CheckWin();
            TickBurst(dt);
        }
        else if (m_State == State::Won)
        {
            if (Input::IsKeyPressed(KeyCode::Space))
            {
                ++m_Level;
                StartLevel();
            }
        }
        else // Died / GameOver
        {
            if (Input::IsKeyPressed(KeyCode::Space))
            {
                m_Score = 0;
                m_Lives = 3;
                m_Level = 1;
                StartLevel();
            }
        }

        PhysicsSystem::OnUpdate(m_Scene.get(), m_PhysicsWorld.get(), dt);
        ParticleSystem::OnUpdate(m_Scene.get(), dt);
        m_Scene->OnUpdate(dt);
    }

    void OnRender() override
    {
        Renderer2D::ResetStats();
        m_Renderer->RenderScene(m_Scene.get(),
                                GetWindow().GetWidth(),
                                GetWindow().GetHeight());

        // Particles use their own BeginScene / EndScene pass
        glm::mat4 vp = ComputeVP();
        ParticleSystem::OnRender(m_Scene.get(), vp);
    }

    void OnShutdown() override
    {
        ParticleSystem::OnSceneStop(m_Scene.get());
        if (m_PhysicsStarted)
            PhysicsSystem::OnSceneStop(m_Scene.get());
    }

    // -----------------------------------------------------------------------
    // HUD — created once; persists across levels and restarts
    // -----------------------------------------------------------------------
    void CreateHUD()
    {
        // Score — top left
        m_ScoreText = m_Scene->CreateEntity("HUD_Score");
        m_ScoreText.GetComponent<TransformComponent>().Position = { -12.5f, 9.2f, 0.5f };
        auto& st = m_ScoreText.AddComponent<TextComponent>();
        st.FontSize = 0.55f;
        st.Color    = { 1.0f, 1.0f, 1.0f, 1.0f };
        st.Text     = "SCORE: 0";

        // Lives — top right
        m_LivesText = m_Scene->CreateEntity("HUD_Lives");
        m_LivesText.GetComponent<TransformComponent>().Position = { 5.5f, 9.2f, 0.5f };
        auto& lt = m_LivesText.AddComponent<TextComponent>();
        lt.FontSize = 0.55f;
        lt.Color    = { 0.3f, 1.0f, 0.4f, 1.0f };
        lt.Text     = "LIVES: 3";

        // Level indicator — top centre
        m_LevelText = m_Scene->CreateEntity("HUD_Level");
        m_LevelText.GetComponent<TransformComponent>().Position = { -2.0f, 9.2f, 0.5f };
        auto& lv = m_LevelText.AddComponent<TextComponent>();
        lv.FontSize = 0.55f;
        lv.Color    = { 1.0f, 0.9f, 0.2f, 1.0f };
        lv.Text     = "LEVEL 1";

        // Centre message (launch prompt / state feedback)
        m_MsgText = m_Scene->CreateEntity("HUD_Msg");
        m_MsgText.GetComponent<TransformComponent>().Position = { -6.0f, -1.5f, 0.5f };
        auto& mt = m_MsgText.AddComponent<TextComponent>();
        mt.FontSize = 0.65f;
        mt.Color    = { 1.0f, 1.0f, 0.0f, 1.0f };
        mt.Text     = "";
        mt.Visible  = false;

        // Particle burst emitter — repositioned to each brick as it dies
        m_BurstEmitter = m_Scene->CreateEntity("HUD_Burst");
        auto& em = m_BurstEmitter.AddComponent<ParticleEmitterComponent>();
        em.Velocity          = {  0.0f,  2.0f };
        em.VelocityVariation = {  3.5f,  1.8f };
        em.ColorBegin        = {  1.0f,  0.9f, 0.2f, 1.0f };
        em.ColorEnd          = {  0.8f,  0.2f, 0.0f, 0.0f };
        em.SizeBegin         = 0.28f;
        em.SizeEnd           = 0.0f;
        em.LifeTime          = 0.55f;
        em.EmissionRate      = 150.0f;
        em.Emitting          = false;
    }

    // -----------------------------------------------------------------------
    // Level setup
    // -----------------------------------------------------------------------
    void StartLevel()
    {
        // Tear down physics first so bodies are destroyed cleanly
        if (m_PhysicsStarted)
            PhysicsSystem::OnSceneStop(m_Scene.get());

        // Destroy all game entities; keep Camera and HUD_* entities
        {
            std::vector<Entity> doomed;
            auto view = m_Scene->GetRegistry().view<TagComponent>();
            for (auto h : view)
            {
                const std::string& tag = view.get<TagComponent>(h).Tag;
                if (tag != "Camera" && tag.rfind("HUD_", 0) != 0)
                    doomed.emplace_back(h, m_Scene.get());
            }
            for (auto& e : doomed)
                m_Scene->DestroyEntity(e);
        }

        // Reset per-level state
        m_Launched            = false;
        m_BricksLeft          = 0;
        m_HitsSinceSpeedup    = 0;
        m_SpeedScale          = 1.0f;
        m_BurstTimer          = 0.0f;
        m_State               = State::Playing;

        // Reset burst emitter
        if (m_BurstEmitter)
        {
            auto& em = m_BurstEmitter.GetComponent<ParticleEmitterComponent>();
            em.Emitting = false;
            for (auto& p : em.Particles) p.Active = false;
        }

        BuildWalls();
        BuildPaddle();
        BuildBall();
        BuildBricks();

        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        m_PhysicsStarted = true;

        RefreshHUD();
        ShowMsg("PRESS SPACE TO LAUNCH");
    }

    // -----------------------------------------------------------------------
    // Entity builders
    // -----------------------------------------------------------------------
    void BuildWalls()
    {
        auto makeWall = [&](const char* name, glm::vec3 pos, glm::vec3 scale)
        {
            auto e = m_Scene->CreateEntity(name);
            auto& tf  = e.GetComponent<TransformComponent>();
            tf.Position = pos;
            tf.Scale    = scale;
            e.AddComponent<SpriteRendererComponent>().Color = { 0.22f, 0.22f, 0.28f, 1.0f };
            e.AddComponent<RigidbodyComponent>().Type = RigidbodyComponent::BodyType::Static;
            auto& bc  = e.AddComponent<BoxColliderComponent>();
            bc.Restitution = 1.0f;
            bc.Friction    = 0.0f;
        };

        makeWall("Wall_L", { -11.0f,  0.0f, 0.0f }, {  1.0f, 22.0f, 1.0f });
        makeWall("Wall_R", {  11.0f,  0.0f, 0.0f }, {  1.0f, 22.0f, 1.0f });
        makeWall("Wall_T", {   0.0f,  9.5f, 0.0f }, { 24.0f,  1.0f, 1.0f });
    }

    void BuildPaddle()
    {
        m_Paddle = m_Scene->CreateEntity("Paddle");
        auto& tf = m_Paddle.GetComponent<TransformComponent>();
        tf.Position = { 0.0f, -8.0f, 0.0f };
        tf.Scale    = { 3.0f,  0.5f, 1.0f };
        m_Paddle.AddComponent<SpriteRendererComponent>().Color = { 0.2f, 0.85f, 0.25f, 1.0f };
        auto& rb = m_Paddle.AddComponent<RigidbodyComponent>();
        rb.Type          = RigidbodyComponent::BodyType::Kinematic;
        rb.FixedRotation = true;
        auto& bc = m_Paddle.AddComponent<BoxColliderComponent>();
        bc.Restitution = 1.0f;
        bc.Friction    = 0.0f;
    }

    void BuildBall()
    {
        m_Ball = m_Scene->CreateEntity("Ball");
        auto& tf = m_Ball.GetComponent<TransformComponent>();
        tf.Position = { 0.0f, -7.0f, 0.0f };
        tf.Scale    = { 0.4f,  0.4f, 1.0f };
        m_Ball.AddComponent<SpriteRendererComponent>().Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        auto& rb = m_Ball.AddComponent<RigidbodyComponent>();
        rb.Type          = RigidbodyComponent::BodyType::Dynamic;
        rb.FixedRotation = true;
        rb.GravityScale  = 0.0f;
        rb.LinearDamping = 0.0f;
        auto& cc = m_Ball.AddComponent<CircleColliderComponent>();
        cc.Radius      = 0.2f;
        cc.Restitution = 1.0f;
        cc.Friction    = 0.0f;
    }

    void BuildBricks()
    {
        int slot = ((m_Level - 1) % 3) + 1; // cycles 1→2→3→1→...
        if      (slot == 1) BricksLevel1();
        else if (slot == 2) BricksLevel2();
        else                BricksLevel3();
    }

    // Level 1 — classic 5 × 10 grid
    void BricksLevel1()
    {
        static const glm::vec4 colors[] = {
            { 1.0f, 0.18f, 0.18f, 1.0f }, // red    — 50 pts
            { 1.0f, 0.55f, 0.0f,  1.0f }, // orange — 40 pts
            { 1.0f, 1.0f,  0.1f,  1.0f }, // yellow — 30 pts
            { 0.1f, 0.9f,  0.2f,  1.0f }, // green  — 20 pts
            { 0.2f, 0.55f, 1.0f,  1.0f }, // blue   — 10 pts
        };
        static const int pts[] = { 50, 40, 30, 20, 10 };

        const int rows = 5, cols = 10;
        const float bw = 1.8f, bh = 0.5f, gap = 0.2f;
        const float sx = -(cols * (bw + gap)) * 0.5f + bw * 0.5f;
        const float sy = 5.5f;

        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                SpawnBrick(sx + c*(bw+gap), sy - r*(bh+gap), bw, bh, colors[r], pts[r]);
    }

    // Level 2 — diamond / chevron, 7 rows of varying width
    void BricksLevel2()
    {
        static const glm::vec4 colors[] = {
            { 0.9f, 0.1f, 0.9f, 1.0f }, // purple
            { 1.0f, 0.1f, 0.5f, 1.0f }, // pink
            { 1.0f, 0.5f, 0.0f, 1.0f }, // orange
            { 1.0f, 1.0f, 0.0f, 1.0f }, // yellow
            { 0.0f, 0.9f, 0.3f, 1.0f }, // green
            { 0.0f, 0.7f, 1.0f, 1.0f }, // cyan
            { 0.4f, 0.3f, 1.0f, 1.0f }, // blue
        };
        static const int pts[]    = { 70, 60, 50, 40, 50, 60, 70 };
        static const int widths[] = {  5,  7,  9, 11,  9,  7,  5 };

        const float bw = 1.6f, bh = 0.45f, gap = 0.2f;
        const float sy = 6.3f;

        for (int r = 0; r < 7; ++r)
        {
            int w = widths[r];
            float sx = -(w * (bw + gap)) * 0.5f + bw * 0.5f;
            for (int c = 0; c < w; ++c)
                SpawnBrick(sx + c*(bw+gap), sy - r*(bh+gap), bw, bh, colors[r], pts[r]);
        }
    }

    // Level 3 — checkerboard, 6 × 11 with dark / bright alternating pairs
    void BricksLevel3()
    {
        static const glm::vec4 bright[] = {
            { 1.0f, 0.0f, 0.0f, 1.0f },
            { 1.0f, 0.6f, 0.0f, 1.0f },
            { 0.9f, 0.9f, 0.0f, 1.0f },
            { 0.0f, 0.9f, 0.1f, 1.0f },
            { 0.0f, 0.6f, 1.0f, 1.0f },
            { 0.7f, 0.0f, 1.0f, 1.0f },
        };
        static const glm::vec4 dark[] = {
            { 0.5f, 0.0f, 0.0f, 1.0f },
            { 0.5f, 0.3f, 0.0f, 1.0f },
            { 0.5f, 0.5f, 0.0f, 1.0f },
            { 0.0f, 0.5f, 0.0f, 1.0f },
            { 0.0f, 0.3f, 0.6f, 1.0f },
            { 0.4f, 0.0f, 0.6f, 1.0f },
        };
        static const int pts[] = { 80, 70, 60, 60, 70, 80 };

        const int rows = 6, cols = 11;
        const float bw = 1.63f, bh = 0.45f, gap = 0.18f;
        const float sx = -(cols * (bw + gap)) * 0.5f + bw * 0.5f;
        const float sy = 6.1f;

        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                SpawnBrick(sx + c*(bw+gap), sy - r*(bh+gap), bw, bh,
                           ((r + c) % 2 == 0 ? bright[r] : dark[r]), pts[r]);
    }

    // Shared brick factory — tag format: "Brick_<pts>" for fast parsing
    void SpawnBrick(float x, float y, float bw, float bh, glm::vec4 color, int points)
    {
        auto e  = m_Scene->CreateEntity("Brick_" + std::to_string(points));
        auto& tf = e.GetComponent<TransformComponent>();
        tf.Position = { x, y, 0.0f };
        tf.Scale    = { bw, bh, 1.0f };
        e.AddComponent<SpriteRendererComponent>().Color = color;
        auto& rb = e.AddComponent<RigidbodyComponent>();
        rb.Type = RigidbodyComponent::BodyType::Static;
        auto& bc = e.AddComponent<BoxColliderComponent>();
        bc.Restitution = 1.0f;
        bc.Friction    = 0.0f;
        ++m_BricksLeft;
    }

    // -----------------------------------------------------------------------
    // Per-frame game logic
    // -----------------------------------------------------------------------
    void MovePaddle(float /*dt*/)
    {
        auto& tf = m_Paddle.GetComponent<TransformComponent>();
        auto& rb = m_Paddle.GetComponent<RigidbodyComponent>();

        // Paddle half-width = Scale.x / 2 = 1.5
        // Wall inner edges: left = -10.5, right = +10.5
        const float halfW      = tf.Scale.x * 0.5f;
        const float leftLimit  = -10.5f + halfW;   // -9.0
        const float rightLimit =  10.5f - halfW;   // +9.0

        b2Vec2 vel(0.0f, 0.0f);
        if (Input::IsKeyHeld(KeyCode::Left))  vel.x = -12.0f;
        if (Input::IsKeyHeld(KeyCode::Right)) vel.x =  12.0f;

        if (rb.RuntimeBody)
        {
            float px = rb.RuntimeBody->GetPosition().x;

            // Cancel velocity that would push paddle further into a wall
            if (vel.x < 0.0f && px <= leftLimit)  vel.x = 0.0f;
            if (vel.x > 0.0f && px >= rightLimit) vel.x = 0.0f;

            rb.RuntimeBody->SetLinearVelocity(vel);

            // Hard-snap position if somehow already past boundary
            if (px < leftLimit || px > rightLimit)
            {
                float snapped = glm::clamp(px, leftLimit, rightLimit);
                rb.RuntimeBody->SetTransform({ snapped, rb.RuntimeBody->GetPosition().y }, 0.0f);
                rb.RuntimeBody->SetLinearVelocity({ 0.0f, 0.0f });
            }
        }

        // Ball follows paddle until launched
        if (!m_Launched && m_Ball)
            m_Ball.GetComponent<TransformComponent>().Position.x = tf.Position.x;
    }

    void LaunchBall()
    {
        if (m_Launched || !Input::IsKeyPressed(KeyCode::Space))
            return;

        auto& rb = m_Ball.GetComponent<RigidbodyComponent>();
        if (!rb.RuntimeBody) return;

        float speed = m_BaseLaunch * m_SpeedScale;
        rb.RuntimeBody->SetLinearVelocity({ speed * 0.55f, speed * 0.84f });
        m_Launched = true;
        HideMsg();
    }

    void CheckBallLost()
    {
        auto& pos = m_Ball.GetComponent<TransformComponent>().Position;
        if (pos.y > -11.5f) return;

        --m_Lives;
        RefreshHUD();

        if (m_Lives <= 0)
        {
            m_State = State::Dead;
            ShowMsg("GAME OVER  SPACE TO RETRY");
            return;
        }

        // Reset paddle + ball position without tearing down the whole level
        auto& pTf = m_Paddle.GetComponent<TransformComponent>();
        pTf.Position.x = 0.0f;
        pos = { 0.0f, -7.0f, 0.0f };

        auto& pRb = m_Paddle.GetComponent<RigidbodyComponent>();
        auto& bRb = m_Ball.GetComponent<RigidbodyComponent>();
        if (pRb.RuntimeBody)
        {
            pRb.RuntimeBody->SetTransform({ 0.0f, -8.0f }, 0.0f);
            pRb.RuntimeBody->SetLinearVelocity({ 0.0f, 0.0f });
        }
        if (bRb.RuntimeBody)
        {
            bRb.RuntimeBody->SetTransform({ 0.0f, -7.0f }, 0.0f);
            bRb.RuntimeBody->SetLinearVelocity({ 0.0f, 0.0f });
        }

        m_Launched = false;
        ShowMsg("PRESS SPACE TO LAUNCH");
    }

    void CheckBrickCollisions()
    {
        auto& bt  = m_Ball.GetComponent<TransformComponent>();
        auto& bRb = m_Ball.GetComponent<RigidbodyComponent>();

        auto view = m_Scene->GetRegistry().view<TagComponent, TransformComponent>();

        Entity hitBrick;
        int    hitPts = 0;
        float  bestDistX = 1e9f;

        // Find the nearest overlapping brick (take only one per frame)
        for (auto h : view)
        {
            const std::string& tag = view.get<TagComponent>(h).Tag;
            if (tag.rfind("Brick_", 0) != 0) continue;

            auto& brickTf = view.get<TransformComponent>(h);
            float distX   = std::abs(bt.Position.x - brickTf.Position.x);
            float distY   = std::abs(bt.Position.y - brickTf.Position.y);
            float halfW   = brickTf.Scale.x * 0.5f + 0.2f; // +ball radius
            float halfH   = brickTf.Scale.y * 0.5f + 0.2f;

            if (distX < halfW && distY < halfH && distX < bestDistX)
            {
                bestDistX = distX;
                hitBrick  = Entity(h, m_Scene.get());

                // Velocity reflection
                if (bRb.RuntimeBody)
                {
                    b2Vec2 v = bRb.RuntimeBody->GetLinearVelocity();
                    if (distX / halfW > distY / halfH)
                        v.x = -v.x;
                    else
                        v.y = -v.y;
                    bRb.RuntimeBody->SetLinearVelocity(v);
                }

                // Parse point value from "Brick_<pts>"
                size_t us = tag.rfind('_');
                if (us != std::string::npos)
                {
                    try { hitPts = std::stoi(tag.substr(us + 1)); }
                    catch(...) { hitPts = 10; }
                }
                break;
            }
        }

        if (!hitBrick) return;

        // Burst particles matching the brick's colour
        glm::vec4 brickColor = hitBrick.GetComponent<SpriteRendererComponent>().Color;
        TriggerBurst(hitBrick.GetComponent<TransformComponent>().Position, brickColor);

        m_Scene->DestroyEntity(hitBrick);
        --m_BricksLeft;
        m_Score += hitPts;
        ++m_HitsSinceSpeedup;

        // Speed ramp — every 7 bricks
        if (m_HitsSinceSpeedup >= 7)
        {
            m_SpeedScale       = std::min(m_SpeedScale * 1.12f, 1.8f);
            m_HitsSinceSpeedup = 0;

            // Normalise + scale live ball velocity
            if (bRb.RuntimeBody)
            {
                b2Vec2 v   = bRb.RuntimeBody->GetLinearVelocity();
                float  len = std::sqrt(v.x*v.x + v.y*v.y);
                if (len > 0.001f)
                {
                    float target = m_BaseLaunch * m_SpeedScale;
                    v.x = (v.x / len) * target;
                    v.y = (v.y / len) * target;
                    bRb.RuntimeBody->SetLinearVelocity(v);
                }
            }

            // Tint ball: white → orange → red as it speeds up
            if (m_Ball)
            {
                float t = (m_SpeedScale - 1.0f) / 0.8f; // 0 → 1
                t = glm::clamp(t, 0.0f, 1.0f);
                m_Ball.GetComponent<SpriteRendererComponent>().Color = {
                    1.0f, 1.0f - t * 0.75f, 1.0f - t, 1.0f
                };
            }
        }

        RefreshHUD();
    }

    void CheckWin()
    {
        if (m_BricksLeft <= 0 && m_State == State::Playing)
        {
            m_State = State::Won;
            ShowMsg("LEVEL CLEAR!  SPACE FOR NEXT");
        }
    }

    // -----------------------------------------------------------------------
    // Particle burst helpers
    // -----------------------------------------------------------------------
    void TriggerBurst(glm::vec3 pos, glm::vec4 color)
    {
        if (!m_BurstEmitter) return;
        m_BurstEmitter.GetComponent<TransformComponent>().Position = pos;
        auto& em       = m_BurstEmitter.GetComponent<ParticleEmitterComponent>();
        em.ColorBegin  = { color.r, color.g, color.b, 1.0f };
        em.ColorEnd    = { color.r * 0.4f, color.g * 0.4f, color.b * 0.4f, 0.0f };
        em.Emitting    = true;
        m_BurstTimer   = 0.18f;
    }

    void TickBurst(float dt)
    {
        if (m_BurstTimer <= 0.0f || !m_BurstEmitter) return;
        m_BurstTimer -= dt;
        if (m_BurstTimer <= 0.0f)
        {
            m_BurstTimer = 0.0f;
            m_BurstEmitter.GetComponent<ParticleEmitterComponent>().Emitting = false;
        }
    }

    // -----------------------------------------------------------------------
    // HUD helpers
    // -----------------------------------------------------------------------
    void RefreshHUD()
    {
        if (m_ScoreText)
            m_ScoreText.GetComponent<TextComponent>().Text =
                "SCORE: " + std::to_string(m_Score);
        if (m_LivesText)
            m_LivesText.GetComponent<TextComponent>().Text =
                "LIVES: " + std::to_string(m_Lives);
        if (m_LevelText)
            m_LevelText.GetComponent<TextComponent>().Text =
                "LEVEL " + std::to_string(m_Level);
    }

    void ShowMsg(const std::string& msg)
    {
        if (!m_MsgText) return;
        auto& tc = m_MsgText.GetComponent<TextComponent>();
        tc.Text    = msg;
        tc.Visible = true;
    }

    void HideMsg()
    {
        if (m_MsgText)
            m_MsgText.GetComponent<TextComponent>().Visible = false;
    }

    // -----------------------------------------------------------------------
    // Helper — compute view-projection for particle pass
    // -----------------------------------------------------------------------
    glm::mat4 ComputeVP()
    {
        auto camView = m_Scene->GetRegistry().view<TransformComponent, CameraComponent>();
        for (auto e : camView)
        {
            auto& cam = camView.get<CameraComponent>(e);
            if (!cam.Primary) continue;
            auto& tf = camView.get<TransformComponent>(e);
            float ar = (float)GetWindow().GetWidth() / (float)GetWindow().GetHeight();
            return cam.GetProjection(ar) * glm::inverse(tf.GetTransform());
        }
        // Fallback
        float ar = (float)GetWindow().GetWidth() / (float)GetWindow().GetHeight();
        return glm::ortho(-ar * HALF_H, ar * HALF_H, -HALF_H, HALF_H, -1.0f, 1.0f);
    }

private:
    enum class State { Playing, Won, Dead };

    // Engine objects
    std::unique_ptr<Scene>         m_Scene;
    std::unique_ptr<SceneRenderer> m_Renderer;
    std::unique_ptr<PhysicsWorld>  m_PhysicsWorld;

    // Game entities
    Entity m_Paddle;
    Entity m_Ball;

    // HUD entities (persist across levels)
    Entity m_ScoreText;
    Entity m_LivesText;
    Entity m_LevelText;
    Entity m_MsgText;
    Entity m_BurstEmitter;

    // Game state
    State m_State          = State::Playing;
    int   m_Score          = 0;
    int   m_Lives          = 3;
    int   m_Level          = 1;
    int   m_BricksLeft     = 0;

    // Per-level tracking
    bool  m_Launched           = false;
    bool  m_PhysicsStarted     = false;
    int   m_HitsSinceSpeedup   = 0;
    float m_SpeedScale         = 1.0f;
    float m_BurstTimer         = 0.0f;

    static constexpr float m_BaseLaunch = 9.5f;
};

int main()
{
    BreakoutGame game;
    game.Run();
    return 0;
}
