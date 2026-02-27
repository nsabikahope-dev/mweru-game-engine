#pragma once

#include <glm/glm.hpp>

/**
 * @brief Simple grid renderer for editor viewport
 *
 * Draws a grid to help with entity placement and visualization
 */
class GridRenderer
{
public:
    static void DrawGrid(const glm::mat4& viewProjectionMatrix, float gridSize = 1.0f, int gridCount = 20);
};
