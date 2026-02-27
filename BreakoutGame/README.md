# Breakout Game

A classic Breakout/Arkanoid game built with the custom game engine!

## About

This is a fully playable Breakout clone that demonstrates all the engine's capabilities:
- **ECS Architecture**: Entities for paddle, ball, bricks, and walls
- **Physics System**: Box2D integration for collision and movement
- **Rendering**: Batch rendering with colored sprites
- **Input System**: Keyboard controls for paddle movement
- **Game Logic**: Win/lose conditions, brick destruction, ball physics

## How to Play

### Building
```bash
cd build
make BreakoutGame
```

### Running
```bash
./bin/BreakoutGame
```

### Controls
- **LEFT/RIGHT ARROW**: Move the paddle
- **SPACE**: Launch the ball (at start) or restart game (after win/lose)

### Gameplay
1. Use the paddle to keep the ball in play
2. Break all 50 bricks (5 rows x 10 columns) to win
3. If the ball falls off the bottom, you lose
4. Press SPACE to restart and try again!

### Features
- **50 Colorful Bricks**: 5 different colors in rows (Red, Orange, Yellow, Green, Blue)
- **Physics-Based Ball**: Realistic bouncing with perfect restitution
- **Smooth Paddle Movement**: Kinematic body controlled by keyboard
- **Collision Detection**: Box2D handles all physics collisions
- **Game States**: Playing, Won, Lost with restart functionality
- **No Gravity**: Physics world configured for top-down gameplay
- **Perfect Bounces**: Ball maintains constant speed (restitution = 1.0)

## Technical Details

### Game Objects

**Paddle** (Green)
- Kinematic rigidbody (player-controlled)
- Box collider
- Clamped movement to stay within bounds

**Ball** (White)
- Dynamic rigidbody with physics simulation
- Circle collider for smooth bouncing
- No gravity, no damping (constant speed)
- Perfect restitution for predictable bounces

**Bricks** (5 colors)
- Static rigidbodies
- Box colliders
- Destroyed on collision with ball

**Walls**
- Static rigidbodies (left, right, top)
- Contain the play area
- No bottom wall (death zone)

### Game Logic
- Brick collision detection using AABB (Axis-Aligned Bounding Box)
- Ball velocity reflection on brick hit
- Win condition: All bricks destroyed
- Lose condition: Ball falls below bottom boundary
- Restart: Recreates entire scene from scratch

## Code Structure

```
BreakoutGame/
├── CMakeLists.txt          # Build configuration
├── src/
│   └── BreakoutGame.cpp    # Main game application
└── README.md               # This file
```

### Main Components

1. **OnInit()**: Creates scene, physics world, and initial game state
2. **CreateGame()**: Spawns all game entities (walls, paddle, ball, bricks)
3. **OnUpdate()**: Handles input, physics, collision detection, game state
4. **OnRender()**: Renders the scene using SceneRenderer
5. **HandlePaddleMovement()**: Moves paddle based on arrow keys
6. **HandleBallLaunch()**: Launches ball with initial velocity
7. **CheckBrickCollisions()**: Detects and handles brick destruction
8. **CheckBallOutOfBounds()**: Game over detection
9. **CheckWinCondition()**: Victory detection

## What This Demonstrates

This game showcases:
- ✅ Complete game loop (init, update, render)
- ✅ ECS entity management (creation, destruction, components)
- ✅ Physics integration (Box2D with custom velocities)
- ✅ Input handling (keyboard state queries)
- ✅ Collision detection (both Box2D physics and manual AABB)
- ✅ Game state management (playing, won, lost)
- ✅ Scene restarting (cleanup and recreation)
- ✅ Batch rendering (50+ entities rendered efficiently)

## Future Enhancements

Potential improvements:
- Power-ups (multi-ball, bigger paddle, etc.)
- Score tracking
- Multiple levels with different brick layouts
- Sound effects (when audio system is added)
- Particle effects on brick destruction
- Ball speed increase over time
- Lives system (multiple chances)
- High score persistence (save to file)

## Have Fun!

This demonstrates that the engine is fully capable of creating real, playable games. Enjoy breaking those bricks!
