-- health_bar.lua
-- Tracks an entity's health and updates a HUD text display.
-- Attach to the player entity.
--
-- Scene setup:
--   "HealthText"  - TextComponent (e.g. "HP: 100")
--   "HealthBar"   - SpriteRendererComponent acting as a coloured bar
--                   Scale the bar's X proportionally to current health.
--
-- Call TakeDamage() and Heal() from other scripts via Scene API
-- or expand this script to react to collisions.

local maxHP  = 100
local hp     = 100

local function Refresh()
    Scene.SetText("HealthText", "HP: " .. hp)

    -- Colour: green -> yellow -> red as health drops
    local ratio = hp / maxHP
    local r = 1.0 - ratio        -- 0 at full, 1 at zero
    local g = ratio              -- 1 at full, 0 at zero
    Scene.SetColor("HealthText", r, g, 0.0, 1.0)

    -- Resize the health bar entity (assumes full-health width = 5.0)
    if Scene.Exists("HealthBar") then
        local _, y, z = Scene.GetPosition("HealthBar")
        -- We can't set scale via Scene API yet, but we can hide it at 0
        if hp <= 0 then
            Scene.SetVisible("HealthBar", false)
        else
            Scene.SetVisible("HealthBar", true)
        end
    end
end

function TakeDamage(amount)
    hp = math.max(0, hp - amount)
    Refresh()
    if hp <= 0 then
        Log("Player died!")
        Scene.SetText("HealthText", "DEAD")
    end
end

function Heal(amount)
    hp = math.min(maxHP, hp + amount)
    Refresh()
end

function OnStart()
    Refresh()
    Log("Health system ready. Max HP: " .. maxHP)
end

function OnUpdate(dt)
    -- Example: press H to take 10 damage, press J to heal 10
    if Input.IsKeyPressed("H") then TakeDamage(10) end
    if Input.IsKeyPressed("J") then Heal(10)       end
end
