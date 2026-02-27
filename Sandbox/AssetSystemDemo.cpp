#include <Engine/Core/Application.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/Shader.h>
#include <iostream>

/**
 * @brief Demonstration of the Asset Management System
 *
 * This demo shows:
 * 1. Creating assets directly vs using AssetManager
 * 2. Automatic caching - loading the same asset twice returns cached version
 * 3. Getting assets by UUID
 * 4. Asset lifetime management
 */

void AssetSystemDemo()
{
    std::cout << "==========================================\n";
    std::cout << "  Phase 4: Asset Management System Demo\n";
    std::cout << "==========================================\n\n";

    // Demo 1: Create textures directly (not cached)
    std::cout << "Demo 1: Direct Texture Creation (No Caching)\n";
    std::cout << "---------------------------------------------\n";
    auto texture1 = Engine::Texture2D::CreateEmpty(256, 256);
    auto texture2 = Engine::Texture2D::CreateEmpty(256, 256);
    std::cout << "Created two 256x256 textures directly\n";
    std::cout << "  Texture1 UUID: " << (uint64_t)texture1->GetUUID() << "\n";
    std::cout << "  Texture2 UUID: " << (uint64_t)texture2->GetUUID() << "\n";
    std::cout << "  (Different UUIDs - not cached)\n";
    std::cout << "  AssetManager asset count: " << Engine::AssetManager::GetAssetCount() << "\n\n";

    // Demo 2: Create shader from source (not cached)
    std::cout << "Demo 2: Shader Creation from Source\n";
    std::cout << "------------------------------------\n";
    std::string vertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 a_Position;
        void main() {
            gl_Position = vec4(a_Position, 1.0);
        }
    )";
    std::string fragmentSrc = R"(
        #version 330 core
        layout(location = 0) out vec4 color;
        void main() {
            color = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";
    auto shader = Engine::Shader::CreateFromSource(vertexSrc, fragmentSrc);
    std::cout << "Created shader from source code\n";
    std::cout << "  Shader UUID: " << (uint64_t)shader->GetUUID() << "\n";
    std::cout << "  Shader loaded: " << (shader->IsLoaded() ? "Yes" : "No") << "\n";
    std::cout << "  AssetManager asset count: " << Engine::AssetManager::GetAssetCount() << "\n\n";

    // Demo 3: AssetManager caching demonstration
    std::cout << "Demo 3: AssetManager Caching Test\n";
    std::cout << "----------------------------------\n";
    std::cout << "Note: File loading will fail (no assets directory yet),\n";
    std::cout << "but this demonstrates the caching logic:\n\n";

    // Try to load a texture (will fail but shows caching logic)
    auto loadedTex1 = Engine::AssetManager::Load<Engine::Texture2D>("assets/test.png");
    if (loadedTex1)
    {
        std::cout << "Loaded texture from file: assets/test.png\n";
        std::cout << "  UUID: " << (uint64_t)loadedTex1->GetUUID() << "\n";

        // Try to load the same texture again - should return cached version
        auto loadedTex2 = Engine::AssetManager::Load<Engine::Texture2D>("assets/test.png");
        if (loadedTex1 == loadedTex2)
        {
            std::cout << "Loading same texture again returned CACHED version!\n";
            std::cout << "  Same pointer: " << (loadedTex1.get() == loadedTex2.get() ? "Yes" : "No") << "\n";
            std::cout << "  Same UUID: " << (loadedTex1->GetUUID() == loadedTex2->GetUUID() ? "Yes" : "No") << "\n";
        }
    }
    else
    {
        std::cout << "Failed to load texture (expected - no assets directory)\n";
    }
    std::cout << "  AssetManager asset count: " << Engine::AssetManager::GetAssetCount() << "\n\n";

    // Demo 4: Asset querying
    std::cout << "Demo 4: Asset Querying by UUID\n";
    std::cout << "-------------------------------\n";
    Engine::UUID uuid = texture1->GetUUID();
    std::cout << "Trying to get texture1 by UUID from AssetManager...\n";
    auto queriedTexture = Engine::AssetManager::Get<Engine::Texture2D>(uuid);
    if (queriedTexture)
    {
        std::cout << "  Found in AssetManager (shouldn't happen - not registered)\n";
    }
    else
    {
        std::cout << "  Not found (expected - direct creation doesn't register with AssetManager)\n";
    }
    std::cout << "\n";

    // Demo 5: Asset statistics
    std::cout << "Demo 5: Asset Statistics\n";
    std::cout << "------------------------\n";
    std::cout << "Total assets in AssetManager: " << Engine::AssetManager::GetAssetCount() << "\n";
    std::cout << "\n";

    // Demo 6: Clear cache
    std::cout << "Demo 6: Clearing Asset Cache\n";
    std::cout << "-----------------------------\n";
    std::cout << "Clearing AssetManager cache...\n";
    Engine::AssetManager::Clear();
    std::cout << "Assets after clear: " << Engine::AssetManager::GetAssetCount() << "\n";
    std::cout << "\n";

    std::cout << "==========================================\n";
    std::cout << "  Asset System Demo Complete!\n";
    std::cout << "==========================================\n\n";

    std::cout << "Key Takeaways:\n";
    std::cout << "1. Each asset has a unique UUID\n";
    std::cout << "2. AssetManager::Load() caches assets and returns cached versions\n";
    std::cout << "3. Direct creation (CreateEmpty, CreateFromSource) bypasses cache\n";
    std::cout << "4. Assets can be queried by UUID\n";
    std::cout << "5. AssetManager provides centralized asset lifetime management\n";
}
