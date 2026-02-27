#include "Engine/Animation/AnimationSystem.h"
#include "Engine/ECS/Components.h"
#include "Engine/ECS/Entity.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Rendering/Texture.h"

#include <iostream>

namespace Engine {

static void LoadFrames(SpriteAnimationComponent& anim)
{
    anim.Frames.clear();

    // Build FramePaths from BasePath + FrameCount if not set explicitly
    if (anim.FramePaths.empty() && !anim.BasePath.empty() && anim.FrameCount > 0)
    {
        for (int i = 0; i < anim.FrameCount; ++i)
        {
            anim.FramePaths.push_back(anim.BasePath + "_" + std::to_string(i) + ".png");
        }
    }

    for (const auto& path : anim.FramePaths)
    {
        auto tex = Texture2D::Create(path);
        if (tex && tex->IsLoaded())
            anim.Frames.push_back(tex);
        else
        {
            std::cerr << "[AnimationSystem] Could not load frame: " << path << "\n";
            anim.Frames.push_back(nullptr); // keep index alignment
        }
    }
}

void AnimationSystem::OnSceneStart(Scene* scene)
{
    auto view = scene->GetRegistry().view<SpriteAnimationComponent>();
    for (auto handle : view)
    {
        auto& anim = scene->GetRegistry().get<SpriteAnimationComponent>(handle);
        LoadFrames(anim);
        anim.CurrentFrame = 0;
        anim.Timer        = 0.0f;

        // Apply first frame immediately
        if (!anim.Frames.empty() && anim.Frames[0])
        {
            if (scene->GetRegistry().all_of<SpriteRendererComponent>(handle))
                scene->GetRegistry().get<SpriteRendererComponent>(handle).Texture = anim.Frames[0];
        }
    }
}

void AnimationSystem::OnSceneStop(Scene* scene)
{
    auto view = scene->GetRegistry().view<SpriteAnimationComponent>();
    for (auto handle : view)
    {
        auto& anim = scene->GetRegistry().get<SpriteAnimationComponent>(handle);
        anim.Frames.clear();
    }
}

void AnimationSystem::OnUpdate(Scene* scene, float deltaTime)
{
    auto view = scene->GetRegistry().view<SpriteAnimationComponent, SpriteRendererComponent>();
    for (auto handle : view)
    {
        auto& anim   = view.get<SpriteAnimationComponent>(handle);
        auto& sprite = view.get<SpriteRendererComponent>(handle);

        if (!anim.Playing || anim.Frames.empty())
            continue;

        anim.Timer += deltaTime;
        if (anim.Timer >= anim.FrameTime)
        {
            anim.Timer -= anim.FrameTime;

            int next = anim.CurrentFrame + 1;
            if (next >= static_cast<int>(anim.Frames.size()))
            {
                next = anim.Loop ? 0 : anim.CurrentFrame; // stop at last frame if not looping
            }
            anim.CurrentFrame = next;

            if (anim.Frames[anim.CurrentFrame])
                sprite.Texture = anim.Frames[anim.CurrentFrame];
        }
    }
}

} // namespace Engine
