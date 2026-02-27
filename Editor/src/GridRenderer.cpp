#include "../include/GridRenderer.h"
#include <Engine/Rendering/Renderer2D.h>
#include <glm/gtc/matrix_transform.hpp>

void GridRenderer::DrawGrid(const glm::mat4& viewProjectionMatrix, float gridSize, int gridCount)
{
    // Draw grid using colored quads (simple approach)
    // We'll draw thin lines as quads

    float halfExtent = gridSize * gridCount / 2.0f;
    glm::vec4 gridColor = glm::vec4(0.3f, 0.3f, 0.3f, 0.5f);
    glm::vec4 axisColorX = glm::vec4(0.8f, 0.2f, 0.2f, 0.7f); // Red for X axis
    glm::vec4 axisColorY = glm::vec4(0.2f, 0.8f, 0.2f, 0.7f); // Green for Y axis

    float lineThickness = 0.02f;

    // Draw vertical lines (parallel to Y axis)
    for (int i = -gridCount/2; i <= gridCount/2; ++i)
    {
        float x = i * gridSize;
        glm::vec3 position(x, 0.0f, 0.0f);
        glm::vec3 scale(lineThickness, halfExtent * 2.0f, 1.0f);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                             glm::scale(glm::mat4(1.0f), scale);

        // X axis is red, others are gray
        glm::vec4 color = (i == 0) ? axisColorX : gridColor;
        Engine::Renderer2D::DrawQuad(transform, color);
    }

    // Draw horizontal lines (parallel to X axis)
    for (int i = -gridCount/2; i <= gridCount/2; ++i)
    {
        float y = i * gridSize;
        glm::vec3 position(0.0f, y, 0.0f);
        glm::vec3 scale(halfExtent * 2.0f, lineThickness, 1.0f);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                             glm::scale(glm::mat4(1.0f), scale);

        // Y axis is green, others are gray
        glm::vec4 color = (i == 0) ? axisColorY : gridColor;
        Engine::Renderer2D::DrawQuad(transform, color);
    }
}
