-- Template script
-- Copy this file and rename it for your entity.
--
-- Available functions:
--   GetPosition()  -> x, y, z          SetPosition(x, y, z)
--   GetScale()     -> x, y, z          SetScale(x, y, z)
--   GetRotation()  -> z (radians)      SetRotation(z)
--   GetColor()     -> r, g, b, a       SetColor(r, g, b, a)
--   GetVelocity()  -> x, y             SetVelocity(x, y)
--   ApplyForce(x, y)                   ApplyImpulse(x, y)
--   Log(message)
--
-- Input:
--   Input.IsKeyHeld("W")     -- held every frame
--   Input.IsKeyPressed("W")  -- true only on the first frame
--   Input.IsKeyReleased("W") -- true only when released
--   Input.GetMouseX() / Input.GetMouseY()
--
-- Key names: A-Z, Up, Down, Left, Right, Space, Escape, Enter,
--            Shift, Ctrl, Alt, 0-9

function OnStart()
    -- Called once when the scene starts playing
end

function OnUpdate(dt)
    -- Called every frame; dt = time since last frame in seconds
end
