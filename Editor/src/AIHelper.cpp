#include "../include/AIHelper.h"

#include <json.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using json = nlohmann::json;

// -----------------------------------------------------------------------
// Static member
// -----------------------------------------------------------------------
std::string AIHelper::s_LastMethod;

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static bool Contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

// -----------------------------------------------------------------------
// Engine API documentation sent to Claude as system context
// -----------------------------------------------------------------------
static const char* k_EngineAPIDoc = R"(
You are a Lua script generator for a 2D game engine. Generate ONLY a Lua script (no explanation, no markdown fences).

Available engine API (all are global functions):

-- Transform
GetPosition()          -> x, y, z (floats)
SetPosition(x, y, z)
GetScale()             -> x, y, z
SetScale(x, y, z)
GetRotation()          -> z (radians)
SetRotation(z)

-- Sprite colour
GetColor()             -> r, g, b, a (0.0-1.0)
SetColor(r, g, b, a)

-- Physics (only when entity has Rigidbody)
GetVelocity()          -> x, y
SetVelocity(x, y)
ApplyForce(x, y)
ApplyImpulse(x, y)

-- Input (key names: A-Z, Up, Down, Left, Right, Space, Escape, Enter, Shift, Ctrl, 0-9)
Input.IsKeyHeld("W")        -- true every frame while held
Input.IsKeyPressed("Space") -- true only on first press frame
Input.IsKeyReleased("A")
Input.GetMouseX()           -- screen pixels
Input.GetMouseY()

-- Utility
Log(message)

Script callbacks (define these functions):
  function OnStart()        -- called once when scene starts playing
  function OnUpdate(dt)     -- called every frame; dt = seconds since last frame

Keep scripts simple, well-commented, and beginner-friendly.
)";

// -----------------------------------------------------------------------
// Template library
// -----------------------------------------------------------------------
static const char* k_TplMovement = R"(-- Movement script (WASD / arrow keys)
local speed = 5.0

function OnStart()
    Log("Movement ready! Use WASD or arrow keys.")
end

function OnUpdate(dt)
    local x, y, z = GetPosition()

    if Input.IsKeyHeld("D") or Input.IsKeyHeld("Right") then x = x + speed * dt end
    if Input.IsKeyHeld("A") or Input.IsKeyHeld("Left")  then x = x - speed * dt end
    if Input.IsKeyHeld("W") or Input.IsKeyHeld("Up")    then y = y + speed * dt end
    if Input.IsKeyHeld("S") or Input.IsKeyHeld("Down")  then y = y - speed * dt end

    SetPosition(x, y, z)
end
)";

static const char* k_TplJump = R"(-- Platformer movement with jumping
-- Requires: Rigidbody (Dynamic), enable Fixed Rotation for best results
local moveSpeed    = 5.0
local jumpStrength = 8.0

function OnStart()
    Log("Platformer ready! A/D to move, Space to jump.")
end

function OnUpdate(dt)
    local vx, vy = GetVelocity()

    if Input.IsKeyHeld("D") or Input.IsKeyHeld("Right") then
        SetVelocity(moveSpeed, vy)
    elseif Input.IsKeyHeld("A") or Input.IsKeyHeld("Left") then
        SetVelocity(-moveSpeed, vy)
    else
        SetVelocity(vx * 0.85, vy)  -- friction
    end

    -- Jump only when roughly on the ground
    if Input.IsKeyPressed("Space") and math.abs(vy) < 0.5 then
        ApplyImpulse(0, jumpStrength)
    end
end
)";

static const char* k_TplRotate = R"(-- Continuous rotation
local speed = 2.0   -- radians per second

function OnStart()
    Log("Rotate script started!")
end

function OnUpdate(dt)
    SetRotation(GetRotation() + speed * dt)
end
)";

static const char* k_TplRainbow = R"(-- Cycles through rainbow colours
local time  = 0.0
local speed = 1.0   -- cycles per second

function OnStart()
    Log("Rainbow script started!")
end

