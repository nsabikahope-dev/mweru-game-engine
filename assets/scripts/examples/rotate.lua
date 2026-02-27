-- rotate.lua
-- Continuously rotates an entity.
-- Attach to any entity that has a TransformComponent.

local rotSpeed = 2.0   -- radians per second

function OnStart()
    Log("Rotate script started!")
end

function OnUpdate(dt)
    local angle = GetRotation()
    SetRotation(angle + rotSpeed * dt)
end
