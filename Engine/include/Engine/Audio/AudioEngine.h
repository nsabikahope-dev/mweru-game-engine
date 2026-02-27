#pragma once

namespace Engine {

/**
 * @brief Manages the OpenAL device and context.
 *
 * Call Init() once at application startup and Shutdown() at exit.
 * All other audio classes check IsInitialized() before touching AL.
 */
class AudioEngine
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized() { return s_Initialized; }

    /** Global volume: 0.0 (silent) to 1.0 (full). */
    static void  SetMasterVolume(float volume);
    static float GetMasterVolume();

private:
    static bool   s_Initialized;
    static void*  s_Device;    // ALCdevice*
    static void*  s_Context;   // ALCcontext*
};

} // namespace Engine