function OnUpdate(dt)
    time = time + dt * speed
    local r = math.sin(time)         * 0.5 + 0.5
    local g = math.sin(time + 2.094) * 0.5 + 0.5
    local b = math.sin(time + 4.189) * 0.5 + 0.5
    SetColor(r, g, b, 1.0)
end
)";

static const char* k_TplBounce = R"(-- Sine-wave bobbing up and down
local amplitude = 1.0   -- world units
local frequency = 2.0   -- oscillations per second
local time   = 0.0
local startY = nil

function OnStart()
    local _, y, _ = GetPosition()
    startY = y
    Log("Bounce script started!")
end

function OnUpdate(dt)
    time = time + dt
    local x, _, z = GetPosition()
    local newY = startY + math.sin(time * frequency * math.pi * 2) * amplitude
    SetPosition(x, newY, z)
end
)";

static const char* k_TplFollowMouse = R"(-- Smoothly follows the mouse cursor
-- Mouse coords are in screen pixels; this converts to approximate world units.
-- Tune 'scale' if the entity overshoots or undershoots.
local smoothing = 6.0   -- higher = snappier
local scale     = 64.0  -- pixels per world unit (approx)

function OnStart()
    Log("Follow-mouse script started!")
end

function OnUpdate(dt)
    local mx = Input.GetMouseX()
    local my = Input.GetMouseY()

    -- Screen centre (approximate for 1280x720 viewport)
    local wx = (mx - 640) / scale
    local wy = -(my - 360) / scale

    local ex, ey, ez = GetPosition()
    local nx = ex + (wx - ex) * smoothing * dt
    local ny = ey + (wy - ey) * smoothing * dt
    SetPosition(nx, ny, ez)
end
)";

static const char* k_TplPulse = R"(-- Pulses the entity size up and down
local baseScale = 1.0
local amplitude = 0.3
local frequency = 2.0
local time = 0.0

function OnStart()
    Log("Pulse script started!")
end

function OnUpdate(dt)
    time = time + dt
    local s = baseScale + math.sin(time * frequency * math.pi * 2) * amplitude
    SetScale(s, s, 1.0)
end
)";

static const char* k_TplEmpty = R"(-- Custom script
-- Edit OnStart and OnUpdate below.

function OnStart()
    Log("Script started!")
end

function OnUpdate(dt)
    -- Your code here
end
)";

// -----------------------------------------------------------------------
// Template matching
// -----------------------------------------------------------------------
std::string AIHelper::GenerateFromTemplate(const std::string& description)
{
    std::string d = ToLower(description);

    // Ordered from most-specific to most-general
    if ((Contains(d, "follow") && Contains(d, "mouse")) ||
        Contains(d, "track mouse") || Contains(d, "cursor"))
        return k_TplFollowMouse;

    if (Contains(d, "jump") || Contains(d, "platform") || Contains(d, "platformer"))
        return k_TplJump;

    if ((Contains(d, "move") || Contains(d, "walk") || Contains(d, "run")) &&
        (Contains(d, "wasd") || Contains(d, "arrow") || Contains(d, "key")))
        return k_TplMovement;

    if (Contains(d, "move") || Contains(d, "walk") || Contains(d, "control"))
        return k_TplMovement;

    if (Contains(d, "rotate") || Contains(d, "spin") || Contains(d, "turn"))
        return k_TplRotate;

    if (Contains(d, "rainbow") || Contains(d, "colour") || Contains(d, "color"))
        return k_TplRainbow;

    if (Contains(d, "bounce") || Contains(d, "bob") || Contains(d, "oscillat") ||
        Contains(d, "float") || Contains(d, "hover"))
        return k_TplBounce;

    if (Contains(d, "pulse") || Contains(d, "throb") || Contains(d, "breath") ||
        Contains(d, "scale") || Contains(d, "size"))
        return k_TplPulse;

    return k_TplEmpty;
}

