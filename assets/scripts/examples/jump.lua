-- jump.lua
-- Side-scroller movement with jumping.
-- Requires a Rigidbody (Dynamic) and a collider.
-- Tip: enable "Fixed Rotation" on the Rigidbody for best results.
--
-- Controls:
--   A / Left  -> move left
--   D / Right -> move right
--   Space     -> jump (only when near the ground)

local moveSpeed = 5.0
local jumpStrength = 8.0

function OnStart()
    Log("Jump script ready! A/D to move, Space to jump.")
end

function OnUpdate(dt)
    local vx, vy = GetVelocity()

    -- Horizontal movement
    if Input.IsKeyHeld("D") or Input.IsKeyHeld("Right") then
        SetVelocity(moveSpeed, vy)
    elseif Input.IsKeyHeld("A") or Input.IsKeyHeld("Left") then
        SetVelocity(-moveSpeed, vy)
    else
        -- Light friction when no key is held
        SetVelocity(vx * 0.85, vy)
    end

    -- Jump: only allow when the entity is roughly on the ground
    -- (vertical velocity close to zero means standing still vertically)
    if Input.IsKeyPressed("Space") and math.abs(vy) < 0.5 then
        ApplyImpulse(0, jumpStrength)
    end
end
