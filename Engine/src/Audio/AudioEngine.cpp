#include "Engine/Audio/AudioEngine.h"

#ifndef __EMSCRIPTEN__
#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#endif

namespace Engine {

bool   AudioEngine::s_Initialized = false;
void*  AudioEngine::s_Device      = nullptr;
void*  AudioEngine::s_Context     = nullptr;

#ifdef __EMSCRIPTEN__
// Audio is silent on WebAssembly builds (OpenAL/sndfile not available)
void  AudioEngine::Init()              { s_Initialized = true; }
void  AudioEngine::Shutdown()          { s_Initialized = false; }
void  AudioEngine::SetMasterVolume(float) {}
float AudioEngine::GetMasterVolume()   { return 1.0f; }

#else

void AudioEngine::Init()
{
    // Open default audio device
    ALCdevice* device = alcOpenDevice(nullptr);
    if (!device)
    {
        std::cerr << "[AudioEngine] Failed to open audio device\n";
        return;
    }

    ALCcontext* context = alcCreateContext(device, nullptr);
    if (!context)
    {
        std::cerr << "[AudioEngine] Failed to create audio context\n";
        alcCloseDevice(device);
        return;
    }

    if (!alcMakeContextCurrent(context))
    {
        std::cerr << "[AudioEngine] Failed to make audio context current\n";
        alcDestroyContext(context);
        alcCloseDevice(device);
        return;
    }

    s_Device      = device;
    s_Context     = context;
    s_Initialized = true;

    // Default listener orientation: looking down -Z, up is +Y
    float orientation[] = { 0.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f };
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListenerfv(AL_ORIENTATION, orientation);
    alListenerf(AL_GAIN, 1.0f);

    std::cout << "[AudioEngine] Initialized — device: "
              << alcGetString(device, ALC_DEVICE_SPECIFIER) << "\n";
}

void AudioEngine::Shutdown()
{
    if (!s_Initialized)
        return;

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(static_cast<ALCcontext*>(s_Context));
    alcCloseDevice(static_cast<ALCdevice*>(s_Device));

    s_Context     = nullptr;
    s_Device      = nullptr;
    s_Initialized = false;

    std::cout << "[AudioEngine] Shutdown\n";
}

void AudioEngine::SetMasterVolume(float volume)
{
    if (s_Initialized)
        alListenerf(AL_GAIN, volume);
}

float AudioEngine::GetMasterVolume()
{
    float gain = 1.0f;
    if (s_Initialized)
        alGetListenerf(AL_GAIN, &gain);
    return gain;
}

#endif // __EMSCRIPTEN__

} // namespace Engine
