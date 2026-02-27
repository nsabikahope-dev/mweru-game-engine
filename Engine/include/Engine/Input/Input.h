#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Engine {

/**
 * @brief Key codes (subset of SDL scancodes for common keys)
 */
enum class KeyCode
{
    // Alphabet
    A = 4, B = 5, C = 6, D = 7, E = 8, F = 9, G = 10, H = 11, I = 12,
    J = 13, K = 14, L = 15, M = 16, N = 17, O = 18, P = 19, Q = 20,
    R = 21, S = 22, T = 23, U = 24, V = 25, W = 26, X = 27, Y = 28, Z = 29,

    // Numbers
    Num1 = 30, Num2 = 31, Num3 = 32, Num4 = 33, Num5 = 34,
    Num6 = 35, Num7 = 36, Num8 = 37, Num9 = 38, Num0 = 39,

    // Special keys
    Space = 44,
    Escape = 41,
    Enter = 40,
    Tab = 43,
    Backspace = 42,
    Delete = 76,

    // Arrow keys
    Right = 79,
    Left = 80,
    Down = 81,
    Up = 82,

    // Modifiers
    LeftShift = 225,
    RightShift = 229,
    LeftControl = 224,
    RightControl = 228,
    LeftAlt = 226,
    RightAlt = 230
};

/**
 * @brief Mouse button codes
 */
enum class MouseButton
{
    Left = 1,
    Middle = 2,
    Right = 3,
    X1 = 4,
    X2 = 5
};

/**
 * @brief Input manager for keyboard and mouse
 *
 * Provides a clean API for querying input state.
 *
 * Usage:
 *   if (Input::IsKeyPressed(KeyCode::Space))
 *       Jump();
 *
 *   if (Input::IsKeyHeld(KeyCode::W))
 *       MoveForward();
 *
 *   glm::vec2 mousePos = Input::GetMousePosition();
 *   bool clicking = Input::IsMouseButtonHeld(MouseButton::Left);
 */
class Input
{
public:
    /**
     * @brief Check if a key was just pressed this frame
     * Returns true only on the frame the key goes from up to down
     */
    static bool IsKeyPressed(KeyCode key);

    /**
     * @brief Check if a key is currently held down
     * Returns true every frame while the key is down
     */
    static bool IsKeyHeld(KeyCode key);

    /**
     * @brief Check if a key was just released this frame
     * Returns true only on the frame the key goes from down to up
     */
    static bool IsKeyReleased(KeyCode key);

    /**
     * @brief Check if a mouse button was just pressed this frame
     */
    static bool IsMouseButtonPressed(MouseButton button);

    /**
     * @brief Check if a mouse button is currently held down
     */
    static bool IsMouseButtonHeld(MouseButton button);

    /**
     * @brief Check if a mouse button was just released this frame
     */
    static bool IsMouseButtonReleased(MouseButton button);

    /**
     * @brief Get the current mouse position in window coordinates
     */
    static glm::vec2 GetMousePosition();

    /**
     * @brief Get the mouse position delta since last frame
     */
    static glm::vec2 GetMouseDelta();

    /**
     * @brief Get the mouse scroll wheel delta
     * Positive = scroll up, Negative = scroll down
     */
    static float GetMouseScroll();

    /**
     * @brief Update input state (called by Application each frame)
     */
    static void Update();

    /**
     * @brief Set key state (called by event system)
     */
    static void SetKeyState(KeyCode key, bool pressed);

    /**
     * @brief Set mouse button state (called by event system)
     */
    static void SetMouseButtonState(MouseButton button, bool pressed);

    /**
     * @brief Set mouse position (called by event system)
     */
    static void SetMousePosition(float x, float y);

    /**
     * @brief Set mouse scroll (called by event system)
     */
    static void SetMouseScroll(float delta);

private:
    static constexpr int MaxKeys = 512;
    static constexpr int MaxMouseButtons = 8;

    static bool s_KeyStates[MaxKeys];
    static bool s_PrevKeyStates[MaxKeys];

    static bool s_MouseButtonStates[MaxMouseButtons];
    static bool s_PrevMouseButtonStates[MaxMouseButtons];

    static glm::vec2 s_MousePosition;
    static glm::vec2 s_PrevMousePosition;
    static float s_MouseScroll;
};

} // namespace Engine
