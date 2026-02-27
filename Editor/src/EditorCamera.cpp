#include "../include/EditorCamera.h"
#include <Engine/Input/Input.h>
#include <algorithm>
#include <iostream>

using namespace Engine;

EditorCamera::EditorCamera(float width, float height)
    : m_ViewportWidth(width), m_ViewportHeight(height)
{
    m_AspectRatio = width / height;
    RecalculateProjectionMatrix();
    RecalculateViewMatrix();
}

void EditorCamera::OnUpdate(float deltaTime)
{
    // Only process input if viewport is hovered and focused
    if (!m_ViewportHovered || !m_ViewportFocused)
        return;

    // Handle zoom (scroll wheel)
    float scroll = Input::GetMouseScroll();
    if (scroll != 0.0f)
    {
        m_ZoomLevel -= scroll * m_ZoomSpeed * m_ZoomLevel; // Zoom speed scales with zoom level
        m_ZoomLevel = std::clamp(m_ZoomLevel, 0.5f, 50.0f);
        RecalculateProjectionMatrix();
        RecalculateViewMatrix();
    }

    // Handle pan (right mouse button or middle mouse button)
    if (Input::IsMouseButtonHeld(MouseButton::Right) || Input::IsMouseButtonHeld(MouseButton::Middle))
    {
        glm::vec2 mouseDelta = Input::GetMouseDelta();

        // Scale pan speed based on zoom level
        float panSpeedX = m_PanSpeed * m_ZoomLevel / m_ViewportWidth * 2.0f;
        float panSpeedY = m_PanSpeed * m_ZoomLevel / m_ViewportHeight * 2.0f;

        m_Position.x -= mouseDelta.x * panSpeedX;
        m_Position.y += mouseDelta.y * panSpeedY; // Y is inverted

        RecalculateViewMatrix();
    }
}

void EditorCamera::OnResize(float width, float height)
{
    if (width > 0 && height > 0)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        m_AspectRatio = width / height;
        RecalculateProjectionMatrix();
        RecalculateViewMatrix();
    }
}

glm::mat4 EditorCamera::GetViewProjectionMatrix() const
{
    return m_ViewProjectionMatrix;
}

void EditorCamera::SetZoomLevel(float zoomLevel)
{
    m_ZoomLevel = std::clamp(zoomLevel, 0.5f, 50.0f);
    RecalculateProjectionMatrix();
    RecalculateViewMatrix();
}

void EditorCamera::FocusOn(const glm::vec3& position)
{
    m_Position = position;
    RecalculateViewMatrix();
}

void EditorCamera::RecalculateViewMatrix()
{
    // Simple orthographic camera (looking down -Z axis)
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position);

    m_ViewMatrix = glm::inverse(transform);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void EditorCamera::RecalculateProjectionMatrix()
{
    // Orthographic projection
    float orthoLeft = -m_ZoomLevel * m_AspectRatio * 0.5f;
    float orthoRight = m_ZoomLevel * m_AspectRatio * 0.5f;
    float orthoBottom = -m_ZoomLevel * 0.5f;
    float orthoTop = m_ZoomLevel * 0.5f;

    m_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1.0f, 1.0f);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}
