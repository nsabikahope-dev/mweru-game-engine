#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>
#include <vector>
#include <array>

class b2Body; // Forward declaration (global namespace)

namespace Engine {

class Texture2D; // Forward declaration

/**
 * @brief Tag component - gives an entity a name
 */
struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag)
        : Tag(tag) {}
};

/**
 * @brief Transform component - position, rotation, scale
 */
struct TransformComponent
{
    glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& position)
        : Position(position) {}

    /**
     * @brief Get the transform matrix (TRS: Translation * Rotation * Scale)
     */
    glm::mat4 GetTransform() const
    {
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), Rotation.x, { 1, 0, 0 })
                           * glm::rotate(glm::mat4(1.0f), Rotation.y, { 0, 1, 0 })
                           * glm::rotate(glm::mat4(1.0f), Rotation.z, { 0, 0, 1 });

        return glm::translate(glm::mat4(1.0f), Position)
             * rotation
             * glm::scale(glm::mat4(1.0f), Scale);
    }
};

/**
 * @brief Sprite renderer component - renders a colored sprite with optional texture
 */
struct SpriteRendererComponent
{
    glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
    std::shared_ptr<Texture2D> Texture;

    SpriteRendererComponent() = default;
    SpriteRendererComponent(const SpriteRendererComponent&) = default;
    SpriteRendererComponent(const glm::vec4& color)
        : Color(color) {}
};

/**
 * @brief Camera component - defines a camera view
 */
struct CameraComponent
{
    bool Primary = true; // Is this the primary/active camera?
    bool FixedAspectRatio = false;

    float OrthographicSize = 10.0f;
    float OrthographicNear = -1.0f;
    float OrthographicFar = 1.0f;

    CameraComponent() = default;
    CameraComponent(const CameraComponent&) = default;

    /**
     * @brief Get the projection matrix
     */
    glm::mat4 GetProjection(float aspectRatio) const
    {
        float orthoLeft = -OrthographicSize * aspectRatio * 0.5f;
        float orthoRight = OrthographicSize * aspectRatio * 0.5f;
        float orthoBottom = -OrthographicSize * 0.5f;
        float orthoTop = OrthographicSize * 0.5f;

        return glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, OrthographicNear, OrthographicFar);
    }
};

/**
 * @brief Rigidbody component - physics simulation body
 */
struct RigidbodyComponent
{
    enum class BodyType { Static = 0, Kinematic, Dynamic };

    BodyType Type = BodyType::Dynamic;
    bool FixedRotation = false;

    // Physics properties
    float Mass = 1.0f;
    float GravityScale = 1.0f;
    float LinearDamping = 0.0f;
    float AngularDamping = 0.01f;

    // Runtime Box2D body (managed by PhysicsSystem)
    b2Body* RuntimeBody = nullptr;

    RigidbodyComponent() = default;
    RigidbodyComponent(const RigidbodyComponent&) = default;
};

/**
 * @brief Box collider component - rectangular collision shape
 */
struct BoxColliderComponent
{
    glm::vec2 Size = { 1.0f, 1.0f };
    glm::vec2 Offset = { 0.0f, 0.0f };

    // Physics material
    float Density = 1.0f;
    float Friction = 0.3f;
    float Restitution = 0.0f;  // Bounciness (0 = no bounce, 1 = perfect bounce)

    // Runtime data (managed by PhysicsSystem)
    void* RuntimeFixture = nullptr;

    BoxColliderComponent() = default;
    BoxColliderComponent(const BoxColliderComponent&) = default;
};

/**
 * @brief Circle collider component - circular collision shape
 */
struct CircleColliderComponent
{
    float Radius = 0.5f;
    glm::vec2 Offset = { 0.0f, 0.0f };

    // Physics material
    float Density = 1.0f;
    float Friction = 0.3f;
    float Restitution = 0.0f;

    // Runtime data (managed by PhysicsSystem)
    void* RuntimeFixture = nullptr;

    CircleColliderComponent() = default;
    CircleColliderComponent(const CircleColliderComponent&) = default;
};

/**
 * @brief Script component - attaches a Lua script to an entity
 */
struct ScriptComponent
{
    std::string ScriptPath;   // Path to the .lua file
    bool Enabled = true;

    ScriptComponent() = default;
    ScriptComponent(const ScriptComponent&) = default;
    ScriptComponent(const std::string& path) : ScriptPath(path) {}
};

