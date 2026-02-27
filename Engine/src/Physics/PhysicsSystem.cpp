#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Scene/Scene.h"
#include "Engine/ECS/Components.h"

#include <box2d/box2d.h>
#include <iostream>

namespace Engine {

static b2BodyType Rigidbody2DTypeToBox2DType(RigidbodyComponent::BodyType bodyType)
{
    switch (bodyType)
    {
        case RigidbodyComponent::BodyType::Static:    return b2_staticBody;
        case RigidbodyComponent::BodyType::Kinematic: return b2_kinematicBody;
        case RigidbodyComponent::BodyType::Dynamic:   return b2_dynamicBody;
    }

    return b2_staticBody;
}

void PhysicsSystem::OnSceneStart(Scene* scene, PhysicsWorld* physicsWorld)
{
    // Create Box2D bodies for all entities with Rigidbody components
    auto view = scene->GetRegistry().view<TransformComponent, RigidbodyComponent>();
    int count = 0;
    for (auto entity : view)
    {
        CreateBody(scene, entity, physicsWorld);
        count++;
    }

    std::cout << "[PhysicsSystem] Created " << count << " physics bodies\n";
}

void PhysicsSystem::OnUpdate(Scene* scene, PhysicsWorld* physicsWorld, float deltaTime)
{
    // Step physics simulation
    physicsWorld->Step(deltaTime);

    // Sync physics bodies to transforms
    SyncPhysicsToTransform(scene);
}

void PhysicsSystem::OnSceneStop(Scene* scene)
{
    // Destroy all Box2D bodies
    auto view = scene->GetRegistry().view<RigidbodyComponent>();
    for (auto entity : view)
    {
        DestroyBody(scene, entity);
    }

    std::cout << "[PhysicsSystem] Destroyed all physics bodies\n";
}

void PhysicsSystem::CreateBody(Scene* scene, entt::entity entity, PhysicsWorld* physicsWorld)
{
    auto& transform = scene->GetRegistry().get<TransformComponent>(entity);
    auto& rb = scene->GetRegistry().get<RigidbodyComponent>(entity);

    b2BodyDef bodyDef;
    bodyDef.type = Rigidbody2DTypeToBox2DType(rb.Type);
    bodyDef.position.Set(transform.Position.x, transform.Position.y);
    bodyDef.angle = transform.Rotation.z;
    bodyDef.fixedRotation = rb.FixedRotation;
    bodyDef.linearDamping = rb.LinearDamping;
    bodyDef.angularDamping = rb.AngularDamping;
    bodyDef.gravityScale = rb.GravityScale;

    b2Body* body = physicsWorld->GetNativeWorld()->CreateBody(&bodyDef);
    rb.RuntimeBody = body;

    // Add box collider if present
    if (scene->GetRegistry().all_of<BoxColliderComponent>(entity))
    {
        auto& bc = scene->GetRegistry().get<BoxColliderComponent>(entity);

        b2PolygonShape boxShape;
        boxShape.SetAsBox(bc.Size.x * transform.Scale.x * 0.5f,
                         bc.Size.y * transform.Scale.y * 0.5f,
                         b2Vec2(bc.Offset.x, bc.Offset.y), 0.0f);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &boxShape;
        fixtureDef.density = bc.Density;
        fixtureDef.friction = bc.Friction;
        fixtureDef.restitution = bc.Restitution;

        b2Fixture* fixture = body->CreateFixture(&fixtureDef);
        bc.RuntimeFixture = fixture;
    }

    // Add circle collider if present
    if (scene->GetRegistry().all_of<CircleColliderComponent>(entity))
    {
        auto& cc = scene->GetRegistry().get<CircleColliderComponent>(entity);

        b2CircleShape circleShape;
        circleShape.m_radius = cc.Radius * std::max(transform.Scale.x, transform.Scale.y);
        circleShape.m_p.Set(cc.Offset.x, cc.Offset.y);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circleShape;
        fixtureDef.density = cc.Density;
        fixtureDef.friction = cc.Friction;
        fixtureDef.restitution = cc.Restitution;

        b2Fixture* fixture = body->CreateFixture(&fixtureDef);
        cc.RuntimeFixture = fixture;
    }
}

void PhysicsSystem::DestroyBody(Scene* scene, entt::entity entity)
{
    auto& rb = scene->GetRegistry().get<RigidbodyComponent>(entity);

    if (rb.RuntimeBody)
    {
        // Box2D automatically destroys all fixtures when destroying the body
        b2World* world = rb.RuntimeBody->GetWorld();
        world->DestroyBody(rb.RuntimeBody);
        rb.RuntimeBody = nullptr;
    }

    // Clear fixture pointers
    if (scene->GetRegistry().all_of<BoxColliderComponent>(entity))
    {
        auto& bc = scene->GetRegistry().get<BoxColliderComponent>(entity);
        bc.RuntimeFixture = nullptr;
    }

    if (scene->GetRegistry().all_of<CircleColliderComponent>(entity))
    {
        auto& cc = scene->GetRegistry().get<CircleColliderComponent>(entity);
        cc.RuntimeFixture = nullptr;
    }
}

void PhysicsSystem::SyncPhysicsToTransform(Scene* scene)
{
    // Update transforms from physics bodies (for dynamic bodies)
    auto view = scene->GetRegistry().view<TransformComponent, RigidbodyComponent>();
    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& rb = view.get<RigidbodyComponent>(entity);

        if (rb.RuntimeBody &&
            (rb.Type == RigidbodyComponent::BodyType::Dynamic ||
             rb.Type == RigidbodyComponent::BodyType::Kinematic))
        {
            const auto& position = rb.RuntimeBody->GetPosition();
            transform.Position.x = position.x;
            transform.Position.y = position.y;
            transform.Rotation.z = rb.RuntimeBody->GetAngle();
        }
    }
}

} // namespace Engine
