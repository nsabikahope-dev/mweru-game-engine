#pragma once

#include "Engine/Assets/Asset.h"

#include <string>
#include <glm/glm.hpp>
#include <memory>

namespace Engine {

/**
 * @brief OpenGL shader program wrapper with asset management
 *
 * Usage with AssetManager:
 *   auto shader = AssetManager::Load<Shader>("assets/shaders/basic.glsl");
 *   shader->Bind();
 *   shader->SetUniformMat4("u_ViewProjection", camera.GetViewProjection());
 *
 * Direct usage (for inline shaders):
 *   auto shader = Shader::CreateFromSource(vertexSrc, fragmentSrc);
 */
class Shader : public Asset
{
public:
    ~Shader();

    void Bind() const;
    void Unbind() const;

    // Uniform setters
    void SetUniformInt(const std::string& name, int value);
    void SetUniformFloat(const std::string& name, float value);
    void SetUniformFloat2(const std::string& name, const glm::vec2& value);
    void SetUniformFloat3(const std::string& name, const glm::vec3& value);
    void SetUniformFloat4(const std::string& name, const glm::vec4& value);
    void SetUniformMat3(const std::string& name, const glm::mat3& matrix);
    void SetUniformMat4(const std::string& name, const glm::mat4& matrix);

    uint32_t GetRendererID() const { return m_RendererID; }
    AssetType GetType() const override { return AssetType::Shader; }

    /**
     * @brief Create shader from file (used by AssetManager)
     * Expected format: Single .glsl file with #type vertex / #type fragment directives
     */
    static std::shared_ptr<Shader> Create(const std::string& filepath);

    /**
     * @brief Create shader directly from source code
     */
    static std::shared_ptr<Shader> CreateFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);

private:
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);

    uint32_t CompileShader(uint32_t type, const std::string& source);
    int GetUniformLocation(const std::string& name) const;

private:
    uint32_t m_RendererID;
};

} // namespace Engine
