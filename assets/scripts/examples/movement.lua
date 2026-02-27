-- movement.lua
-- Makes an entity move with WASD (or arrow) keys.
-- Attach to any entity that has a TransformComponent.
--
-- Controls:
--   W / Up    -> move up
--   S / Down  -> move down
--   A / Left  -> move left
--   D / Right -> move right

local speed = 5.0   -- world units per second

function OnStart()
    Log("Movement script ready! Use WASD or arrow keys.")
end

function OnUpdate(dt)
    local x, y, z = GetPosition()

    if Input.IsKeyHeld("D") or Input.IsKeyHeld("Right") then
        x = x + speed * dt
    end
    if Input.IsKeyHeld("A") or Input.IsKeyHeld("Left") then
        x = x - speed * dt
    end
    if Input.IsKeyHeld("W") or Input.IsKeyHeld("Up") then
        y = y + speed * dt
    end
    if Input.IsKeyHeld("S") or Input.IsKeyHeld("Down") then
        y = y - speed * dt
    end

    SetPosition(x, y, z)
end
