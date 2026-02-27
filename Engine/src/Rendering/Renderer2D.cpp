#include "Engine/Rendering/Renderer2D.h"
#include "Engine/Rendering/Shader.h"
#include "Engine/Rendering/Texture.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace Engine {

struct QuadVertex
{
    glm::vec3 Position;
    glm::vec4 Color;
    glm::vec2 TexCoord;
    float TexIndex;
};

struct Renderer2DData
{
    static const uint32_t MaxQuads = 10000;
    static const uint32_t MaxVertices = MaxQuads * 4;
    static const uint32_t MaxIndices = MaxQuads * 6;
    static const uint32_t MaxTextureSlots = 8; // 8 texture slots for OpenGL 3.3 compatibility

    uint32_t QuadVAO = 0;
    uint32_t QuadVBO = 0;
    uint32_t QuadEBO = 0;

    uint32_t QuadIndexCount = 0;
    QuadVertex* QuadVertexBufferBase = nullptr;
    QuadVertex* QuadVertexBufferPtr = nullptr;

    std::array<std::shared_ptr<Texture2D>, MaxTextureSlots> TextureSlots;
    uint32_t TextureSlotIndex = 1; // 0 = white texture

    std::shared_ptr<Texture2D> WhiteTexture;
    std::shared_ptr<Shader> TextureShader;

    glm::vec4 QuadVertexPositions[4];

    Renderer2DStats Stats;
};

static Renderer2DData s_Data;

void Renderer2D::Init()
{
    // Create vertex array
    glGenVertexArrays(1, &s_Data.QuadVAO);
    glBindVertexArray(s_Data.QuadVAO);

    // Create vertex buffer
    glGenBuffers(1, &s_Data.QuadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, Renderer2DData::MaxVertices * sizeof(QuadVertex), nullptr, GL_DYNAMIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void*)offsetof(QuadVertex, Position));

    // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void*)offsetof(QuadVertex, Color));

    // TexCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void*)offsetof(QuadVertex, TexCoord));

    // TexIndex
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void*)offsetof(QuadVertex, TexIndex));

    // Create index buffer
    uint32_t* quadIndices = new uint32_t[Renderer2DData::MaxIndices];
    uint32_t offset = 0;
    for (uint32_t i = 0; i < Renderer2DData::MaxIndices; i += 6)
    {
        quadIndices[i + 0] = offset + 0;
        quadIndices[i + 1] = offset + 1;
        quadIndices[i + 2] = offset + 2;

        quadIndices[i + 3] = offset + 2;
        quadIndices[i + 4] = offset + 3;
        quadIndices[i + 5] = offset + 0;

        offset += 4;
    }

    glGenBuffers(1, &s_Data.QuadEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data.QuadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Renderer2DData::MaxIndices * sizeof(uint32_t), quadIndices, GL_STATIC_DRAW);

    delete[] quadIndices;

    // Create white texture
    s_Data.WhiteTexture = Texture2D::CreateEmpty(1, 1);
    uint32_t whiteTextureData = 0xffffffff;
    s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

    s_Data.TextureSlots[0] = s_Data.WhiteTexture;

    // Create shader
    std::string vertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec4 a_Color;
        layout(location = 2) in vec2 a_TexCoord;
        layout(location = 3) in float a_TexIndex;

        uniform mat4 u_ViewProjection;

        out vec4 v_Color;
        out vec2 v_TexCoord;
        out float v_TexIndex;

        void main()
        {
            v_Color = a_Color;
            v_TexCoord = a_TexCoord;
            v_TexIndex = a_TexIndex;
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
        }
    )";

    std::string fragmentSrc = R"(
        #version 330 core
        layout(location = 0) out vec4 color;

        in vec4 v_Color;
        in vec2 v_TexCoord;
        in float v_TexIndex;

        uniform sampler2D u_Textures[8];

        void main()
        {
            vec4 texColor = v_Color;
            int index = int(v_TexIndex);

            // Manual texture sampling for OpenGL 3.3 compatibility
            switch(index)
            {
                case 0:  texColor *= texture(u_Textures[0],  v_TexCoord); break;
                case 1:  texColor *= texture(u_Textures[1],  v_TexCoord); break;
                case 2:  texColor *= texture(u_Textures[2],  v_TexCoord); break;
                case 3:  texColor *= texture(u_Textures[3],  v_TexCoord); break;
                case 4:  texColor *= texture(u_Textures[4],  v_TexCoord); break;
                case 5:  texColor *= texture(u_Textures[5],  v_TexCoord); break;
                case 6:  texColor *= texture(u_Textures[6],  v_TexCoord); break;
                case 7:  texColor *= texture(u_Textures[7],  v_TexCoord); break;
            }

            color = texColor;
        }
    )";

    s_Data.TextureShader = Shader::CreateFromSource(vertexSrc, fragmentSrc);

    // Set texture slots
    int samplers[8];
    for (int i = 0; i < 8; i++)
        samplers[i] = i;

    s_Data.TextureShader->Bind();
    glUniform1iv(glGetUniformLocation(s_Data.TextureShader->GetRendererID(), "u_Textures"), 8, samplers);

    // Allocate vertex buffer
    s_Data.QuadVertexBufferBase = new QuadVertex[Renderer2DData::MaxVertices];

    // Set quad vertex positions
    s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
    s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
    s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
    s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

    // Enable alpha blending for sprites, text, and transparent surfaces
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer2D::Shutdown()
{
    glDeleteVertexArrays(1, &s_Data.QuadVAO);
    glDeleteBuffers(1, &s_Data.QuadVBO);
    glDeleteBuffers(1, &s_Data.QuadEBO);

    delete[] s_Data.QuadVertexBufferBase;
}

