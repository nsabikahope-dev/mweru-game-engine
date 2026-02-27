#pragma once

#include <string>

/**
 * @brief AI-powered Lua script generator
 *
 * Converts plain-English descriptions into Lua scripts for the engine.
 *
 * Two modes (automatic priority):
 *   1. Claude API — if ANTHROPIC_API_KEY env var is set. Uses claude-haiku for
 *      fast, context-aware generation with the engine's full API documented.
 *   2. Template matching — fast offline fallback, recognises common patterns:
 *      "move / wasd / arrow"  -> WASD movement
 *      "jump / platform"      -> physics jump
 *      "rotate / spin"        -> continuous rotation
 *      "rainbow / color cycle"-> colour animation
 *      "bounce / bob"         -> sine-wave bobbing
 *      "follow mouse"         -> mouse tracking
 *
 * Usage:
 *   std::string code = AIHelper::GenerateScript("make the player move with arrow keys");
 *   // Writes code to path, returns empty string on total failure
 */
class AIHelper
{
public:
    /**
     * @brief Generate a script from a plain-English description.
     * Tries Claude API first; falls back to template matching.
     * @return Lua source code, or empty string on failure.
     */
    static std::string GenerateScript(const std::string& description);

    /**
     * @brief Pattern-match the description and return a Lua template.
     * Always succeeds (returns a minimal script if nothing matches).
     */
    static std::string GenerateFromTemplate(const std::string& description);

    /**
     * @brief Call the Claude API with the description.
     * @return Generated Lua source, or empty string if unavailable / error.
     */
    static std::string GenerateFromClaude(const std::string& description);

    /** @brief True when ANTHROPIC_API_KEY is set in the environment. */
    static bool IsClaudeAvailable();

    /** @brief Human-readable label for the last method used ("Claude API" / "template"). */
    static const std::string& GetLastMethod();

private:
    static std::string s_LastMethod;
};
