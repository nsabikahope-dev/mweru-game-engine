-- camera_follow.lua
-- Smoothly follows a target entity.
-- Attach this script to your Camera entity.
--
-- Scene setup:
--   Set TARGET_NAME below to the tag of the entity to follow.

local TARGET_NAME = "Player"
local SMOOTH      = 5.0     -- lerp speed (higher = snappier)
local OFFSET_X    = 0.0     -- horizontal offset from target
local OFFSET_Y    = 1.0     -- vertical offset (look slightly ahead)

function OnUpdate(dt)
    if not Scene.Exists(TARGET_NAME) then return end

    local tx, ty, _ = Scene.GetPosition(TARGET_NAME)
    local cx, cy, cz = GetPosition()

    -- Lerp camera toward the target position
    local newX = cx + (tx + OFFSET_X - cx) * SMOOTH * dt
    local newY = cy + (ty + OFFSET_Y - cy) * SMOOTH * dt

    SetPosition(newX, newY, cz)
end
