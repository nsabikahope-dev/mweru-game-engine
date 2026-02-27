#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Scene/Scene.h"
#include "Engine/ECS/Entity.h"
#include "Engine/ECS/Components.h"
#include "Engine/Rendering/Texture.h"

#include <json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace Engine {

static void SerializeEntity(json& j, Entity entity)
{
    // Tag
    if (entity.HasComponent<TagComponent>())
    {
        auto& tag = entity.GetComponent<TagComponent>();
        j["TagComponent"] = {
            {"Tag", tag.Tag}
        };
    }

    // Transform
    if (entity.HasComponent<TransformComponent>())
    {
        auto& transform = entity.GetComponent<TransformComponent>();
        j["TransformComponent"] = {
            {"Position", {transform.Position.x, transform.Position.y, transform.Position.z}},
            {"Rotation", {transform.Rotation.x, transform.Rotation.y, transform.Rotation.z}},
            {"Scale", {transform.Scale.x, transform.Scale.y, transform.Scale.z}}
        };
    }

    // Sprite Renderer
    if (entity.HasComponent<SpriteRendererComponent>())
    {
        auto& sprite = entity.GetComponent<SpriteRendererComponent>();
        j["SpriteRendererComponent"] = {
            {"Color", {sprite.Color.r, sprite.Color.g, sprite.Color.b, sprite.Color.a}},
            {"TexturePath", sprite.Texture ? sprite.Texture->GetPath() : ""}
        };
    }

    // Camera
    if (entity.HasComponent<CameraComponent>())
    {
        auto& camera = entity.GetComponent<CameraComponent>();
        j["CameraComponent"] = {
            {"Primary", camera.Primary},
            {"FixedAspectRatio", camera.FixedAspectRatio},
            {"OrthographicSize", camera.OrthographicSize},
            {"OrthographicNear", camera.OrthographicNear},
            {"OrthographicFar", camera.OrthographicFar}
        };
    }

    // Rigidbody
    if (entity.HasComponent<RigidbodyComponent>())
    {
        auto& rb = entity.GetComponent<RigidbodyComponent>();
        j["RigidbodyComponent"] = {
            {"Type", static_cast<int>(rb.Type)},
            {"FixedRotation", rb.FixedRotation},
            {"Mass", rb.Mass},
            {"GravityScale", rb.GravityScale},
            {"LinearDamping", rb.LinearDamping},
            {"AngularDamping", rb.AngularDamping}
        };
    }

    // Box Collider
    if (entity.HasComponent<BoxColliderComponent>())
    {
        auto& bc = entity.GetComponent<BoxColliderComponent>();
        j["BoxColliderComponent"] = {
            {"Size", {bc.Size.x, bc.Size.y}},
            {"Offset", {bc.Offset.x, bc.Offset.y}},
            {"Density", bc.Density},
            {"Friction", bc.Friction},
            {"Restitution", bc.Restitution}
        };
    }

    // Circle Collider
    if (entity.HasComponent<CircleColliderComponent>())
    {
        auto& cc = entity.GetComponent<CircleColliderComponent>();
        j["CircleColliderComponent"] = {
            {"Radius", cc.Radius},
            {"Offset", {cc.Offset.x, cc.Offset.y}},
            {"Density", cc.Density},
            {"Friction", cc.Friction},
            {"Restitution", cc.Restitution}
        };
    }

    // Script
    if (entity.HasComponent<ScriptComponent>())
    {
        auto& sc = entity.GetComponent<ScriptComponent>();
        j["ScriptComponent"] = {
            {"ScriptPath", sc.ScriptPath},
            {"Enabled", sc.Enabled}
        };
    }

    // Audio Source
    if (entity.HasComponent<AudioSourceComponent>())
    {
        auto& src = entity.GetComponent<AudioSourceComponent>();
        j["AudioSourceComponent"] = {
            {"ClipPath",    src.ClipPath},
            {"Volume",      src.Volume},
            {"Pitch",       src.Pitch},
            {"Loop",        src.Loop},
            {"PlayOnStart", src.PlayOnStart},
            {"Spatial",     src.Spatial}
        };
    }

    // Audio Listener
    if (entity.HasComponent<AudioListenerComponent>())
    {
        j["AudioListenerComponent"] = json::object();
    }

    // Sprite Animation
    if (entity.HasComponent<SpriteAnimationComponent>())
    {
        auto& anim = entity.GetComponent<SpriteAnimationComponent>();
        json paths = json::array();
        for (const auto& p : anim.FramePaths)
            paths.push_back(p);

        j["SpriteAnimationComponent"] = {
            {"BasePath",   anim.BasePath},
            {"FrameCount", anim.FrameCount},
            {"FrameTime",  anim.FrameTime},
            {"Loop",       anim.Loop},
            {"Playing",    anim.Playing},
            {"FramePaths", paths}
        };
    }

    // Particle Emitter
    if (entity.HasComponent<ParticleEmitterComponent>())
    {
        auto& e = entity.GetComponent<ParticleEmitterComponent>();
        j["ParticleEmitterComponent"] = {
            {"Velocity",          {e.Velocity.x, e.Velocity.y}},
            {"VelocityVariation", {e.VelocityVariation.x, e.VelocityVariation.y}},
            {"ColorBegin",        {e.ColorBegin.r, e.ColorBegin.g, e.ColorBegin.b, e.ColorBegin.a}},
            {"ColorEnd",          {e.ColorEnd.r,   e.ColorEnd.g,   e.ColorEnd.b,   e.ColorEnd.a}},
            {"SizeBegin",         e.SizeBegin},
            {"SizeEnd",           e.SizeEnd},
            {"LifeTime",          e.LifeTime},
            {"EmissionRate",      e.EmissionRate},
            {"Emitting",          e.Emitting}
        };
    }

    // Text
    if (entity.HasComponent<TextComponent>())
    {
        auto& tc = entity.GetComponent<TextComponent>();
        j["TextComponent"] = {
            {"Text",        tc.Text},
            {"Color",       {tc.Color.r, tc.Color.g, tc.Color.b, tc.Color.a}},
            {"FontSize",    tc.FontSize},
            {"LineSpacing", tc.LineSpacing},
            {"Visible",     tc.Visible}
        };
    }

    // Dialogue
    if (entity.HasComponent<DialogueComponent>())
    {
        auto& dlg = entity.GetComponent<DialogueComponent>();
        json linesArr = json::array();
        for (const auto& ln : dlg.Lines)
        {
            json lineJ;
            lineJ["Speaker"] = ln.Speaker;
            lineJ["Text"]    = ln.Text;
            json choicesArr = json::array();
            for (const auto& ch : ln.Choices)
                choicesArr.push_back(ch);
            lineJ["Choices"] = choicesArr;
            linesArr.push_back(lineJ);
        }
        j["DialogueComponent"] = {
            {"Lines",           linesArr},
            {"AutoAdvance",     dlg.AutoAdvance},
            {"AutoAdvanceTime", dlg.AutoAdvanceTime},
            {"BoxColor",        {dlg.BoxColor.r,     dlg.BoxColor.g,     dlg.BoxColor.b,     dlg.BoxColor.a}},
            {"TextColor",       {dlg.TextColor.r,    dlg.TextColor.g,    dlg.TextColor.b,    dlg.TextColor.a}},
            {"SpeakerColor",    {dlg.SpeakerColor.r, dlg.SpeakerColor.g, dlg.SpeakerColor.b, dlg.SpeakerColor.a}},
            {"BoxHeight",       dlg.BoxHeight},
            {"FontSize",        dlg.FontSize},
            {"AdvanceKey",      dlg.AdvanceKey}
        };
    }

    // Panel
    if (entity.HasComponent<PanelComponent>())
    {
        auto& panel = entity.GetComponent<PanelComponent>();
        j["PanelComponent"] = {
            {"BackgroundColor", {panel.BackgroundColor.r, panel.BackgroundColor.g, panel.BackgroundColor.b, panel.BackgroundColor.a}},
            {"BorderColor",     {panel.BorderColor.r,     panel.BorderColor.g,     panel.BorderColor.b,     panel.BorderColor.a}},
            {"BorderWidth",     panel.BorderWidth},
            {"ShowBackground",  panel.ShowBackground},
            {"ShowBorder",      panel.ShowBorder}
        };
    }
}

