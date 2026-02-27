#include "Engine/Rendering/Texture.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

namespace Engine {

Texture2D::Texture2D(const std::string& path)
    : m_RendererID(0), m_Width(0), m_Height(0), m_InternalFormat(0), m_DataFormat(0)
{
    m_Path = path;

    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (data)
    {
        m_Width = width;
        m_Height = height;

        // Determine format based on channels
        if (channels == 4)
        {
            m_InternalFormat = GL_RGBA8;
            m_DataFormat = GL_RGBA;
        }
        else if (channels == 3)
        {
            m_InternalFormat = GL_RGB8;
            m_DataFormat = GL_RGB;
        }

        // Create OpenGL texture (OpenGL 3.3 compatible)
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, GL_UNSIGNED_BYTE, data);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);

        m_Loaded = true;
        std::cout << "Texture loaded: " << path << " (" << m_Width << "x" << m_Height << ", " << channels << " channels)\n";
    }
    else
    {
        std::cerr << "Failed to load texture: " << path << "\n";
        std::cerr << "STB Error: " << stbi_failure_reason() << "\n";
    }
}

std::shared_ptr<Texture2D> Texture2D::Create(const std::string& path)
{
    return std::shared_ptr<Texture2D>(new Texture2D(path));
}

std::shared_ptr<Texture2D> Texture2D::CreateEmpty(uint32_t width, uint32_t height)
{
    return std::shared_ptr<Texture2D>(new Texture2D(width, height));
}

Texture2D::Texture2D(uint32_t width, uint32_t height)
    : m_RendererID(0), m_Width(width), m_Height(height)
{
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &m_RendererID);
}

void Texture2D::Bind(uint32_t slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

void Texture2D::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::SetData(void* data, uint32_t size)
{
    uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
    if (size != m_Width * m_Height * bpp)
    {
        std::cerr << "Data size must match entire texture!\n";
        return;
    }

    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Engine
