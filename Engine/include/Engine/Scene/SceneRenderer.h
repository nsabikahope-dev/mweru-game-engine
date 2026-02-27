#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace Engine {

class Scene;
class Shader;

/**
 * @brief Renders a scene with all its entities using Renderer2D
 *
 * SceneRenderer iterates through all entities with renderable components
 * and submits them to the batch renderer for efficient rendering.
 */
class SceneRenderer
{
public:
    SceneRenderer();
    ~SceneRenderer();

    /**
     * @brief Render a scene
     * @param scene The scene to render
     * @param viewportWidth Width of the viewport
     * @param viewportHeight Height of the viewport
     */
    void RenderScene(Scene* scene, uint32_t viewportWidth, uint32_t viewportHeight);

    /**
     * @brief Render a scene with a custom view-projection matrix
     * @param scene The scene to render
     * @param viewProjection Custom view-projection matrix (e.g., from editor camera)
     */
    void RenderScene(Scene* scene, const glm::mat4& viewProjection);
};

} // namespace Engine