// =========================================================================
// Phase 7: Audio
// =========================================================================

/**
 * @brief Audio source - plays a sound file at this entity's location.
 *
 * Supported formats: .wav, .ogg, .flac (anything libsndfile can read).
 */
struct AudioSourceComponent
{
    std::string ClipPath;          // Path to audio file
    float       Volume     = 1.0f; // 0 = silent, 1 = full
    float       Pitch      = 1.0f; // 1 = normal speed
    bool        Loop       = false;
    bool        PlayOnStart = true;
    bool        Spatial    = false; // true = 3D positional audio

    // Runtime (managed by AudioSystem — do not set manually)
    unsigned int SourceID = 0;
    unsigned int BufferID = 0;

    AudioSourceComponent() = default;
    AudioSourceComponent(const AudioSourceComponent&) = default;
    AudioSourceComponent(const std::string& path) : ClipPath(path) {}
};

/**
 * @brief Audio listener - marks this entity as the ears of the scene.
 *
 * Only one listener should exist per scene. Its TransformComponent
 * position is used to spatialise all AudioSourceComponents.
 */
struct AudioListenerComponent
{
    bool Active = true;  // Dummy field — EnTT ETO skips empty structs

    AudioListenerComponent() = default;
    AudioListenerComponent(const AudioListenerComponent&) = default;
};

// =========================================================================
// Phase 12: Animation
// =========================================================================

/**
 * @brief Frame-based sprite animation.
 *
 * Stores a list of texture file paths (one per frame). At runtime the
 * AnimationSystem swaps the entity's SpriteRendererComponent::Texture
 * every FrameTime seconds.
 *
 * Usage:
 *   BasePath = "assets/sprites/player_walk"
 *   FrameCount = 4
 *   → loads player_walk_0.png … player_walk_3.png
 *
 * Or set FramePaths explicitly for non-sequential names.
 */
struct SpriteAnimationComponent
{
    std::string BasePath;           // Optional: base name for auto-naming
    int         FrameCount  = 1;   // Used with BasePath; ignored if FramePaths set
    float       FrameTime   = 0.1f; // Seconds per frame
    bool        Loop        = true;
    bool        Playing     = true;

    // Explicit per-frame paths (populated from BasePath+FrameCount if empty at start)
    std::vector<std::string> FramePaths;

    // Runtime
    int   CurrentFrame = 0;
    float Timer        = 0.0f;
    // Loaded textures (managed by AnimationSystem)
    std::vector<std::shared_ptr<Texture2D>> Frames;

    SpriteAnimationComponent() = default;
    SpriteAnimationComponent(const SpriteAnimationComponent&) = default;
};

// =========================================================================
// Phase 12: Particles
// =========================================================================

struct Particle
{
    glm::vec2 Position       = { 0.0f, 0.0f };
    glm::vec2 Velocity       = { 0.0f, 0.0f };
    glm::vec4 ColorBegin     = { 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 ColorEnd       = { 1.0f, 1.0f, 1.0f, 0.0f };
    float     SizeBegin      = 0.3f;
    float     SizeEnd        = 0.0f;
    float     LifeTime       = 1.0f;
    float     LifeRemaining  = 0.0f;
    bool      Active         = false;
};

/**
 * @brief Simple CPU particle emitter.
 *
 * Emits coloured quad particles from the entity's position.
 * The ParticleSystem handles spawning, update, and rendering.
 */
struct ParticleEmitterComponent
{
    // Emission config
    glm::vec2 Velocity          = {  0.0f,  2.0f };
    glm::vec2 VelocityVariation = {  1.0f,  0.5f };
    glm::vec4 ColorBegin        = {  1.0f,  0.5f, 0.0f, 1.0f };
    glm::vec4 ColorEnd          = {  0.5f,  0.0f, 0.0f, 0.0f };
    float     SizeBegin         = 0.3f;
    float     SizeEnd           = 0.0f;
    float     LifeTime          = 1.5f;
    float     EmissionRate      = 20.0f; // particles per second
    bool      Emitting          = false;

    // Runtime particle pool
    static constexpr int MaxParticles = 200;
    std::array<Particle, MaxParticles> Particles{};
    int   PoolIndex  = MaxParticles - 1; // ring-buffer head
    float EmitTimer  = 0.0f;

