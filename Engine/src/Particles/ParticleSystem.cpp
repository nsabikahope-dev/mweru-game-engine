#include "Engine/Particles/ParticleSystem.h"
#include "Engine/ECS/Components.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Rendering/Renderer2D.h"

#include <glm/glm.hpp>
#include <cstdlib>   // rand()
#include <cmath>

namespace Engine {

// Random float in [-1, 1]
static float RandF() { return (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f; }

// -----------------------------------------------------------------------
// Emit one particle from the emitter at world-space position 'origin'
// -----------------------------------------------------------------------
static void EmitParticle(ParticleEmitterComponent& emitter, glm::vec2 origin)
{
    // Advance pool index (ring buffer)
    emitter.PoolIndex = (emitter.PoolIndex + 1) % ParticleEmitterComponent::MaxParticles;
    Particle& p = emitter.Particles[emitter.PoolIndex];

    p.Active    = true;
    p.Position  = origin;

    p.Velocity  = emitter.Velocity +
                  glm::vec2(emitter.VelocityVariation.x * RandF(),
                            emitter.VelocityVariation.y * RandF());

    p.ColorBegin = emitter.ColorBegin;
    p.ColorEnd   = emitter.ColorEnd;

    p.SizeBegin  = emitter.SizeBegin + 0.1f * RandF();
    p.SizeEnd    = emitter.SizeEnd;

    p.LifeTime       = emitter.LifeTime;
    p.LifeRemaining  = emitter.LifeTime;
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------
void ParticleSystem::OnUpdate(Scene* scene, float deltaTime)
{
    auto view = scene->GetRegistry().view<ParticleEmitterComponent, TransformComponent>();

    for (auto handle : view)
    {
        auto& emitter   = view.get<ParticleEmitterComponent>(handle);
        auto& transform = view.get<TransformComponent>(handle);
        glm::vec2 origin = { transform.Position.x, transform.Position.y };

        // Emit new particles
        if (emitter.Emitting)
        {
            emitter.EmitTimer += deltaTime;
            float interval = 1.0f / emitter.EmissionRate;
            while (emitter.EmitTimer >= interval)
            {
                EmitParticle(emitter, origin);
                emitter.EmitTimer -= interval;
            }
        }

        // Simulate existing particles
        for (auto& p : emitter.Particles)
        {
            if (!p.Active)
                continue;

            p.LifeRemaining -= deltaTime;
            if (p.LifeRemaining <= 0.0f)
            {
                p.Active = false;
                continue;
            }

            p.Position += p.Velocity * deltaTime;
        }
    }
}

void ParticleSystem::OnRender(Scene* scene, const glm::mat4& viewProjection)
{
    auto view = scene->GetRegistry().view<ParticleEmitterComponent>();
    if (view.empty())
        return;

    Renderer2D::BeginScene(viewProjection);

    for (auto handle : view)
    {
        auto& emitter = view.get<ParticleEmitterComponent>(handle);

        for (const auto& p : emitter.Particles)
        {
            if (!p.Active)
                continue;

            float life   = p.LifeRemaining / p.LifeTime;          // 1 → 0
            glm::vec4 color = glm::mix(p.ColorEnd, p.ColorBegin, life);
            float size  = glm::mix(p.SizeEnd, p.SizeBegin, life);

            Renderer2D::DrawQuad(
                glm::vec3(p.Position.x, p.Position.y, 0.1f),
                glm::vec2(size, size),
                color
            );
        }
    }

    Renderer2D::EndScene();
}

void ParticleSystem::OnSceneStop(Scene* scene)
{
    auto view = scene->GetRegistry().view<ParticleEmitterComponent>();
    for (auto handle : view)
    {
        auto& emitter = view.get<ParticleEmitterComponent>(handle);
        for (auto& p : emitter.Particles)
            p.Active = false;
        emitter.EmitTimer = 0.0f;
    }
}

} // namespace Engine
