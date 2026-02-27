#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Editor camera with pan and zoom controls
 *
 * Provides an independent camera for the editor viewport that doesn't
 * affect the game cameras. Supports mouse pan (right-click drag) and
 * zoom (scroll wheel).
 *
 * Usage:
 *   EditorCamera camera(1280, 720);
 *   camera.OnUpdate(deltaTime);
 *   glm::mat4 viewProj = camera.GetViewProjectionMatrix();
 */
class EditorCamera
{
public:
    EditorCamera(float width, float height);

    /**
     * @brief Update camera (call every frame)
     */
    void OnUpdate(float deltaTime);

    /**
     * @brief Handle viewport resize
     */
    void OnResize(float width, float height);

    /**
     * @brief Get the view-projection matrix
     */
    glm::mat4 GetViewProjectionMatrix() const;

    /**
     * @brief Get camera position
     */
    glm::vec3 GetPosition() const { return m_Position; }

    /**
     * @brief Set camera position
     */
    void SetPosition(const glm::vec3& position) { m_Position = position; }

    /**
     * @brief Get zoom level (orthographic size)
     */
    float GetZoomLevel() const { return m_ZoomLevel; }

    /**
     * @brief Set zoom level
     */
    void SetZoomLevel(float zoomLevel);

    /**
     * @brief Focus camera on a specific position
     */
    void FocusOn(const glm::vec3& position);

    /**
     * @brief Check if viewport is hovered
     */
    void SetViewportHovered(bool hovered) { m_ViewportHovered = hovered; }

    /**
     * @brief Check if viewport is focused
     */
    void SetViewportFocused(bool focused) { m_ViewportFocused = focused; }

private:
    void RecalculateViewMatrix();
    void RecalculateProjectionMatrix();

    glm::vec2 GetMouseDelta() const;
    float GetMouseScroll() const;
    bool IsMouseButtonHeld(int button) const;

private:
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewProjectionMatrix;

    glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
    float m_ZoomLevel = 10.0f;

    float m_AspectRatio = 1.778f; // 16:9
    float m_ViewportWidth = 1280.0f;
    float m_ViewportHeight = 720.0f;

    bool m_ViewportHovered = false;
    bool m_ViewportFocused = false;

    // Pan control
    glm::vec2 m_LastMousePosition = glm::vec2(0.0f);
    float m_PanSpeed = 1.0f;
    float m_ZoomSpeed = 0.2f;
};
