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

#include <iostream>
#include <memory>

using namespace Engine;

class SerializationDemoApp : public Application
{
public:
    SerializationDemoApp()
        : Application("Phase 8: Scene Serialization Demo", 1920, 1080)
    {
    }

    void OnInit() override
    {
        std::cout << "==========================================\n";
        std::cout << "  Phase 8: Scene Serialization Demo\n";
        std::cout << "==========================================\n";
        std::cout << "Controls:\n";
        std::cout << "  S - Save current scene to 'saved_scene.scene'\n";
        std::cout << "  L - Load scene from 'saved_scene.scene'\n";
        std::cout << "  N - Create new random scene\n";
        std::cout << "  C - Clear scene\n";
        std::cout << "  SPACE - Spawn box\n";
        std::cout << "\n";

        // Create scene and renderer
        m_Scene = std::make_unique<Scene>();
        m_Renderer = std::make_unique<SceneRenderer>();
        m_PhysicsWorld = std::make_unique<PhysicsWorld>(glm::vec2(0.0f, -9.8f));

        CreateDefaultScene();

        std::cout << "Demo initialized! Create some objects, then press 'S' to save.\n";
        std::cout << "Press 'C' to clear the scene, then 'L' to load it back!\n\n";
    }

    void CreateDefaultScene()
    {
        // Create camera
        m_Camera = m_Scene->CreateEntity("Main Camera");
        auto& cameraComponent = m_Camera.AddComponent<CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.OrthographicSize = 20.0f;

        // Create ground
        auto ground = m_Scene->CreateEntity("Ground");
        auto& groundTransform = ground.GetComponent<TransformComponent>();
        groundTransform.Position = glm::vec3(0.0f, -8.0f, 0.0f);
        groundTransform.Scale = glm::vec3(20.0f, 1.0f, 1.0f);

        auto& groundSprite = ground.AddComponent<SpriteRendererComponent>();
        groundSprite.Color = glm::vec4(0.3f, 0.7f, 0.3f, 1.0f);

        auto& groundRb = ground.AddComponent<RigidbodyComponent>();
        groundRb.Type = RigidbodyComponent::BodyType::Static;

        auto& groundCollider = ground.AddComponent<BoxColliderComponent>();
        groundCollider.Friction = 0.5f;

        // Create walls
        CreateWall("Left Wall", glm::vec3(-10.0f, 0.0f, 0.0f), glm::vec3(1.0f, 20.0f, 1.0f));
        CreateWall("Right Wall", glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(1.0f, 20.0f, 1.0f));

        // Create some initial boxes
        CreateBox("Box 1", glm::vec3(0.0f, 5.0f, 0.0f), glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));
        CreateBox("Box 2", glm::vec3(2.0f, 8.0f, 0.0f), glm::vec4(0.3f, 1.0f, 0.3f, 1.0f));
        CreateBox("Box 3", glm::vec3(-2.0f, 11.0f, 0.0f), glm::vec4(0.3f, 0.3f, 1.0f, 1.0f));

