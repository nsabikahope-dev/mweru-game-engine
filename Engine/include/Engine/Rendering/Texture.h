#pragma once

#include "Engine/Assets/Asset.h"

#include <string>
#include <cstdint>
#include <memory>

namespace Engine {

/**
 * @brief 2D texture wrapper with asset management
 *
 * Usage with AssetManager:
 *   auto texture = AssetManager::Load<Texture2D>("assets/sprite.png");
 *   texture->Bind(0);
 *
 * Direct usage (for procedural textures):
 *   auto texture = Texture2D::CreateEmpty(256, 256);
 */
class Texture2D : public Asset
{
public:
    ~Texture2D();

    void Bind(uint32_t slot = 0) const;
    void Unbind() const;

    void SetData(void* data, uint32_t size);

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetRendererID() const { return m_RendererID; }

    AssetType GetType() const override { return AssetType::Texture2D; }

    /**
     * @brief Create a texture from file (used by AssetManager)
     */
    static std::shared_ptr<Texture2D> Create(const std::string& path);

    /**
     * @brief Create an empty texture (for procedural generation)
     */
    static std::shared_ptr<Texture2D> CreateEmpty(uint32_t width, uint32_t height);

private:
    Texture2D(const std::string& path);
    Texture2D(uint32_t width, uint32_t height);

    uint32_t m_RendererID;
    uint32_t m_Width, m_Height;
    uint32_t m_InternalFormat, m_DataFormat;
};

} // namespace Engine
