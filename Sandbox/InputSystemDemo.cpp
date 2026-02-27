#include <Engine/Input/Input.h>
#include <iostream>

using namespace Engine;

/**
 * @brief Demonstration of the Input System
 *
 * This demo shows:
 * 1. Key press/hold/release detection
 * 2. Mouse button states
 * 3. Mouse position and delta tracking
 * 4. Mouse scroll wheel detection
 */

void InputSystemDemo()
{
    std::cout << "==========================================\n";
    std::cout << "  Phase 5: Input System Demo\n";
    std::cout << "==========================================\n\n";

    std::cout << "Input System Features:\n";
    std::cout << "----------------------\n\n";

    std::cout << "1. Key States:\n";
    std::cout << "   - IsKeyPressed(key): True only on first frame when key goes down\n";
    std::cout << "   - IsKeyHeld(key): True every frame while key is down\n";
    std::cout << "   - IsKeyReleased(key): True only on first frame when key goes up\n\n";

    std::cout << "2. Mouse Buttons:\n";
    std::cout << "   - IsMouseButtonPressed(button): Click detection\n";
    std::cout << "   - IsMouseButtonHeld(button): Drag detection\n";
    std::cout << "   - IsMouseButtonReleased(button): Release detection\n\n";

    std::cout << "3. Mouse Movement:\n";
    std::cout << "   - GetMousePosition(): Current position in window\n";
    std::cout << "   - GetMouseDelta(): Movement since last frame\n";
    std::cout << "   - GetMouseScroll(): Scroll wheel delta\n\n";

    std::cout << "4. Key Codes (examples):\n";
    std::cout << "   - KeyCode::W, A, S, D (movement)\n";
    std::cout << "   - KeyCode::Space, Escape, Enter\n";
    std::cout << "   - KeyCode::Up, Down, Left, Right (arrows)\n";
    std::cout << "   - KeyCode::Num1 through Num0\n";
    std::cout << "   - KeyCode::LeftShift, LeftControl, LeftAlt\n\n";

    std::cout << "5. Mouse Buttons:\n";
    std::cout << "   - MouseButton::Left, Middle, Right\n";
    std::cout << "   - MouseButton::X1, X2 (extra buttons)\n\n";

    std::cout << "Example Usage:\n";
    std::cout << "--------------\n";
    std::cout << "if (Input::IsKeyPressed(KeyCode::Space))\n";
    std::cout << "    Jump();  // Only jumps once per press\n\n";
    std::cout << "if (Input::IsKeyHeld(KeyCode::W))\n";
    std::cout << "    MoveForward(deltaTime);  // Continuous movement\n\n";
    std::cout << "if (Input::IsMouseButtonPressed(MouseButton::Left))\n";
    std::cout << "    StartDrag();\n\n";
    std::cout << "glm::vec2 mousePos = Input::GetMousePosition();\n";
    std::cout << "glm::vec2 mouseDelta = Input::GetMouseDelta();\n\n";

    std::cout << "==========================================\n";
    std::cout << "  Try the controls in the stress test!\n";
    std::cout << "==========================================\n\n";
}
