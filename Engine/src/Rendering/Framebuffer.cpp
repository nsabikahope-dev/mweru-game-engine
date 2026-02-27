#include "Engine/Rendering/Framebuffer.h"

#include <glad/glad.h>
#include <iostream>

namespace Engine {

Framebuffer::Framebuffer(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
    Invalidate();
}

Framebuffer::~Framebuffer()
{
    Cleanup();
}

void Framebuffer::Invalidate()
{
    if (m_FramebufferID)
    {
        Cleanup();
    }

    // Create framebuffer
    glGenFramebuffers(1, &m_FramebufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);

    // Create color attachment texture
    glGenTextures(1, &m_ColorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

    // Create depth attachment renderbuffer
    glGenRenderbuffers(1, &m_DepthAttachment);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthAttachment);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR: Framebuffer is not complete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Cleanup()
{
    if (m_FramebufferID)
    {
        glDeleteFramebuffers(1, &m_FramebufferID);
        m_FramebufferID = 0;
    }

    if (m_ColorAttachment)
    {
        glDeleteTextures(1, &m_ColorAttachment);
        m_ColorAttachment = 0;
    }

    if (m_DepthAttachment)
    {
        glDeleteRenderbuffers(1, &m_DepthAttachment);
        m_DepthAttachment = 0;
    }
}

void Framebuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);
    glViewport(0, 0, m_Width, m_Height);
}

void Framebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 || width > 8192 || height > 8192)
    {
        std::cerr << "WARNING: Invalid framebuffer size: " << width << "x" << height << "\n";
        return;
    }

    m_Width = width;
    m_Height = height;
    Invalidate();
}

} // namespace Engine
