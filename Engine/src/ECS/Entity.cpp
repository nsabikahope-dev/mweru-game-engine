#include "Engine/ECS/Entity.h"
#include "Engine/Scene/Scene.h"

namespace Engine {

Entity::Entity(entt::entity handle, Scene* scene)
    : m_EntityHandle(handle), m_Scene(scene)
{
}

} // namespace Engine
