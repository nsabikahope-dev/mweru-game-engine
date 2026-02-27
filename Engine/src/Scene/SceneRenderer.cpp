#include "Engine/Scene/SceneRenderer.h"
#include "Engine/Scene/Scene.h"
#include "Engine/ECS/Components.h"
#include "Engine/Rendering/Renderer2D.h"
#include "Engine/Rendering/Font.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

SceneRenderer::SceneRenderer()
{
    Renderer2D::Init();
    Font::Init();
}

SceneRenderer::~SceneRenderer()
{
    Font::Shutdown();
    Renderer2D::Shutdown();
}

// ---------------------------------------------------------------------------
// Internal render helpers
// ---------------------------------------------------------------------------

static void RenderPanels(Scene* scene)
{
    auto view = scene->GetRegistry().view<TransformComponent, PanelComponent>();
    for (auto handle : view)
    {
        auto [tf, panel] = view.get<TransformComponent, PanelComponent>(handle);

        // Background quad
        if (panel.ShowBackground)
            Renderer2D::DrawQuad(tf.GetTransform(), panel.BackgroundColor);

        // Border: four thin quads around the edges
        if (panel.ShowBorder && panel.BorderWidth > 0.0f)
        {
            float bw = panel.BorderWidth;
            float hw = tf.Scale.x * 0.5f;
            float hh = tf.Scale.y * 0.5f;
            glm::vec3 pos = tf.Position;

            // Top
            Renderer2D::DrawQuad(
                glm::translate(glm::mat4(1.f), pos + glm::vec3(0.f, hh + bw * 0.5f, 0.001f))
                * glm::scale(glm::mat4(1.f), { tf.Scale.x + bw * 2.f, bw, 1.f }),
                panel.BorderColor);
            // Bottom
            Renderer2D::DrawQuad(
                glm::translate(glm::mat4(1.f), pos + glm::vec3(0.f, -(hh + bw * 0.5f), 0.001f))
                * glm::scale(glm::mat4(1.f), { tf.Scale.x + bw * 2.f, bw, 1.f }),
                panel.BorderColor);
            // Left
            Renderer2D::DrawQuad(
                glm::translate(glm::mat4(1.f), pos + glm::vec3(-(hw + bw * 0.5f), 0.f, 0.001f))
                * glm::scale(glm::mat4(1.f), { bw, tf.Scale.y, 1.f }),
                panel.BorderColor);
            // Right
            Renderer2D::DrawQuad(
                glm::translate(glm::mat4(1.f), pos + glm::vec3(hw + bw * 0.5f, 0.f, 0.001f))
                * glm::scale(glm::mat4(1.f), { bw, tf.Scale.y, 1.f }),
                panel.BorderColor);
        }
    }
}

static void RenderText(Scene* scene)
{
    auto view = scene->GetRegistry().view<TransformComponent, TextComponent>();
    for (auto handle : view)
    {
        auto [tf, text] = view.get<TransformComponent, TextComponent>(handle);
        if (!text.Visible || text.Text.empty())
            continue;

        Font::DrawTextMultiline(text.Text, tf.Position, text.FontSize,
                                text.LineSpacing, text.Color);
    }
}

