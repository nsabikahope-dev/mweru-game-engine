#pragma once

#include <cstdint>

namespace Engine {

/**
 * @brief Framebuffer for rendering to texture
 *
 * Allows rendering the scene to a texture that can be displayed
 * in ImGui viewports or used for post-processing effects.
 *
 * Usage:
 *   Framebuffer fb(1280, 720);
 *   fb.Bind();
 *   // ... render scene ...
 *   fb.Unbind();
 *   uint32_t textureID = fb.GetColorAttachment();
 *   ImGui::Image((void*)(intptr_t)textureID, ImVec2(width, height));
 */
class Framebuffer
{
public:
    Framebuffer(uint32_t width, uint32_t height);
    ~Framebuffer();

    /**
     * @brief Bind this framebuffer for rendering
     */
    void Bind();

    /**
     * @brief Unbind framebuffer (returns to default framebuffer)
     */
    void Unbind();

    /**
     * @brief Resize the framebuffer
     */
    void Resize(uint32_t width, uint32_t height);

    /**
     * @brief Get the color attachment texture ID
     */
    uint32_t GetColorAttachment() const { return m_ColorAttachment; }

    /**
     * @brief Get framebuffer width
     */
    uint32_t GetWidth() const { return m_Width; }

    /**
     * @brief Get framebuffer height
     */
    uint32_t GetHeight() const { return m_Height; }

private:
    void Invalidate();
    void Cleanup();

private:
    uint32_t m_FramebufferID = 0;
    uint32_t m_ColorAttachment = 0;
    uint32_t m_DepthAttachment = 0;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};

} // namespace Engine
