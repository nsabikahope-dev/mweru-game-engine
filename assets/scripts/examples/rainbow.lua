-- rainbow.lua
-- Cycles through rainbow colours over time.
-- Attach to any entity that has a SpriteRendererComponent.

local time  = 0.0
local speed = 1.0   -- full cycles per second

function OnStart()
    Log("Rainbow script started!")
end

function OnUpdate(dt)
    time = time + dt * speed

    -- Offset each channel by 120 degrees (2*pi/3 ≈ 2.094)
    local r = math.sin(time)         * 0.5 + 0.5
    local g = math.sin(time + 2.094) * 0.5 + 0.5
    local b = math.sin(time + 4.189) * 0.5 + 0.5

    SetColor(r, g, b, 1.0)
end
