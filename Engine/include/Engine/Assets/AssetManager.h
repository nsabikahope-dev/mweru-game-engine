#pragma once

#include "Engine/Assets/Asset.h"
#include "Engine/Assets/UUID.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

namespace Engine {

/**
 * @brief Manages loading, caching, and lifetime of assets
 *
 * AssetManager provides a centralized system for loading and caching assets.
 * It ensures that each asset is only loaded once and provides reference counting.
 *
 * Usage:
 *   auto texture = AssetManager::Load<Texture2D>("assets/player.png");
 *   auto shader = AssetManager::Get<Shader>(uuid);
 */
class AssetManager
{
public:
    /**
     * @brief Load an asset from file
     * @tparam T Asset type (must derive from Asset)
     * @param path File path to load from
     * @return Shared pointer to the loaded asset
     */
    template<typename T>
    static std::shared_ptr<T> Load(const std::string& path)
    {
        static_assert(std::is_base_of<Asset, T>::value, "T must derive from Asset");

        // Check if already loaded
        auto it = s_PathToUUID.find(path);
        if (it != s_PathToUUID.end())
        {
            // Asset already loaded, return cached version
            return std::static_pointer_cast<T>(s_AssetRegistry[it->second]);
        }

        // Load new asset
        std::shared_ptr<T> asset = T::Create(path);
        if (asset && asset->IsLoaded())
        {
            Register(asset);
            return asset;
        }

        return nullptr;
    }

    /**
     * @brief Get an asset by UUID
     * @tparam T Asset type (must derive from Asset)
     * @param uuid Asset's unique identifier
     * @return Shared pointer to the asset, or nullptr if not found
     */
    template<typename T>
    static std::shared_ptr<T> Get(UUID uuid)
    {
        static_assert(std::is_base_of<Asset, T>::value, "T must derive from Asset");

        auto it = s_AssetRegistry.find(uuid);
        if (it != s_AssetRegistry.end())
        {
            return std::static_pointer_cast<T>(it->second);
        }

        return nullptr;
    }

    /**
     * @brief Check if an asset exists in the registry
     */
    static bool Exists(UUID uuid);

    /**
     * @brief Check if an asset exists at a path
     */
    static bool Exists(const std::string& path);

    /**
     * @brief Get UUID from file path
     */
    static UUID GetUUIDFromPath(const std::string& path);

    /**
     * @brief Unload an asset
     */
    static void Unload(UUID uuid);

    /**
     * @brief Clear all cached assets
     */
    static void Clear();

    /**
     * @brief Get asset count
     */
    static size_t GetAssetCount() { return s_AssetRegistry.size(); }

private:
    /**
     * @brief Register an asset with the manager
     */
    static void Register(std::shared_ptr<Asset> asset);

    static std::unordered_map<UUID, std::shared_ptr<Asset>> s_AssetRegistry;
    static std::unordered_map<std::string, UUID> s_PathToUUID;
};

} // namespace Engine
