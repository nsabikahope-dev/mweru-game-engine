#pragma once

#include "Engine/Assets/UUID.h"
#include <string>

namespace Engine {

/**
 * @brief Asset type enumeration
 */
enum class AssetType
{
    None = 0,
    Texture2D,
    Shader,
    Scene
};

/**
 * @brief Base class for all assets
 *
 * Assets are uniquely identifiable resources that can be loaded,
 * cached, and referenced throughout the engine.
 */
class Asset
{
public:
    virtual ~Asset() = default;

    /**
     * @brief Get the asset's unique identifier
     */
    UUID GetUUID() const { return m_UUID; }

    /**
     * @brief Get the asset's file path
     */
    const std::string& GetPath() const { return m_Path; }

    /**
     * @brief Get the asset type
     */
    virtual AssetType GetType() const = 0;

    /**
     * @brief Check if the asset is loaded
     */
    bool IsLoaded() const { return m_Loaded; }

protected:
    UUID m_UUID;
    std::string m_Path;
    bool m_Loaded = false;

    friend class AssetManager;
};

} // namespace Engine