// -----------------------------------------------------------------------
// Claude API via curl CLI
// -----------------------------------------------------------------------
bool AIHelper::IsClaudeAvailable()
{
    const char* key = std::getenv("ANTHROPIC_API_KEY");
    return key != nullptr && key[0] != '\0';
}

std::string AIHelper::GenerateFromClaude(const std::string& description)
{
    const char* apiKey = std::getenv("ANTHROPIC_API_KEY");
    if (!apiKey || apiKey[0] == '\0')
        return "";

    // Build the JSON request body
    json requestBody = {
        {"model", "claude-haiku-4-5-20251001"},
        {"max_tokens", 1024},
        {"system", k_EngineAPIDoc},
        {"messages", json::array({
            {{"role", "user"}, {"content", "Generate a Lua script that does: " + description}}
        })}
    };

    // Write payload to a temp file (avoids shell injection from user input)
    const std::string payloadPath = "/tmp/engine_ai_payload.json";
    const std::string responsePath = "/tmp/engine_ai_response.json";

    {
        std::ofstream f(payloadPath);
        if (!f.is_open()) {
            std::cerr << "[AIHelper] Could not write payload temp file\n";
            return "";
        }
        f << requestBody.dump();
    }

    // Build and run curl command
    std::string cmd =
        "curl -s -X POST \"https://api.anthropic.com/v1/messages\""
        " -H \"x-api-key: " + std::string(apiKey) + "\""
        " -H \"anthropic-version: 2023-06-01\""
        " -H \"content-type: application/json\""
        " --data @" + payloadPath +
        " -o " + responsePath +
        " --max-time 15"
        " 2>/dev/null";

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "[AIHelper] curl command failed (exit " << ret << ")\n";
        return "";
    }

    // Read and parse response
    std::ifstream rf(responsePath);
    if (!rf.is_open()) {
        std::cerr << "[AIHelper] Could not read response file\n";
        return "";
    }

    std::string responseStr((std::istreambuf_iterator<char>(rf)),
                             std::istreambuf_iterator<char>());

    try {
        json response = json::parse(responseStr);

        // Check for API error
        if (response.contains("error")) {
            std::string errMsg = response["error"].value("message", "unknown error");
            std::cerr << "[AIHelper] Claude API error: " << errMsg << "\n";
            return "";
        }

        // Extract generated text
        if (response.contains("content") && !response["content"].empty()) {
            std::string text = response["content"][0].value("text", "");

            // Strip markdown code fences if Claude wrapped the code
            auto stripFences = [](std::string s) -> std::string {
                // Remove ```lua or ``` at start
                auto start = s.find("```");
                if (start != std::string::npos) {
                    auto newline = s.find('\n', start);
                    if (newline != std::string::npos)
                        s = s.substr(newline + 1);
                }
                // Remove trailing ```
                auto end = s.rfind("```");
                if (end != std::string::npos)
                    s = s.substr(0, end);
                return s;
            };

            return stripFences(text);
        }
    }
    catch (const json::exception& e) {
        std::cerr << "[AIHelper] Failed to parse Claude response: " << e.what() << "\n";
        std::cerr << "[AIHelper] Raw response: " << responseStr.substr(0, 200) << "\n";
    }

    return "";
}

// -----------------------------------------------------------------------
// Public entry point
// -----------------------------------------------------------------------
std::string AIHelper::GenerateScript(const std::string& description)
{
    if (description.empty())
        return "";

    // Try Claude first
    if (IsClaudeAvailable()) {
        std::cout << "[AIHelper] Calling Claude API...\n";
        std::string result = GenerateFromClaude(description);
        if (!result.empty()) {
            s_LastMethod = "Claude API";
            std::cout << "[AIHelper] Script generated via Claude API\n";
            return result;
        }
        std::cerr << "[AIHelper] Claude failed, falling back to templates\n";
    }

    // Fallback: templates
    std::string result = GenerateFromTemplate(description);
    s_LastMethod = "template";
    std::cout << "[AIHelper] Script generated via template matching\n";
    return result;
}

const std::string& AIHelper::GetLastMethod()
{
    return s_LastMethod;
}