    ParticleEmitterComponent() = default;
    ParticleEmitterComponent(const ParticleEmitterComponent&) = default;
};

// =========================================================================
// Visual Novel / Comics (Phase 13)
// =========================================================================

/**
 * @brief Renders a text string at the entity's world position using the
 *        built-in bitmap font.
 *
 * Text origin is the bottom-left corner of the first character.
 * Multi-line text is supported via embedded '\\n' characters.
 */
struct TextComponent
{
    std::string Text       = "Hello World";
    glm::vec4   Color      = { 1.0f, 1.0f, 1.0f, 1.0f };
    float       FontSize   = 0.5f;   // world units per character cell
    float       LineSpacing = 1.2f;  // multiplier on FontSize between lines
    bool        Visible    = true;

    TextComponent() = default;
    TextComponent(const TextComponent&) = default;
    TextComponent(const std::string& text) : Text(text) {}
};

/**
 * @brief A single line of visual-novel dialogue.
 */
struct DialogueLine
{
    std::string              Speaker;
    std::string              Text;
    std::vector<std::string> Choices; // optional branch choices (future use)

    DialogueLine() = default;
    DialogueLine(const std::string& spk, const std::string& txt)
        : Speaker(spk), Text(txt) {}
};

/**
 * @brief Visual-novel style dialogue box rendered in world-space.
 *
 * The entity's Transform controls the screen position.
 * DialogueSystem advances lines when the configured key is pressed.
 */
struct DialogueComponent
{
    std::vector<DialogueLine> Lines;

    int   CurrentLine      = 0;
    bool  Active           = false;
    bool  AutoAdvance      = false;
    float AutoAdvanceTime  = 3.0f;  // seconds per line
    float AutoAdvanceTimer = 0.0f;

    // Visual styling
    glm::vec4   BoxColor     = { 0.0f, 0.0f, 0.0f, 0.85f };
    glm::vec4   TextColor    = { 1.0f, 1.0f, 1.0f, 1.0f  };
    glm::vec4   SpeakerColor = { 1.0f, 0.9f, 0.3f, 1.0f  };
    float       BoxHeight    = 2.5f;   // world units
    float       FontSize     = 0.35f;  // character cell size
    std::string AdvanceKey   = "Space";

    DialogueComponent() = default;
    DialogueComponent(const DialogueComponent&) = default;
};

/**
 * @brief Game timer — counts up or down; fires OnTimerExpired() Lua callback.
 */
struct TimerComponent
{
    float Duration  = 60.0f; // seconds total; 0 = unlimited count-up
    float Elapsed   = 0.0f;
    bool  Active    = true;
    bool  CountDown = true;  // false → count up
    bool  Expired   = false;
    bool  Loop      = false; // restart automatically when expired

    float GetRemaining() const { return Duration > 0.0f ? std::max(0.0f, Duration - Elapsed) : 0.0f; }
    float GetDisplayTime() const { return CountDown ? GetRemaining() : Elapsed; }

    TimerComponent() = default;
    TimerComponent(const TimerComponent&) = default;
};

/**
 * @brief Video playback component.
 *
 * In the web runtime this plays an HTML5 <video> element overlaid on the
 * canvas. Set AutoPlay = true to start immediately when the scene begins, or
 * call PlayVideo(url) from a Lua script to start at a specific moment.
 *
 * Lua API (bound to the entity that owns this component):
 *   PlayVideo(url)       — load and start a video URL
 *   PauseVideo()         — pause playback
 *   StopVideo()          — stop and hide video
 *   IsVideoPlaying()     — bool
 *   SetVideoLoop(bool)   — toggle looping
 *
 * On desktop the calls are no-ops (console log only); video is a web feature.
 */
struct VideoComponent
{
    std::string VideoUrl;
    bool        Loop     = false;
    bool        AutoPlay = false;
    bool        Visible  = true;

    VideoComponent() = default;
    VideoComponent(const VideoComponent&) = default;
    VideoComponent(const std::string& url) : VideoUrl(url) {}
};

/**
 * @brief Comic-panel border drawn around the entity's bounding box.
 *
 * Useful for creating manga / comic-style panel layouts.
 */
struct PanelComponent
{
    glm::vec4 BackgroundColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 BorderColor     = { 0.0f, 0.0f, 0.0f, 1.0f };
    float     BorderWidth     = 0.08f; // world units
    bool      ShowBackground  = true;
    bool      ShowBorder      = true;

    PanelComponent() = default;
    PanelComponent(const PanelComponent&) = default;
};

} // namespace Engine
