#include "Engine/Input/Input.h"

#include <cstring>

namespace Engine {

// Static member initialization
bool Input::s_KeyStates[MaxKeys] = { false };
bool Input::s_PrevKeyStates[MaxKeys] = { false };

bool Input::s_MouseButtonStates[MaxMouseButtons] = { false };
bool Input::s_PrevMouseButtonStates[MaxMouseButtons] = { false };

glm::vec2 Input::s_MousePosition = glm::vec2(0.0f);
glm::vec2 Input::s_PrevMousePosition = glm::vec2(0.0f);
float Input::s_MouseScroll = 0.0f;

bool Input::IsKeyPressed(KeyCode key)
{
    int keyIndex = static_cast<int>(key);
    if (keyIndex < 0 || keyIndex >= MaxKeys)
        return false;

    return s_KeyStates[keyIndex] && !s_PrevKeyStates[keyIndex];
}

bool Input::IsKeyHeld(KeyCode key)
{
    int keyIndex = static_cast<int>(key);
    if (keyIndex < 0 || keyIndex >= MaxKeys)
        return false;

    return s_KeyStates[keyIndex];
}

bool Input::IsKeyReleased(KeyCode key)
{
    int keyIndex = static_cast<int>(key);
    if (keyIndex < 0 || keyIndex >= MaxKeys)
        return false;

    return !s_KeyStates[keyIndex] && s_PrevKeyStates[keyIndex];
}

bool Input::IsMouseButtonPressed(MouseButton button)
{
    int buttonIndex = static_cast<int>(button);
    if (buttonIndex < 0 || buttonIndex >= MaxMouseButtons)
        return false;

    return s_MouseButtonStates[buttonIndex] && !s_PrevMouseButtonStates[buttonIndex];
}

bool Input::IsMouseButtonHeld(MouseButton button)
{
    int buttonIndex = static_cast<int>(button);
    if (buttonIndex < 0 || buttonIndex >= MaxMouseButtons)
        return false;

    return s_MouseButtonStates[buttonIndex];
}

bool Input::IsMouseButtonReleased(MouseButton button)
{
    int buttonIndex = static_cast<int>(button);
    if (buttonIndex < 0 || buttonIndex >= MaxMouseButtons)
        return false;

    return !s_MouseButtonStates[buttonIndex] && s_PrevMouseButtonStates[buttonIndex];
}

glm::vec2 Input::GetMousePosition()
{
    return s_MousePosition;
}

glm::vec2 Input::GetMouseDelta()
{
    return s_MousePosition - s_PrevMousePosition;
}

float Input::GetMouseScroll()
{
    return s_MouseScroll;
}

void Input::Update()
{
    // Copy current states to previous states
    std::memcpy(s_PrevKeyStates, s_KeyStates, sizeof(s_KeyStates));
    std::memcpy(s_PrevMouseButtonStates, s_MouseButtonStates, sizeof(s_MouseButtonStates));

    s_PrevMousePosition = s_MousePosition;
    s_MouseScroll = 0.0f; // Reset scroll each frame
}

void Input::SetKeyState(KeyCode key, bool pressed)
{
    int keyIndex = static_cast<int>(key);
    if (keyIndex >= 0 && keyIndex < MaxKeys)
        s_KeyStates[keyIndex] = pressed;
}

void Input::SetMouseButtonState(MouseButton button, bool pressed)
{
    int buttonIndex = static_cast<int>(button);
    if (buttonIndex >= 0 && buttonIndex < MaxMouseButtons)
        s_MouseButtonStates[buttonIndex] = pressed;
}

void Input::SetMousePosition(float x, float y)
{
    s_MousePosition = glm::vec2(x, y);
}

void Input::SetMouseScroll(float delta)
{
    s_MouseScroll = delta;
}

} // namespace Engine
