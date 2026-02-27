#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace Engine {

class Shader;
class Texture2D;

/**
 * @brief Statistics for renderer performance tracking
 */
struct Renderer2DStats
{
    uint32_t DrawCalls = 0;
    uint32_t QuadCount = 0;
    uint32_t VertexCount = 0;
    uint32_t IndexCount = 0;

    void Reset()
    {
        DrawCalls = 0;
        QuadCount = 0;
        VertexCount = 0;
        IndexCount = 0;
    }
};

/**
 * @brief High-performance 2D batch renderer
 *
 * Renderer2D batches multiple sprite draw calls into a single draw call
 * to minimize GPU overhead and maximize performance.
 *
 * Usage:
 *   Renderer2D::Init();
 *   Renderer2D::BeginScene(camera);
 *   Renderer2D::DrawQuad(position, size, color);
 *   Renderer2D::DrawQuad(position, size, texture);
 *   Renderer2D::EndScene();
 *   auto stats = Renderer2D::GetStats();
 */
class Renderer2D
{
public:
    /**
     * @brief Initialize the renderer
     */
    static void Init();

    /**
     * @brief Shutdown and cleanup
     */
    static void Shutdown();

    /**
     * @brief Begin a new scene with a camera
     * @param viewProjectionMatrix Combined view-projection matrix from camera
     */
    static void BeginScene(const glm::mat4& viewProjectionMatrix);

    /**
     * @brief End the current scene and flush remaining batches
     */
    static void EndScene();

    /**
     * @brief Flush the current batch (force a draw call)
     */
    static void Flush();

    // ===== Draw Commands =====

    /**
     * @brief Draw a colored quad
     */
    static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);

    /**
     * @brief Draw a colored quad with rotation
     */
    static void DrawQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);

    /**
     * @brief Draw a textured quad
     */
    static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

    /**
     * @brief Draw a textured quad with rotation
     */
    static void DrawQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

    /**
     * @brief Draw a quad with a full transform matrix
     */
    static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);

    /**
     * @brief Draw a textured quad with a full transform matrix
     */
    static void DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

    /**
     * @brief Draw a sub-region of a texture atlas (for sprite sheets, font atlases, etc.)
     * @param uvMin Bottom-left UV in [0,1] space
     * @param uvMax Top-right UV in [0,1] space
     */
    static void DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture2D>& texture,
                         const glm::vec2& uvMin, const glm::vec2& uvMax,
                         const glm::vec4& tintColor = glm::vec4(1.0f));

    // ===== Statistics =====

    /**
     * @brief Get rendering statistics
     */
    static Renderer2DStats GetStats();

    /**
     * @brief Reset statistics
     */
    static void ResetStats();
};

} // namespace Engine