static void DeserializeEntity(const json& j, Entity entity)
{
    // Tag
    if (j.contains("TagComponent"))
    {
        auto& tag = entity.GetComponent<TagComponent>();
        tag.Tag = j["TagComponent"]["Tag"];
    }

    // Transform
    if (j.contains("TransformComponent"))
    {
        auto& transform = entity.GetComponent<TransformComponent>();
        auto pos = j["TransformComponent"]["Position"];
        auto rot = j["TransformComponent"]["Rotation"];
        auto scale = j["TransformComponent"]["Scale"];

        transform.Position = glm::vec3(pos[0], pos[1], pos[2]);
        transform.Rotation = glm::vec3(rot[0], rot[1], rot[2]);
        transform.Scale = glm::vec3(scale[0], scale[1], scale[2]);
    }

    // Sprite Renderer
    if (j.contains("SpriteRendererComponent"))
    {
        auto& sprite = entity.AddComponent<SpriteRendererComponent>();
        auto color = j["SpriteRendererComponent"]["Color"];
        sprite.Color = glm::vec4(color[0], color[1], color[2], color[3]);

        std::string texturePath = j["SpriteRendererComponent"]["TexturePath"];
        if (!texturePath.empty())
        {
            sprite.Texture = Texture2D::Create(texturePath);
        }
    }

    // Camera
    if (j.contains("CameraComponent"))
    {
        auto& camera = entity.AddComponent<CameraComponent>();
        camera.Primary = j["CameraComponent"]["Primary"];
        camera.FixedAspectRatio = j["CameraComponent"]["FixedAspectRatio"];
        camera.OrthographicSize = j["CameraComponent"]["OrthographicSize"];
        camera.OrthographicNear = j["CameraComponent"]["OrthographicNear"];
        camera.OrthographicFar = j["CameraComponent"]["OrthographicFar"];
    }

    // Rigidbody
    if (j.contains("RigidbodyComponent"))
    {
        auto& rb = entity.AddComponent<RigidbodyComponent>();
        rb.Type = static_cast<RigidbodyComponent::BodyType>(j["RigidbodyComponent"]["Type"].get<int>());
        rb.FixedRotation = j["RigidbodyComponent"]["FixedRotation"];
        rb.Mass = j["RigidbodyComponent"]["Mass"];
        rb.GravityScale = j["RigidbodyComponent"]["GravityScale"];
        rb.LinearDamping = j["RigidbodyComponent"]["LinearDamping"];
        rb.AngularDamping = j["RigidbodyComponent"]["AngularDamping"];
    }

    // Box Collider
    if (j.contains("BoxColliderComponent"))
    {
        auto& bc = entity.AddComponent<BoxColliderComponent>();
        auto size = j["BoxColliderComponent"]["Size"];
        auto offset = j["BoxColliderComponent"]["Offset"];

        bc.Size = glm::vec2(size[0], size[1]);
        bc.Offset = glm::vec2(offset[0], offset[1]);
        bc.Density = j["BoxColliderComponent"]["Density"];
        bc.Friction = j["BoxColliderComponent"]["Friction"];
        bc.Restitution = j["BoxColliderComponent"]["Restitution"];
    }

    // Circle Collider
    if (j.contains("CircleColliderComponent"))
    {
        auto& cc = entity.AddComponent<CircleColliderComponent>();
        auto offset = j["CircleColliderComponent"]["Offset"];

        cc.Radius = j["CircleColliderComponent"]["Radius"];
        cc.Offset = glm::vec2(offset[0], offset[1]);
        cc.Density = j["CircleColliderComponent"]["Density"];
        cc.Friction = j["CircleColliderComponent"]["Friction"];
        cc.Restitution = j["CircleColliderComponent"]["Restitution"];
    }

    // Script
    if (j.contains("ScriptComponent"))
    {
        auto& sc = entity.AddComponent<ScriptComponent>();
        sc.ScriptPath = j["ScriptComponent"]["ScriptPath"];
        sc.Enabled = j["ScriptComponent"]["Enabled"];
    }

    // Audio Source
    if (j.contains("AudioSourceComponent"))
    {
        auto& src = entity.AddComponent<AudioSourceComponent>();
        src.ClipPath    = j["AudioSourceComponent"]["ClipPath"];
        src.Volume      = j["AudioSourceComponent"]["Volume"];
        src.Pitch       = j["AudioSourceComponent"]["Pitch"];
        src.Loop        = j["AudioSourceComponent"]["Loop"];
        src.PlayOnStart = j["AudioSourceComponent"]["PlayOnStart"];
        src.Spatial     = j["AudioSourceComponent"]["Spatial"];
    }

    // Audio Listener
    if (j.contains("AudioListenerComponent"))
    {
        entity.AddComponent<AudioListenerComponent>();
    }

    // Sprite Animation
    if (j.contains("SpriteAnimationComponent"))
    {
        auto& anim = entity.AddComponent<SpriteAnimationComponent>();
        anim.BasePath   = j["SpriteAnimationComponent"]["BasePath"];
        anim.FrameCount = j["SpriteAnimationComponent"]["FrameCount"];
        anim.FrameTime  = j["SpriteAnimationComponent"]["FrameTime"];
        anim.Loop       = j["SpriteAnimationComponent"]["Loop"];
        anim.Playing    = j["SpriteAnimationComponent"]["Playing"];

        for (const auto& p : j["SpriteAnimationComponent"]["FramePaths"])
            anim.FramePaths.push_back(p.get<std::string>());
    }

    // Particle Emitter
    if (j.contains("ParticleEmitterComponent"))
    {
        auto& e = entity.AddComponent<ParticleEmitterComponent>();
        auto vel = j["ParticleEmitterComponent"]["Velocity"];
        auto velVar = j["ParticleEmitterComponent"]["VelocityVariation"];
        auto cb  = j["ParticleEmitterComponent"]["ColorBegin"];
        auto ce  = j["ParticleEmitterComponent"]["ColorEnd"];

        e.Velocity          = glm::vec2(vel[0], vel[1]);
        e.VelocityVariation = glm::vec2(velVar[0], velVar[1]);
        e.ColorBegin        = glm::vec4(cb[0], cb[1], cb[2], cb[3]);
        e.ColorEnd          = glm::vec4(ce[0], ce[1], ce[2], ce[3]);
        e.SizeBegin         = j["ParticleEmitterComponent"]["SizeBegin"];
        e.SizeEnd           = j["ParticleEmitterComponent"]["SizeEnd"];
        e.LifeTime          = j["ParticleEmitterComponent"]["LifeTime"];
        e.EmissionRate      = j["ParticleEmitterComponent"]["EmissionRate"];
        e.Emitting          = j["ParticleEmitterComponent"]["Emitting"];
    }

    // Text
    if (j.contains("TextComponent"))
    {
        auto& tc = entity.AddComponent<TextComponent>();
        tc.Text       = j["TextComponent"]["Text"].get<std::string>();
        auto c = j["TextComponent"]["Color"];
        tc.Color      = glm::vec4(c[0], c[1], c[2], c[3]);
        tc.FontSize   = j["TextComponent"]["FontSize"];
        tc.LineSpacing = j["TextComponent"]["LineSpacing"];
        tc.Visible    = j["TextComponent"]["Visible"];
    }

    // Dialogue
    if (j.contains("DialogueComponent"))
    {
        auto& dlg = entity.AddComponent<DialogueComponent>();
        dlg.AutoAdvance    = j["DialogueComponent"]["AutoAdvance"];
        dlg.AutoAdvanceTime = j["DialogueComponent"]["AutoAdvanceTime"];
        auto bc = j["DialogueComponent"]["BoxColor"];
        dlg.BoxColor     = glm::vec4(bc[0], bc[1], bc[2], bc[3]);
        auto tc2 = j["DialogueComponent"]["TextColor"];
        dlg.TextColor    = glm::vec4(tc2[0], tc2[1], tc2[2], tc2[3]);
        auto sc = j["DialogueComponent"]["SpeakerColor"];
        dlg.SpeakerColor = glm::vec4(sc[0], sc[1], sc[2], sc[3]);
        dlg.BoxHeight    = j["DialogueComponent"]["BoxHeight"];
        dlg.FontSize     = j["DialogueComponent"]["FontSize"];
        dlg.AdvanceKey   = j["DialogueComponent"]["AdvanceKey"].get<std::string>();

        for (const auto& lineJ : j["DialogueComponent"]["Lines"])
        {
            DialogueLine ln;
            ln.Speaker = lineJ["Speaker"].get<std::string>();
            ln.Text    = lineJ["Text"].get<std::string>();
            for (const auto& ch : lineJ["Choices"])
                ln.Choices.push_back(ch.get<std::string>());
            dlg.Lines.push_back(std::move(ln));
        }
    }

    // Panel
    if (j.contains("PanelComponent"))
    {
        auto& panel = entity.AddComponent<PanelComponent>();
        auto bgc = j["PanelComponent"]["BackgroundColor"];
        panel.BackgroundColor = glm::vec4(bgc[0], bgc[1], bgc[2], bgc[3]);
        auto brc = j["PanelComponent"]["BorderColor"];
        panel.BorderColor     = glm::vec4(brc[0], brc[1], brc[2], brc[3]);
        panel.BorderWidth     = j["PanelComponent"]["BorderWidth"];
        panel.ShowBackground  = j["PanelComponent"]["ShowBackground"];
        panel.ShowBorder      = j["PanelComponent"]["ShowBorder"];
    }
}

