#include <Engine/Core/Application.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneRenderer.h>
#include <Engine/Physics/PhysicsWorld.h>
#include <Engine/Physics/PhysicsSystem.h>
#include <Engine/ECS/Entity.h>
#include <Engine/ECS/Components.h>
#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer2D.h>

#include <iostream>
#include <memory>

using namespace Engine;

class PhysicsDemoApp : public Application
{
public:
    PhysicsDemoApp()
        : Application("Phase 6: Physics Demo - Box2D", 1920, 1080)
    {
    }

    void OnInit() override
    {
        std::cout << "==========================================\n";
        std::cout << "  Phase 6: Physics System Demo\n";
        std::cout << "==========================================\n";
        std::cout << "Controls:\n";
        std::cout << "  SPACE - Spawn falling box\n";
        std::cout << "  B - Spawn bouncy ball\n";
        std::cout << "  R - Reset scene\n";
        std::cout << "  G - Toggle gravity\n";
        std::cout << "\n";

        // Create scene and renderer
        m_Scene = std::make_unique<Scene>();
        m_Renderer = std::make_unique<SceneRenderer>();
        m_PhysicsWorld = std::make_unique<PhysicsWorld>(glm::vec2(0.0f, -9.8f));

        // Create camera
        m_Camera = m_Scene->CreateEntity("Camera");
        auto& cameraComponent = m_Camera.AddComponent<CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.OrthographicSize = 20.0f;

        CreateInitialScene();

        // Initialize physics
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());

