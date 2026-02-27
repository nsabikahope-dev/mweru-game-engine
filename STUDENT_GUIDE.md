# Student-Friendly Game Engine Guide

## Welcome Students! 🎮

This game engine was built specifically to help you learn game development! You can create 2D games using simple Lua scripts or even describe what you want in plain English.

## Quick Start for Students

### 1. Open the Editor
```bash
cd build
./bin/Editor
```

### 2. Create Your First Game Character

**Using the Visual Editor (Easiest!):**
1. Click "Create Entity" in Hierarchy panel
2. Name it "Player"
3. In Inspector, click "Add Component" → "Sprite Renderer"
4. Pick a color for your character
5. Click "Add Component" → "Script" (coming in Phase 11)

### 3. Make It Move with Simple Lua

**Option A: Write Simple Lua Code**
```lua
-- This makes the player move when you press arrow keys!
function OnUpdate(deltaTime)
    local speed = 5

    if Input.IsKeyHeld("Left") then
        transform.Position.x = transform.Position.x - speed * deltaTime
    end

    if Input.IsKeyHeld("Right") then
        transform.Position.x = transform.Position.x + speed * deltaTime
    end

    if Input.IsKeyHeld("Up") then
        transform.Position.y = transform.Position.y + speed * deltaTime
    end

    if Input.IsKeyHeld("Down") then
        transform.Position.y = transform.Position.y - speed * deltaTime
    end
end
```

**Option B: Use AI Helper (Super Easy!)**

Just describe what you want:
```
AI Helper: "Make the player move when I press arrow keys"
```

The AI generates the Lua code for you! Then you can:
- Click "Apply" to use it
- Click "Edit" to modify it
- Click "Learn" to see how it works

## What Can You Make?

### Beginner Projects
- **Moving Character**: Use arrow keys to move around
- **Color Changer**: Press space to change colors
- **Follow Mouse**: Make object follow your mouse
- **Bouncing Ball**: Make a ball bounce around the screen

### Intermediate Projects
- **Simple Platformer**: Jump and run game
- **Catch Game**: Catch falling objects
- **Maze Game**: Navigate through a maze
- **Breakout Clone**: Break bricks with a ball

### Advanced Projects
- **RPG**: With inventory and NPCs
- **Shooter**: Space invaders style
- **Puzzle Game**: Sokoban or match-3
- **Adventure Game**: Point-and-click style

## Available Commands in Lua

### Moving Things Around
```lua
-- Change position
transform.Position.x = 5
transform.Position.y = 3

-- Change rotation
transform.Rotation.z = 45  -- degrees

-- Change size
transform.Scale.x = 2  -- twice as big
transform.Scale.y = 2
```

### Checking Keyboard Input
```lua
if Input.IsKeyPressed("Space") then
    -- Fires once when key is first pressed
end

if Input.IsKeyHeld("W") then
    -- Fires every frame while key is held
end

if Input.IsKeyReleased("Escape") then
    -- Fires once when key is released
end
```

### Checking Mouse Input
```lua
if Input.IsMouseButtonHeld("Left") then
    -- Left mouse button is pressed
end

local mouseX, mouseY = Input.GetMousePosition()
-- Get mouse position on screen
```

### Changing Colors
```lua
-- Make sprite renderer red
sprite.Color.r = 1.0
sprite.Color.g = 0.0
sprite.Color.b = 0.0
sprite.Color.a = 1.0  -- alpha (transparency)
```

### Using Physics
```lua
-- Add velocity (needs Rigidbody component)
rigidbody.Velocity.x = 10
rigidbody.Velocity.y = 5

-- Jump!
rigidbody.Velocity.y = 15
```

### Time and Delta Time
```lua
function OnUpdate(deltaTime)
    -- deltaTime = time since last frame
    -- Use it to make movement smooth!

    local speed = 5
    transform.Position.x = transform.Position.x + speed * deltaTime
    -- This moves 5 units per second (smooth on any computer!)
end
```

## Example Scripts for Students

### 1. Simple Movement
```lua
-- File: PlayerMovement.lua
function OnUpdate(deltaTime)
    local speed = 5

    if Input.IsKeyHeld("A") then
        transform.Position.x = transform.Position.x - speed * deltaTime
    end
    if Input.IsKeyHeld("D") then
        transform.Position.x = transform.Position.x + speed * deltaTime
    end
    if Input.IsKeyHeld("W") then
        transform.Position.y = transform.Position.y + speed * deltaTime
    end
    if Input.IsKeyHeld("S") then
        transform.Position.y = transform.Position.y - speed * deltaTime
    end
end
```