bool SceneSerializer::Serialize(const Scene* scene, const std::string& filepath)
{
    std::string jsonString = SerializeToString(scene);

    std::ofstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[SceneSerializer] Failed to open file for writing: " << filepath << "\n";
        return false;
    }

    file << jsonString;
    file.close();

    std::cout << "[SceneSerializer] Scene saved to: " << filepath << "\n";
    return true;
}

bool SceneSerializer::Deserialize(Scene* scene, const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[SceneSerializer] Failed to open file for reading: " << filepath << "\n";
        return false;
    }

    std::string jsonString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return DeserializeFromString(scene, jsonString);
}

std::string SceneSerializer::SerializeToString(const Scene* scene)
{
    json sceneJson;
    sceneJson["Scene"] = "Untitled";
    sceneJson["Entities"] = json::array();

    // Serialize all entities
    auto view = scene->GetRegistry().view<TagComponent>();
    for (auto entityHandle : view)
    {
        Entity entity(entityHandle, const_cast<Scene*>(scene));

        json entityJson;
        entityJson["Entity"] = static_cast<uint32_t>(entityHandle);
        SerializeEntity(entityJson, entity);

        sceneJson["Entities"].push_back(entityJson);
    }

    return sceneJson.dump(4);  // Pretty print with 4-space indentation
}

bool SceneSerializer::DeserializeFromString(Scene* scene, const std::string& jsonString)
{
    try
    {
        json sceneJson = json::parse(jsonString);

        if (!sceneJson.contains("Entities"))
        {
            std::cerr << "[SceneSerializer] Invalid scene file: missing Entities array\n";
            return false;
        }

        // Deserialize all entities
        for (const auto& entityJson : sceneJson["Entities"])
        {
            std::string entityName = entityJson.contains("TagComponent") ?
                entityJson["TagComponent"]["Tag"] : "Entity";

            Entity entity = scene->CreateEntity(entityName);
            DeserializeEntity(entityJson, entity);
        }

        std::cout << "[SceneSerializer] Scene loaded successfully!\n";
        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "[SceneSerializer] JSON parsing error: " << e.what() << "\n";
        return false;
    }
}

} // namespace Engine
