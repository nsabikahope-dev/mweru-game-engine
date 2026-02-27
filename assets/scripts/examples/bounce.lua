-- bounce.lua
-- Makes an entity bob up and down using a sine wave.
-- Attach to any entity that has a TransformComponent.

local amplitude = 1.0   -- how far it moves (world units)
local frequency = 2.0   -- oscillations per second

local time    = 0.0
local startY  = nil     -- set once in OnStart

function OnStart()
    local _, y, _ = GetPosition()
    startY = y
    Log("Bounce script started!")
end

function OnUpdate(dt)
    time = time + dt

    local _, _, z = GetPosition()
    local x, _, _ = GetPosition()   -- keep current x
    local newY = startY + math.sin(time * frequency * math.pi * 2) * amplitude

    SetPosition(x, newY, z)
end