        std::cout << "Physics demo initialized!\n\n";
    }

    void CreateInitialScene()
    {
        // Create ground (static body)
        auto ground = m_Scene->CreateEntity("Ground");
        auto& groundTransform = ground.GetComponent<TransformComponent>();
        groundTransform.Position = glm::vec3(0.0f, -8.0f, 0.0f);
        groundTransform.Scale = glm::vec3(20.0f, 1.0f, 1.0f);

        auto& groundSprite = ground.AddComponent<SpriteRendererComponent>();
        groundSprite.Color = glm::vec4(0.3f, 0.7f, 0.3f, 1.0f);

        auto& groundRb = ground.AddComponent<RigidbodyComponent>();
        groundRb.Type = RigidbodyComponent::BodyType::Static;

        auto& groundCollider = ground.AddComponent<BoxColliderComponent>();
        // groundCollider uses default size of 1x1 which will be scaled by transform

        // Create walls
        CreateWall(glm::vec3(-10.0f, 0.0f, 0.0f), glm::vec3(1.0f, 20.0f, 1.0f));
        CreateWall(glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(1.0f, 20.0f, 1.0f));

        // Create a few initial boxes
        CreateBox(glm::vec3(0.0f, 5.0f, 0.0f));
        CreateBox(glm::vec3(2.0f, 8.0f, 0.0f));
        CreateBox(glm::vec3(-2.0f, 11.0f, 0.0f));
    }

    void CreateWall(const glm::vec3& position, const glm::vec3& scale)
    {
        auto wall = m_Scene->CreateEntity("Wall");
        auto& transform = wall.GetComponent<TransformComponent>();
        transform.Position = position;
        transform.Scale = scale;

        auto& sprite = wall.AddComponent<SpriteRendererComponent>();
        sprite.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

        auto& rb = wall.AddComponent<RigidbodyComponent>();
        rb.Type = RigidbodyComponent::BodyType::Static;

        wall.AddComponent<BoxColliderComponent>();
    }

    void CreateBox(const glm::vec3& position)
    {
        auto box = m_Scene->CreateEntity("Box");
        auto& transform = box.GetComponent<TransformComponent>();
        transform.Position = position;
        transform.Scale = glm::vec3(1.0f, 1.0f, 1.0f);

        auto& sprite = box.AddComponent<SpriteRendererComponent>();
        sprite.Color = glm::vec4(
            0.5f + (rand() % 50) / 100.0f,
            0.5f + (rand() % 50) / 100.0f,
            0.5f + (rand() % 50) / 100.0f,
            1.0f
        );

        auto& rb = box.AddComponent<RigidbodyComponent>();
        rb.Type = RigidbodyComponent::BodyType::Dynamic;
        rb.Mass = 1.0f;

        auto& collider = box.AddComponent<BoxColliderComponent>();
        collider.Density = 1.0f;
        collider.Friction = 0.5f;
        collider.Restitution = 0.2f;  // Slight bounce

        m_DynamicObjects.push_back(box);
    }

    void CreateBall(const glm::vec3& position)
    {
        auto ball = m_Scene->CreateEntity("Ball");
        auto& transform = ball.GetComponent<TransformComponent>();
        transform.Position = position;
        transform.Scale = glm::vec3(1.0f, 1.0f, 1.0f);

        auto& sprite = ball.AddComponent<SpriteRendererComponent>();
        sprite.Color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);

        auto& rb = ball.AddComponent<RigidbodyComponent>();
        rb.Type = RigidbodyComponent::BodyType::Dynamic;
        rb.Mass = 1.0f;

        auto& collider = ball.AddComponent<CircleColliderComponent>();
        collider.Radius = 0.5f;
        collider.Density = 0.8f;
        collider.Friction = 0.2f;
        collider.Restitution = 0.8f;  // Very bouncy!

        m_DynamicObjects.push_back(ball);
    }

    void ResetScene()
    {
        // Clean up physics
        PhysicsSystem::OnSceneStop(m_Scene.get());

        // Recreate scene
        m_Scene = std::make_unique<Scene>();
        m_DynamicObjects.clear();

        // Recreate camera
        m_Camera = m_Scene->CreateEntity("Camera");
        auto& cameraComponent = m_Camera.AddComponent<CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.OrthographicSize = 20.0f;

        CreateInitialScene();

        // Reinitialize physics
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());

        std::cout << "Scene reset!\n";
    }

    void OnUpdate(Timestep ts) override
    {
        // Handle input
        if (Input::IsKeyPressed(KeyCode::Space))
        {
            CreateBox(glm::vec3(
                (rand() % 10) - 5.0f,  // Random X between -5 and 5
                10.0f,
                0.0f
            ));
            // Need to create physics body for new entity
            PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        }

        if (Input::IsKeyPressed(KeyCode::B))
        {
            CreateBall(glm::vec3(
                (rand() % 10) - 5.0f,
                10.0f,
                0.0f
            ));
            PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        }

        if (Input::IsKeyPressed(KeyCode::R))
        {
            ResetScene();
        }

        if (Input::IsKeyPressed(KeyCode::G))
        {
            auto gravity = m_PhysicsWorld->GetGravity();
            if (gravity.y < 0.0f)
            {
                m_PhysicsWorld->SetGravity(glm::vec2(0.0f, 0.0f));
                std::cout << "Gravity OFF\n";
            }
            else
            {
                m_PhysicsWorld->SetGravity(glm::vec2(0.0f, -9.8f));
                std::cout << "Gravity ON\n";
            }
        }

        // Update physics
        PhysicsSystem::OnUpdate(m_Scene.get(), m_PhysicsWorld.get(), ts.GetSeconds());

        // Update scene
        m_Scene->OnUpdate(ts.GetSeconds());
    }

    void OnRender() override
    {
        // Reset stats for this frame
        Renderer2D::ResetStats();

        // Render the scene
        m_Renderer->RenderScene(m_Scene.get(), GetWindow().GetWidth(), GetWindow().GetHeight());
    }

    void OnShutdown() override
    {
        PhysicsSystem::OnSceneStop(m_Scene.get());
        std::cout << "Physics demo shutdown!\n";
    }

private:
    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<SceneRenderer> m_Renderer;
    std::unique_ptr<PhysicsWorld> m_PhysicsWorld;
    Entity m_Camera;
    std::vector<Entity> m_DynamicObjects;
};

int main(int argc, char** argv)
{
    PhysicsDemoApp app;
    app.Run();
    return 0;
}
