#include "Engine/Physics/PhysicsWorld.h"

#include <box2d/box2d.h>
#include <iostream>

namespace Engine {

PhysicsWorld::PhysicsWorld(const glm::vec2& gravity)
{
    b2Vec2 b2Gravity(gravity.x, gravity.y);
    m_World = std::make_unique<b2World>(b2Gravity);

    std::cout << "[PhysicsWorld] Created with gravity: (" << gravity.x << ", " << gravity.y << ")\n";
}

PhysicsWorld::~PhysicsWorld()
{
    std::cout << "[PhysicsWorld] Destroyed\n";
}

void PhysicsWorld::Step(float deltaTime)
{
    // Use fixed timestep accumulator for stable physics
    m_Accumulator += deltaTime;

    while (m_Accumulator >= s_FixedTimestep)
    {
        m_World->Step(s_FixedTimestep, s_VelocityIterations, s_PositionIterations);
        m_Accumulator -= s_FixedTimestep;
    }
}

void PhysicsWorld::SetGravity(const glm::vec2& gravity)
{
    m_World->SetGravity(b2Vec2(gravity.x, gravity.y));
}

glm::vec2 PhysicsWorld::GetGravity() const
{
    b2Vec2 gravity = m_World->GetGravity();
    return glm::vec2(gravity.x, gravity.y);
}

} // namespace Engine
