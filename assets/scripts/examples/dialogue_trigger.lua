-- dialogue_trigger.lua
-- Starts a dialogue sequence when the player walks into a trigger zone.
--
-- Setup:
--   1. Create an entity named "NPC" with a DialogueComponent (fill in lines in the Editor).
--   2. Create an invisible trigger entity (no sprite, BoxCollider set to IsSensor=true).
--   3. Attach this script to the trigger entity.
--   4. (Optional) Create a text entity named "PressE" showing "[E] Talk".
--
-- Controls:
--   E  -> interact when the player is inside the trigger area

local triggered  = false   -- is the player currently in range?
local RANGE      = 2.0     -- distance in world units to activate

function OnStart()
    Log("Dialogue trigger ready.")
    Scene.SetVisible("PressE", false)
end

function OnUpdate(dt)
    -- Check distance from this entity to the player
    local tx, ty, _ = GetPosition()

    if not Scene.Exists("Player") then return end

    local px, py, _ = Scene.GetPosition("Player")
    local dist = math.sqrt((tx - px)^2 + (ty - py)^2)

    if dist < RANGE then
        if not triggered then
            triggered = true
            Scene.SetVisible("PressE", true)
        end

        if Input.IsKeyPressed("E") then
            -- Hide the prompt and start the dialogue
            Scene.SetVisible("PressE", false)
            triggered = false
            -- DialogueComponent becomes Active = true via Scene API
            -- (requires a SetDialogueActive binding or direct scene call)
            Log("Dialogue started with NPC!")
        end
    else
        if triggered then
            triggered = false
            Scene.SetVisible("PressE", false)
        end
    end
end
