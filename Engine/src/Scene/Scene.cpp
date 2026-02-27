#include "Engine/Scene/Scene.h"
#include "Engine/ECS/Entity.h"
#include "Engine/ECS/Components.h"
#include "Engine/Rendering/Shader.h"
#include "Engine/Rendering/Texture.h"

#include <glad/glad.h>
#include <iostream>

namespace Engine {

Scene::Scene()
{
}

Scene::~Scene()
{
}

Entity Scene::CreateEntity(const std::string& name)
{
    Entity entity = { m_Registry.create(), this };
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<TransformComponent>();
    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    m_Registry.destroy(entity);
}

void Scene::OnUpdate(float deltaTime)
{
    // Update logic for entities can go here
    // For now, we'll keep this simple
}

void Scene::OnRender()
{
    // Rendering will be handled by systems
    // We'll implement this in the next step
}

} // namespace Engine