static void RenderDialogue(Scene* scene)
{
    auto view = scene->GetRegistry().view<TransformComponent, DialogueComponent>();
    for (auto handle : view)
    {
        auto [tf, dlg] = view.get<TransformComponent, DialogueComponent>(handle);
        if (!dlg.Active || dlg.Lines.empty())
            continue;

        int lineIdx = dlg.CurrentLine;
        if (lineIdx < 0 || lineIdx >= (int)dlg.Lines.size())
            continue;

        const DialogueLine& line = dlg.Lines[lineIdx];

        // Box background
        float bw = tf.Scale.x;
        float bh = dlg.BoxHeight;
        glm::vec3 boxCenter = tf.Position;
        Renderer2D::DrawQuad(
            glm::translate(glm::mat4(1.f), boxCenter)
            * glm::scale(glm::mat4(1.f), { bw, bh, 1.f }),
            dlg.BoxColor);

        // Speaker name
        float pad = dlg.FontSize * 0.5f;
        if (!line.Speaker.empty())
        {
            glm::vec3 spkPos = boxCenter + glm::vec3(
                -bw * 0.5f + pad,
                 bh * 0.5f - dlg.FontSize * 1.5f,
                 0.01f);
            Font::DrawText(line.Speaker, spkPos, dlg.FontSize * 1.1f, dlg.SpeakerColor);
        }

        // Dialogue text with simple word-wrap
        int charsPerLine = (int)((bw - pad * 2.f) / dlg.FontSize);
        if (charsPerLine < 1) charsPerLine = 1;

        float topOffset = line.Speaker.empty() ? dlg.FontSize * 1.5f : dlg.FontSize * 3.0f;
        glm::vec3 textStart = boxCenter + glm::vec3(
            -bw * 0.5f + pad,
            bh * 0.5f - topOffset,
            0.01f);

        std::string remaining = line.Text;
        float yOff = 0.0f;
        while (!remaining.empty())
        {
            std::string chunk;
            if ((int)remaining.size() <= charsPerLine)
            {
                chunk = remaining;
                remaining.clear();
            }
            else
            {
                int breakAt = charsPerLine;
                for (int i = charsPerLine - 1; i >= 0; i--)
                {
                    if (remaining[(size_t)i] == ' ')
                    {
                        breakAt = i;
                        break;
                    }
                }
                chunk = remaining.substr(0, (size_t)breakAt);
                size_t skip = (size_t)breakAt + (remaining[(size_t)breakAt] == ' ' ? 1u : 0u);
                remaining = remaining.substr(skip);
            }

            glm::vec3 lp = textStart - glm::vec3(0.f, yOff, 0.f);
            Font::DrawText(chunk, lp, dlg.FontSize, dlg.TextColor);
            yOff += dlg.FontSize * 1.3f;

            if (yOff > bh - dlg.FontSize * 2.f)
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void SceneRenderer::RenderScene(Scene* scene, uint32_t viewportWidth, uint32_t viewportHeight)
{
    glm::mat4 viewProjection = glm::mat4(1.0f);
    bool foundCamera = false;

    auto cameraView = scene->GetRegistry().view<TransformComponent, CameraComponent>();
    for (auto entity : cameraView)
    {
        auto [transform, camera] = cameraView.get<TransformComponent, CameraComponent>(entity);
        if (camera.Primary)
        {
            float aspectRatio = (float)viewportWidth / (float)viewportHeight;
            viewProjection = camera.GetProjection(aspectRatio) * glm::inverse(transform.GetTransform());
            foundCamera = true;
            break;
        }
    }

    if (!foundCamera)
    {
        float ar = (float)viewportWidth / (float)viewportHeight;
        float s  = 10.0f;
        viewProjection = glm::ortho(-s * ar * 0.5f, s * ar * 0.5f, -s * 0.5f, s * 0.5f, -1.f, 1.f);
    }

    RenderScene(scene, viewProjection);
}

void SceneRenderer::RenderScene(Scene* scene, const glm::mat4& viewProjection)
{
    Renderer2D::BeginScene(viewProjection);

    // 1. Comic panels (behind everything else)
    RenderPanels(scene);

    // 2. Sprites
    auto spriteView = scene->GetRegistry().view<TransformComponent, SpriteRendererComponent>();
    for (auto entity : spriteView)
    {
        auto [transform, sprite] = spriteView.get<TransformComponent, SpriteRendererComponent>(entity);
        if (sprite.Texture)
            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Texture, sprite.Color);
        else
            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
    }

    // 3. World-space text labels
    RenderText(scene);

    // 4. Dialogue boxes (on top)
    RenderDialogue(scene);

    Renderer2D::EndScene();
}

} // namespace Engine
