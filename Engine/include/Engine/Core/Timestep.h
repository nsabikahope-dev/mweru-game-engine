#pragma once

namespace Engine {

/**
 * @brief Represents the time between frames (delta time)
 *
 * Usage:
 *   Timestep ts(0.016f); // ~60 FPS
 *   float seconds = ts.GetSeconds();
 *   float ms = ts.GetMilliseconds();
 */
class Timestep
{
public:
    Timestep(float time = 0.0f)
        : m_Time(time)
    {
    }

    /**
     * @brief Get time in seconds
     */
    float GetSeconds() const { return m_Time; }

    /**
     * @brief Get time in milliseconds
     */
    float GetMilliseconds() const { return m_Time * 1000.0f; }

    /**
     * @brief Implicit conversion to float (returns seconds)
     */
    operator float() const { return m_Time; }

private:
    float m_Time; // Time in seconds
};

} // namespace Engine
