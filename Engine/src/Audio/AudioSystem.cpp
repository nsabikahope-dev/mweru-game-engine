#include "Engine/Audio/AudioSystem.h"
#include "Engine/Audio/AudioEngine.h"
#include "Engine/ECS/Components.h"
#include "Engine/ECS/Entity.h"
#include "Engine/Scene/Scene.h"

#include <AL/al.h>
#include <sndfile.h>

#include <iostream>
#include <string>
#include <vector>

namespace Engine {

// -----------------------------------------------------------------------
// Private: load an audio file into an OpenAL buffer
// -----------------------------------------------------------------------
unsigned int AudioSystem::LoadBuffer(const std::string& path)
{
    SF_INFO sfInfo{};
    SNDFILE* sndFile = sf_open(path.c_str(), SFM_READ, &sfInfo);
    if (!sndFile)
    {
        std::cerr << "[AudioSystem] Failed to open audio file: " << path << "\n";
        return 0;
    }

    std::vector<int16_t> samples(static_cast<size_t>(sfInfo.frames * sfInfo.channels));
    sf_count_t read = sf_read_short(sndFile, samples.data(),
                                     static_cast<sf_count_t>(samples.size()));
    sf_close(sndFile);

    if (read == 0)
    {
        std::cerr << "[AudioSystem] Audio file has no samples: " << path << "\n";
        return 0;
    }

    ALenum format = (sfInfo.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    ALuint bufferID = 0;
    alGenBuffers(1, &bufferID);
    alBufferData(bufferID, format, samples.data(),
                 static_cast<ALsizei>(read * sizeof(int16_t)),
                 static_cast<ALsizei>(sfInfo.samplerate));

    ALenum err = alGetError();
    if (err != AL_NO_ERROR)
    {
        std::cerr << "[AudioSystem] OpenAL buffer error for " << path
                  << ": " << alGetString(err) << "\n";
        alDeleteBuffers(1, &bufferID);
        return 0;
    }

    return bufferID;
}

// -----------------------------------------------------------------------
// Private: stop + delete a single entity's AL resources
// -----------------------------------------------------------------------
void AudioSystem::FreeSource(Entity entity)
{
    if (!entity.HasComponent<AudioSourceComponent>())
        return;

    auto& src = entity.GetComponent<AudioSourceComponent>();

    if (src.SourceID)
    {
        alSourceStop(src.SourceID);
        alDeleteSources(1, &src.SourceID);
        src.SourceID = 0;
    }
    if (src.BufferID)
    {
        alDeleteBuffers(1, &src.BufferID);
        src.BufferID = 0;
    }
}

// -----------------------------------------------------------------------
// Public lifecycle
// -----------------------------------------------------------------------
void AudioSystem::OnSceneStart(Scene* scene)
{
    if (!AudioEngine::IsInitialized())
        return;

    auto view = scene->GetRegistry().view<AudioSourceComponent>();
    int count = 0;

    for (auto handle : view)
    {
        Entity entity(handle, scene);
        auto& src = entity.GetComponent<AudioSourceComponent>();

        if (src.ClipPath.empty())
            continue;

        // Load audio data into an AL buffer
        src.BufferID = LoadBuffer(src.ClipPath);
        if (!src.BufferID)
            continue;

        // Create AL source and configure it
        ALuint sourceID = 0;
        alGenSources(1, &sourceID);
        alSourcei(sourceID, AL_BUFFER, static_cast<ALint>(src.BufferID));
        alSourcef(sourceID, AL_GAIN,  src.Volume);
        alSourcef(sourceID, AL_PITCH, src.Pitch);
        alSourcei(sourceID, AL_LOOPING, src.Loop ? AL_TRUE : AL_FALSE);

        // Position: use entity transform if spatial audio is enabled
        if (src.Spatial && entity.HasComponent<TransformComponent>())
        {
            auto& t = entity.GetComponent<TransformComponent>();
            alSource3f(sourceID, AL_POSITION, t.Position.x, t.Position.y, t.Position.z);
        }
        else
        {
            // Non-spatial: place at listener so distance attenuation has no effect
            alSourcei(sourceID, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(sourceID, AL_POSITION, 0.0f, 0.0f, 0.0f);
        }

        src.SourceID = sourceID;
        ++count;

        if (src.PlayOnStart)
            alSourcePlay(sourceID);
    }

    std::cout << "[AudioSystem] Started " << count << " audio source(s)\n";
}

void AudioSystem::OnSceneStop(Scene* scene)
{
    if (!AudioEngine::IsInitialized())
        return;

    auto view = scene->GetRegistry().view<AudioSourceComponent>();
    for (auto handle : view)
    {
        Entity entity(handle, scene);
        FreeSource(entity);
    }

    std::cout << "[AudioSystem] Stopped all audio sources\n";
}

void AudioSystem::OnUpdate(Scene* scene)
{
    if (!AudioEngine::IsInitialized())
        return;

    // Update AL listener from the entity that carries AudioListenerComponent
    auto listenerView = scene->GetRegistry().view<AudioListenerComponent, TransformComponent>();
    for (auto handle : listenerView)
    {
        auto& t = scene->GetRegistry().get<TransformComponent>(handle);
        alListener3f(AL_POSITION, t.Position.x, t.Position.y, 0.0f);
        break; // Only one listener
    }

    // Update spatial source positions
    auto srcView = scene->GetRegistry().view<AudioSourceComponent, TransformComponent>();
    for (auto handle : srcView)
    {
        auto& src = scene->GetRegistry().get<AudioSourceComponent>(handle);
        if (!src.SourceID || !src.Spatial)
            continue;
        auto& t = scene->GetRegistry().get<TransformComponent>(handle);
        alSource3f(src.SourceID, AL_POSITION, t.Position.x, t.Position.y, t.Position.z);
    }
}

// -----------------------------------------------------------------------
// Per-entity controls
// -----------------------------------------------------------------------
void AudioSystem::Play(Entity entity)
{
    if (!AudioEngine::IsInitialized() || !entity.HasComponent<AudioSourceComponent>())
        return;
    auto& src = entity.GetComponent<AudioSourceComponent>();
    if (src.SourceID)
        alSourcePlay(src.SourceID);
}

void AudioSystem::Stop(Entity entity)
{
    if (!AudioEngine::IsInitialized() || !entity.HasComponent<AudioSourceComponent>())
        return;
    auto& src = entity.GetComponent<AudioSourceComponent>();
    if (src.SourceID)
        alSourceStop(src.SourceID);
}

void AudioSystem::Pause(Entity entity)
{
    if (!AudioEngine::IsInitialized() || !entity.HasComponent<AudioSourceComponent>())
        return;
    auto& src = entity.GetComponent<AudioSourceComponent>();
    if (src.SourceID)
        alSourcePause(src.SourceID);
}

void AudioSystem::SetVolume(Entity entity, float volume)
{
    if (!AudioEngine::IsInitialized() || !entity.HasComponent<AudioSourceComponent>())
        return;
    auto& src = entity.GetComponent<AudioSourceComponent>();
    src.Volume = volume;
    if (src.SourceID)
        alSourcef(src.SourceID, AL_GAIN, volume);
}

float AudioSystem::GetVolume(Entity entity)
{
    if (!entity.HasComponent<AudioSourceComponent>())
        return 0.0f;
    return entity.GetComponent<AudioSourceComponent>().Volume;
}

} // namespace Engine