### 2. Jump Mechanic
```lua
-- File: Jump.lua
local isOnGround = true

function OnUpdate(deltaTime)
    if Input.IsKeyPressed("Space") and isOnGround then
        rigidbody.Velocity.y = 10  -- Jump!
        isOnGround = false
    end
end

function OnCollision(other)
    if other.Tag == "Ground" then
        isOnGround = true
    end
end
```

### 3. Follow Mouse
```lua
-- File: FollowMouse.lua
function OnUpdate(deltaTime)
    local mouseX, mouseY = Input.GetMousePosition()
    local speed = 3

    -- Move towards mouse position
    transform.Position.x = transform.Position.x + (mouseX - transform.Position.x) * speed * deltaTime
    transform.Position.y = transform.Position.y + (mouseY - transform.Position.y) * speed * deltaTime
end
```

### 4. Color Rainbow
```lua
-- File: RainbowColor.lua
local time = 0

function OnUpdate(deltaTime)
    time = time + deltaTime

    -- Cycle through rainbow colors
    sprite.Color.r = math.sin(time) * 0.5 + 0.5
    sprite.Color.g = math.sin(time + 2) * 0.5 + 0.5
    sprite.Color.b = math.sin(time + 4) * 0.5 + 0.5
end
```

### 5. Shoot Bullets
```lua
-- File: Shooting.lua
function OnUpdate(deltaTime)
    if Input.IsKeyPressed("Space") then
        -- Create a bullet entity
        local bullet = Scene.CreateEntity("Bullet")
        bullet.transform.Position.x = transform.Position.x
        bullet.transform.Position.y = transform.Position.y

        -- Add velocity to bullet
        bullet.rigidbody.Velocity.y = 20
    end
end
```

## Using the AI Helper

The AI Helper can generate scripts for you! Here's how:

### Step 1: Describe What You Want
Click the "AI Helper" button in the Script component and type:
```
"Make the player jump when I press spacebar"
```

### Step 2: AI Generates Code
The AI will create Lua code like:
```lua
function OnUpdate(deltaTime)
    if Input.IsKeyPressed("Space") then
        rigidbody.Velocity.y = 10
    end
end
```

### Step 3: Learn From It!
- Click "Explain" to understand how it works
- Click "Modify" to add your own ideas
- Click "Apply" to use it in your game

## Common Student Questions

**Q: "My character moves too fast!"**
A: Reduce the `speed` variable in your script.

**Q: "Nothing happens when I press keys!"**
A: Make sure:
1. The script is attached to the entity
2. The game is playing (not paused)
3. You spelled the key name correctly (case-sensitive!)

**Q: "I get an error about 'rigidbody'!"**
A: You need to add a Rigidbody component to your entity first!
(Add Component → Rigidbody)

**Q: "How do I make enemies?"**
A: Create a new entity, add a script, and make it move on its own:
```lua
function OnUpdate(deltaTime)
    transform.Position.x = transform.Position.x - 2 * deltaTime
    -- Enemy moves left automatically!
end
```

**Q: "Can I make multiplayer games?"**
A: Not yet! But you can make 2-player games on one computer:
- Player 1 uses WASD
- Player 2 uses Arrow Keys

## Tips for Success

1. **Start Small**: Make one thing work before adding more
2. **Save Often**: File → Save Scene (Ctrl+S)
3. **Test Frequently**: Click Play button to test your game
4. **Ask AI**: Use the AI Helper when stuck
5. **Read Errors**: Error messages tell you what's wrong
6. **Experiment**: Try changing numbers to see what happens!

## Debugging Your Scripts

### Print to Console
```lua
function OnUpdate(deltaTime)
    print("Position: " .. transform.Position.x)
    -- This shows values in the console!
end
```

### Common Errors
```lua
-- ❌ WRONG:
transform.position.x = 5  -- lowercase 'p'

-- ✅ CORRECT:
transform.Position.x = 5  -- uppercase 'P'
```

## Next Steps

1. Try the example scripts above
2. Modify them to do something different
3. Combine multiple scripts on one entity
4. Create your first complete game!
5. Share it with friends!

## Need Help?

- Use the AI Helper for script generation
- Check the example scripts
- Ask your teacher
- Look at the error messages (they help!)

**Happy Game Making! 🎮✨**