        // Initialize physics
        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
    }

    void CreateWall(const std::string& name, const glm::vec3& position, const glm::vec3& scale)
    {
        auto wall = m_Scene->CreateEntity(name);
        auto& transform = wall.GetComponent<TransformComponent>();
        transform.Position = position;
        transform.Scale = scale;

        auto& sprite = wall.AddComponent<SpriteRendererComponent>();
        sprite.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

        auto& rb = wall.AddComponent<RigidbodyComponent>();
        rb.Type = RigidbodyComponent::BodyType::Static;

        wall.AddComponent<BoxColliderComponent>();
    }

    void CreateBox(const std::string& name, const glm::vec3& position, const glm::vec4& color)
    {
        auto box = m_Scene->CreateEntity(name);
        auto& transform = box.GetComponent<TransformComponent>();
        transform.Position = position;
        transform.Scale = glm::vec3(1.0f, 1.0f, 1.0f);

        auto& sprite = box.AddComponent<SpriteRendererComponent>();
        sprite.Color = color;

        auto& rb = box.AddComponent<RigidbodyComponent>();
        rb.Type = RigidbodyComponent::BodyType::Dynamic;
        rb.Mass = 1.0f;

        auto& collider = box.AddComponent<BoxColliderComponent>();
        collider.Density = 1.0f;
        collider.Friction = 0.5f;
        collider.Restitution = 0.2f;
    }

    void SaveScene()
    {
        std::cout << "\n[SAVING SCENE]\n";
        std::cout << "Saving scene to: saved_scene.scene\n";

        if (SceneSerializer::Serialize(m_Scene.get(), "saved_scene.scene"))
        {
            std::cout << "SUCCESS: Scene saved!\n";
            std::cout << "You can now clear the scene (C) and reload it (L)\n\n";
        }
        else
        {
            std::cout << "ERROR: Failed to save scene\n\n";
        }
    }

    void LoadScene()
    {
        std::cout << "\n[LOADING SCENE]\n";
        std::cout << "Loading scene from: saved_scene.scene\n";

        // Cleanup current scene
        PhysicsSystem::OnSceneStop(m_Scene.get());
        m_Scene = std::make_unique<Scene>();

        if (SceneSerializer::Deserialize(m_Scene.get(), "saved_scene.scene"))
        {
            std::cout << "SUCCESS: Scene loaded!\n";
            std::cout << "Reinitializing physics...\n";

            // Find camera
            auto view = m_Scene->GetRegistry().view<CameraComponent>();
            if (!view.empty())
            {
                m_Camera = Entity(*view.begin(), m_Scene.get());
            }

            // Reinitialize physics
            PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
            std::cout << "Scene fully restored!\n\n";
        }
        else
        {
            std::cout << "ERROR: Failed to load scene\n";
            std::cout << "Creating default scene instead...\n\n";
            CreateDefaultScene();
        }
    }

    void ClearScene()
    {
        std::cout << "\n[CLEARING SCENE]\n";
        PhysicsSystem::OnSceneStop(m_Scene.get());
        m_Scene = std::make_unique<Scene>();

        // Recreate only camera
        m_Camera = m_Scene->CreateEntity("Main Camera");
        auto& cameraComponent = m_Camera.AddComponent<CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.OrthographicSize = 20.0f;

        std::cout << "Scene cleared! Press 'L' to load saved scene or 'N' for new random scene\n\n";
    }

    void CreateRandomScene()
    {
        std::cout << "\n[CREATING RANDOM SCENE]\n";
        PhysicsSystem::OnSceneStop(m_Scene.get());
        m_Scene = std::make_unique<Scene>();

        // Camera
        m_Camera = m_Scene->CreateEntity("Main Camera");
        auto& cameraComponent = m_Camera.AddComponent<CameraComponent>();
        cameraComponent.Primary = true;
        cameraComponent.OrthographicSize = 20.0f;

        // Random boxes
        for (int i = 0; i < 10; i++)
        {
            float x = (rand() % 16) - 8.0f;
            float y = (rand() % 10) + 2.0f;
            float r = (rand() % 100) / 100.0f;
            float g = (rand() % 100) / 100.0f;
            float b = (rand() % 100) / 100.0f;

            CreateBox("RandomBox" + std::to_string(i),
                     glm::vec3(x, y, 0.0f),
                     glm::vec4(r, g, b, 1.0f));
        }

        // Ground
        CreateWall("Ground", glm::vec3(0.0f, -8.0f, 0.0f), glm::vec3(20.0f, 1.0f, 1.0f));

        PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
        std::cout << "Random scene created!\n\n";
    }

    void OnUpdate(Timestep ts) override
    {
        // Handle input
        if (Input::IsKeyPressed(KeyCode::S))
        {
            SaveScene();
        }

        if (Input::IsKeyPressed(KeyCode::L))
        {
            LoadScene();
        }

        if (Input::IsKeyPressed(KeyCode::N))
        {
            CreateRandomScene();
        }

        if (Input::IsKeyPressed(KeyCode::C))
        {
            ClearScene();
        }

        if (Input::IsKeyPressed(KeyCode::Space))
        {
            CreateBox("SpawnedBox",
                     glm::vec3((rand() % 10) - 5.0f, 10.0f, 0.0f),
                     glm::vec4(
                         (rand() % 100) / 100.0f,
                         (rand() % 100) / 100.0f,
                         (rand() % 100) / 100.0f,
                         1.0f
                     ));
            PhysicsSystem::OnSceneStart(m_Scene.get(), m_PhysicsWorld.get());
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
        std::cout << "Serialization demo shutdown!\n";
    }

private:
    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<SceneRenderer> m_Renderer;
    std::unique_ptr<PhysicsWorld> m_PhysicsWorld;
    Entity m_Camera;
};

int main(int argc, char** argv)
{
    SerializationDemoApp app;
    app.Run();
    return 0;
}
