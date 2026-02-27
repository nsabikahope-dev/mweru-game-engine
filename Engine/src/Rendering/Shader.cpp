#include "Engine/Rendering/Shader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

namespace Engine {

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc)
{
    // Compile vertex shader
    uint32_t vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);

    // Compile fragment shader
    uint32_t fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    // Link shaders into program
    m_RendererID = glCreateProgram();
    glAttachShader(m_RendererID, vertexShader);
    glAttachShader(m_RendererID, fragmentShader);
    glLinkProgram(m_RendererID);

    // Check for linking errors
    int success;
    glGetProgramiv(m_RendererID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_RendererID, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << "\n";
    }
    else
    {
        m_Loaded = true;
    }

    // Delete shaders as they're linked into program now
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

std::shared_ptr<Shader> Shader::Create(const std::string& filepath)
{
    // Read shader file
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << filepath << "\n";
        return nullptr;
    }

    std::string line;
    std::stringstream ss[2];
    int type = -1;

    while (std::getline(file, line))
    {
        if (line.find("#type") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
                type = 0;
            else if (line.find("fragment") != std::string::npos)
                type = 1;
        }
        else if (type >= 0)
        {
            ss[type] << line << '\n';
        }
    }

    std::string vertexSrc = ss[0].str();
    std::string fragmentSrc = ss[1].str();

    if (vertexSrc.empty() || fragmentSrc.empty())
    {
        std::cerr << "Shader file missing #type vertex or #type fragment: " << filepath << "\n";
        return nullptr;
    }

    auto shader = std::shared_ptr<Shader>(new Shader(vertexSrc, fragmentSrc));
    shader->m_Path = filepath;

    if (shader->IsLoaded())
    {
        std::cout << "Shader loaded from file: " << filepath << "\n";
        return shader;
    }

    return nullptr;
}

std::shared_ptr<Shader> Shader::CreateFromSource(const std::string& vertexSrc, const std::string& fragmentSrc)
{
    return std::shared_ptr<Shader>(new Shader(vertexSrc, fragmentSrc));
}

Shader::~Shader()
{
    glDeleteProgram(m_RendererID);
}

uint32_t Shader::CompileShader(uint32_t type, const std::string& source)
{
    uint32_t shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        const char* shaderType = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        std::cerr << shaderType << " shader compilation failed:\n" << infoLog << "\n";
    }

    return shader;
}

void Shader::Bind() const
{
    glUseProgram(m_RendererID);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

int Shader::GetUniformLocation(const std::string& name) const
{
    return glGetUniformLocation(m_RendererID, name.c_str());
}

void Shader::SetUniformInt(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetUniformFloat(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetUniformFloat2(const std::string& name, const glm::vec2& value)
{
    glUniform2f(GetUniformLocation(name), value.x, value.y);
}

void Shader::SetUniformFloat3(const std::string& name, const glm::vec3& value)
{
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

void Shader::SetUniformFloat4(const std::string& name, const glm::vec4& value)
{
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::SetUniformMat3(const std::string& name, const glm::mat3& matrix)
{
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetUniformMat4(const std::string& name, const glm::mat4& matrix)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
}

} // namespace Engine
