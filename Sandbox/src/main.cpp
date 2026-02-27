#include <Engine/Core/Application.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneRenderer.h>
#include <Engine/ECS/Entity.h>
#include <Engine/ECS/Components.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/Renderer2D.h>
#include <Engine/Input/Input.h>

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <memory>
#include <iostream>
#include <random>
#include <sstream>

// System demos
void AssetSystemDemo();
void InputSystemDemo();
#include "../AssetSystemDemo.cpp"
#include "../InputSystemDemo.cpp"

class SandboxApp : public Engine::Application
{
public:
    SandboxApp()
        : Engine::Application("Phase 3: Batch Renderer - 10,000 Sprites", 1920, 1080)
    {
    }

    void OnInit() override
    {
        // Run system demos
        AssetSystemDemo();
        InputSystemDemo();

        std::cout << "===========================================\n";
        std::cout << "  Phase 3: Advanced Rendering Stress Test\n";
        std::cout << "===========================================\n";
        std::cout << "Controls:\n";
        std::cout << "  WASD - Move camera\n";
        std::cout << "  Q/E - Zoom in/out\n";
        std::cout << "  SPACE - Toggle sprite animation\n";
        std::cout << "  1-5 - Change sprite count\n";
        std::cout << "\n";

        // Create scene and renderer
        m_Scene = std::make_unique<Engine::Scene>();
        m_Renderer = std::make_unique<Engine::SceneRenderer>();

        // Create camera
        m_Camera = m_Scene->CreateEntity("Camera");
        auto& cameraComponent = m_Camera.AddComponent<Engine::CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.OrthographicSize = 50.0f;

        // Create stress test sprites
        CreateSprites(10000);

        std::cout << "Stress test initialized!\n";
        std::cout << "Press 1-5 to adjust sprite count:\n";
        std::cout << "  1 = 1,000 sprites\n";
        std::cout << "  2 = 5,000 sprites\n";
        std::cout << "  3 = 10,000 sprites\n";
        std::cout << "  4 = 20,000 sprites\n";
        std::cout << "  5 = 50,000 sprites\n";
    }

    void CreateSprites(int count)
    {
        // Clear existing sprites
        auto view = m_Scene->GetRegistry().view<Engine::SpriteRendererComponent>();
        for (auto entity : view)
        {
            m_Scene->DestroyEntity(Engine::Entity(entity, m_Scene.get()));
        }

        std::cout << "Creating " << count << " sprites...\n";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posX(-100.0f, 100.0f);
        std::uniform_real_distribution<float> posY(-60.0f, 60.0f);
        std::uniform_real_distribution<float> size(0.3f, 1.2f);
        std::uniform_real_distribution<float> color(0.2f, 1.0f);

        for (int i = 0; i < count; i++)
        {
            auto sprite = m_Scene->CreateEntity("Sprite");
            auto& transform = sprite.GetComponent<Engine::TransformComponent>();
            transform.Position = glm::vec3(posX(gen), posY(gen), 0.0f);
            transform.Scale = glm::vec3(size(gen));

            auto& spriteComp = sprite.AddComponent<Engine::SpriteRendererComponent>();
            spriteComp.Color = glm::vec4(color(gen), color(gen), color(gen), 1.0f);
        }

        m_SpriteCount = count;
        std::cout << "Created " << count << " sprites!\n";
    }

    void OnUpdate(Engine::Timestep ts) override
    {
        using namespace Engine;

        // Handle sprite count changes with new Input API
        if (Input::IsKeyPressed(KeyCode::Num1)) CreateSprites(1000);
        if (Input::IsKeyPressed(KeyCode::Num2)) CreateSprites(5000);
        if (Input::IsKeyPressed(KeyCode::Num3)) CreateSprites(10000);
        if (Input::IsKeyPressed(KeyCode::Num4)) CreateSprites(20000);
        if (Input::IsKeyPressed(KeyCode::Num5)) CreateSprites(50000);

        // Toggle sprite animation
        if (Input::IsKeyPressed(KeyCode::Space))
            m_AnimateSprites = !m_AnimateSprites;

        // Camera movement (WASD)
        auto& cameraTransform = m_Camera.GetComponent<TransformComponent>();
        float cameraSpeed = 30.0f * ts.GetSeconds();

        if (Input::IsKeyHeld(KeyCode::A))
            cameraTransform.Position.x -= cameraSpeed;
        if (Input::IsKeyHeld(KeyCode::D))
            cameraTransform.Position.x += cameraSpeed;
        if (Input::IsKeyHeld(KeyCode::W))
            cameraTransform.Position.y += cameraSpeed;
        if (Input::IsKeyHeld(KeyCode::S))
            cameraTransform.Position.y -= cameraSpeed;

        // Camera zoom (Q/E)
        auto& cameraComponent = m_Camera.GetComponent<CameraComponent>();
        float zoomSpeed = 20.0f * ts.GetSeconds();

        if (Input::IsKeyHeld(KeyCode::Q))
            cameraComponent.OrthographicSize -= zoomSpeed;
        if (Input::IsKeyHeld(KeyCode::E))
            cameraComponent.OrthographicSize += zoomSpeed;

        // Clamp zoom
        if (cameraComponent.OrthographicSize < 5.0f)
            cameraComponent.OrthographicSize = 5.0f;
        if (cameraComponent.OrthographicSize > 200.0f)
            cameraComponent.OrthographicSize = 200.0f;

        // Animate sprites if enabled
        if (m_AnimateSprites)
        {
            auto view = m_Scene->GetRegistry().view<Engine::TransformComponent, Engine::SpriteRendererComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<Engine::TransformComponent>(entity);
                transform.Rotation.z += ts.GetSeconds() * 0.5f;
            }
        }

        // Calculate FPS
        m_FrameCount++;
        m_TimeSinceLastFPS += ts.GetSeconds();
        if (m_TimeSinceLastFPS >= 1.0f)
        {
            m_FPS = m_FrameCount / m_TimeSinceLastFPS;
            m_FrameCount = 0;
            m_TimeSinceLastFPS = 0.0f;

            // Update window title with stats
            auto stats = Engine::Renderer2D::GetStats();
            std::stringstream title;
            title << "Phase 3: Batch Renderer | Sprites: " << m_SpriteCount
                  << " | FPS: " << static_cast<int>(m_FPS)
                  << " | Draw Calls: " << stats.DrawCalls
                  << " | Quads: " << stats.QuadCount;
            SDL_SetWindowTitle(GetWindow().GetNativeWindow(), title.str().c_str());
        }

        // Update scene
        m_Scene->OnUpdate(ts.GetSeconds());
    }

    void OnRender() override
    {
        // Reset stats for this frame
        Engine::Renderer2D::ResetStats();

        // Render the scene
        m_Renderer->RenderScene(m_Scene.get(), GetWindow().GetWidth(), GetWindow().GetHeight());
    }

    void OnShutdown() override
    {
        std::cout << "Sandbox shutdown!\n";
    }

private:
    std::unique_ptr<Engine::Scene> m_Scene;
    std::unique_ptr<Engine::SceneRenderer> m_Renderer;
    Engine::Entity m_Camera;

    int m_SpriteCount = 0;
    bool m_AnimateSprites = false;
    float m_FPS = 0.0f;
    int m_FrameCount = 0;
    float m_TimeSinceLastFPS = 0.0f;
};

int main(int argc, char** argv)
{
    SandboxApp app;
    app.Run();
    return 0;
}