void Renderer2D::BeginScene(const glm::mat4& viewProjectionMatrix)
{
    s_Data.TextureShader->Bind();
    s_Data.TextureShader->SetUniformMat4("u_ViewProjection", viewProjectionMatrix);

    s_Data.QuadIndexCount = 0;
    s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

    s_Data.TextureSlotIndex = 1;
}

void Renderer2D::EndScene()
{
    Flush();
}

void Renderer2D::Flush()
{
    if (s_Data.QuadIndexCount == 0)
        return; // Nothing to draw

    uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.QuadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, s_Data.QuadVertexBufferBase);

    // Bind textures
    for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
        s_Data.TextureSlots[i]->Bind(i);

    glBindVertexArray(s_Data.QuadVAO);
    glDrawElements(GL_TRIANGLES, s_Data.QuadIndexCount, GL_UNSIGNED_INT, nullptr);
    s_Data.Stats.DrawCalls++;

    s_Data.QuadIndexCount = 0;
    s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

    s_Data.TextureSlotIndex = 1;
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
        * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

    DrawQuad(transform, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
        * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
        * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

    DrawQuad(transform, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture, const glm::vec4& tintColor)
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
        * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

    DrawQuad(transform, texture, tintColor);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const std::shared_ptr<Texture2D>& texture, const glm::vec4& tintColor)
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
        * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
        * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

    DrawQuad(transform, texture, tintColor);
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
{
    constexpr size_t quadVertexCount = 4;
    const float textureIndex = 0.0f; // White Texture
    constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
        Flush();

    for (size_t i = 0; i < quadVertexCount; i++)
    {
        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture2D>& texture, const glm::vec4& tintColor)
{
    constexpr size_t quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
        Flush();

    float textureIndex = 0.0f;
    for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
    {
        if (s_Data.TextureSlots[i].get() == texture.get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    if (textureIndex == 0.0f)
    {
        if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
            Flush();

        textureIndex = (float)s_Data.TextureSlotIndex;
        s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
        s_Data.TextureSlotIndex++;
    }

    for (size_t i = 0; i < quadVertexCount; i++)
    {
        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color = tintColor;
        s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const std::shared_ptr<Texture2D>& texture,
                          const glm::vec2& uvMin, const glm::vec2& uvMax,
                          const glm::vec4& tintColor)
{
    constexpr size_t quadVertexCount = 4;
    // UV layout matches quad vertex order: BL, BR, TR, TL
    const glm::vec2 textureCoords[] = {
        { uvMin.x, uvMin.y },
        { uvMax.x, uvMin.y },
        { uvMax.x, uvMax.y },
        { uvMin.x, uvMax.y }
    };

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
        Flush();

    float textureIndex = 0.0f;
    for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
    {
        if (s_Data.TextureSlots[i].get() == texture.get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    if (textureIndex == 0.0f)
    {
        if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
            Flush();

        textureIndex = (float)s_Data.TextureSlotIndex;
        s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
        s_Data.TextureSlotIndex++;
    }

    for (size_t i = 0; i < quadVertexCount; i++)
    {
        s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color = tintColor;
        s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
        s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;
    s_Data.Stats.QuadCount++;
}

Renderer2DStats Renderer2D::GetStats()
{
    return s_Data.Stats;
}

void Renderer2D::ResetStats()
{
    s_Data.Stats.Reset();
}

} // namespace Engine
