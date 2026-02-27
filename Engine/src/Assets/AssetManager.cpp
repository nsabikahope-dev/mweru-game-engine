#include "Engine/Assets/AssetManager.h"
#include <iostream>

namespace Engine {

std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::s_AssetRegistry;
std::unordered_map<std::string, UUID> AssetManager::s_PathToUUID;

void AssetManager::Register(std::shared_ptr<Asset> asset)
{
    UUID uuid = asset->GetUUID();
    const std::string& path = asset->GetPath();

    s_AssetRegistry[uuid] = asset;
    s_PathToUUID[path] = uuid;

    std::cout << "[AssetManager] Registered asset: " << path << " (UUID: " << (uint64_t)uuid << ")\n";
}

bool AssetManager::Exists(UUID uuid)
{
    return s_AssetRegistry.find(uuid) != s_AssetRegistry.end();
}

bool AssetManager::Exists(const std::string& path)
{
    return s_PathToUUID.find(path) != s_PathToUUID.end();
}

UUID AssetManager::GetUUIDFromPath(const std::string& path)
{
    auto it = s_PathToUUID.find(path);
    if (it != s_PathToUUID.end())
        return it->second;

    return UUID(0); // Invalid UUID
}

void AssetManager::Unload(UUID uuid)
{
    auto it = s_AssetRegistry.find(uuid);
    if (it != s_AssetRegistry.end())
    {
        // Remove from path map
        const std::string& path = it->second->GetPath();
        s_PathToUUID.erase(path);

        // Remove from registry
        s_AssetRegistry.erase(it);

        std::cout << "[AssetManager] Unloaded asset: " << path << "\n";
    }
}

void AssetManager::Clear()
{
    std::cout << "[AssetManager] Clearing " << s_AssetRegistry.size() << " cached assets\n";
    s_AssetRegistry.clear();
    s_PathToUUID.clear();
}

} // namespace Engine
