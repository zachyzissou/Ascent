# Claude Code Configuration for Ascent

## Project Overview
**Working Title**: Ascent

Peak-inspired climbing game with advanced tool mechanics including:
- Physics-based climbing with stamina management
- Tool-oriented gameplay (anchors, ropes, grappling hooks, pulleys)
- Cooperative multiplayer support
- Dynamic rope physics and cable systems

## Engine & Tech Stack
- **Engine**: Unreal Engine 5.6
- **Platform**: Windows/Linux/Mac
- **Networking**: Built-in Unreal multiplayer framework
- **Physics**: Chaos Physics with CableComponent plugin

## Key Development Commands
```bash
# Generate project files (when adding C++ classes)
UnrealBuildTool -projectfiles -project="ClimbingGame.uproject" -game -rocket -progress

# Build project
UnrealBuildTool ClimbingGame Win64 Development -Project="ClimbingGame.uproject"

# Package for distribution
# Use Unreal Editor: File > Package Project > Windows (64-bit)
```

## Core Systems to Implement
1. **Player Movement**: Physics-based climbing controller
2. **Tool System**: Inventory, durability, and interaction framework  
3. **Rope Physics**: Dynamic cable simulation with anchors
4. **Stamina & Injury**: Resource management affecting movement
5. **Multiplayer**: Co-op assistance and proximity chat
6. **Environment**: Procedural/handcrafted climbing routes

## Project Structure
```
ClimbingGame/
├── Content/
│   ├── Blueprints/
│   │   ├── Player/
│   │   ├── Tools/
│   │   └── Environment/
│   ├── Materials/
│   ├── Meshes/
│   └── Maps/
├── Source/ClimbingGame/
│   ├── Player/
│   ├── Tools/
│   ├── Physics/
│   └── Multiplayer/
└── Config/
```

## Development Notes
- Focus on tool interactions and realistic physics
- Emphasize cooperative gameplay mechanics
- Design for replayability with procedural elements
- Maintain performance with complex rope/cable physics