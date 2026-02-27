#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Engine {

class Texture2D;

/**
 * @brief Built-in 8x8 bitmap font for in-game text rendering.
 *
 * Uses a self-contained glyph atlas — no external font files required.
 * Covers printable ASCII (characters 32–126).
 *
 * Atlas layout: 16 columns × 8 rows of 8×8-pixel glyphs = 128×64 px total.
 *
 * Typical usage (inside a Renderer2D scene):
 *
 *   Font::Init();  // once at startup
 *
 *   Renderer2D::BeginScene(vp);
 *   Font::DrawText("Hello World", {-4, 0, 0}, 0.5f, {1, 1, 0, 1});
 *   Renderer2D::EndScene();
 *
 * Text is drawn as alpha-blended quads, so enable blending before calling
 * (Renderer2D::Init() enables it automatically).
 */
class Font
{
public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /** Build the glyph atlas texture. Call once after the OpenGL context is ready. */
    static void Init();

    /** Release the atlas texture. */
    static void Shutdown();

    // -----------------------------------------------------------------------
    // Drawing  (must be called between Renderer2D::BeginScene / EndScene)
    // -----------------------------------------------------------------------

    /**
     * Draw a single-line string.
     *
     * @param text      ASCII string to render (newlines are ignored here).
     * @param position  World-space position of the bottom-left corner of the
     *                  first character.
     * @param charSize  World units per character cell (width == height).
     * @param color     RGBA tint (default: opaque white).
     */
    static void DrawText(const std::string& text,
                         const glm::vec3&   position,
                         float              charSize,
                         const glm::vec4&   color = glm::vec4(1.0f));

    /**
     * Draw a multi-line string.  Lines are split on '\\n'.
     * Each line descends by @p lineSpacing * charSize world units.
     */
    static void DrawTextMultiline(const std::string& text,
                                  const glm::vec3&   position,
                                  float              charSize,
                                  float              lineSpacing = 1.2f,
                                  const glm::vec4&   color = glm::vec4(1.0f));

    // -----------------------------------------------------------------------
    // Metrics
    // -----------------------------------------------------------------------

    /** Total rendered width of @p text in world units. */
    static float GetTextWidth(const std::string& text, float charSize);

    // -----------------------------------------------------------------------
    // Atlas constants
    // -----------------------------------------------------------------------

    static constexpr int AtlasWidth  = 128; // pixels
    static constexpr int AtlasHeight = 64;  // pixels
    static constexpr int CharWidth   = 8;   // pixels per glyph
    static constexpr int CharHeight  = 8;
    static constexpr int CharsPerRow = 16;

    static std::shared_ptr<Texture2D> GetAtlas() { return s_Atlas; }

private:
    static std::shared_ptr<Texture2D> s_Atlas;
};

} // namespace Engine
