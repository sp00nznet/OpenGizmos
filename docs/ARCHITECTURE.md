# Architecture Overview

OpenGizmos is structured as a modular game engine reimplementing Super Solvers: Gizmos & Gadgets.

## System Layers

```
┌─────────────────────────────────────────┐
│              Game States                │
│    (Title, Menu, Gameplay, Puzzle)      │
├─────────────────────────────────────────┤
│              Game Logic                 │
│  (Player, Entities, Rooms, Puzzles)     │
├─────────────────────────────────────────┤
│              Engine Layer               │
│   (Renderer, Audio, Input, GameLoop)    │
├─────────────────────────────────────────┤
│             Asset Loader                │
│  (NE Extractor, GRP Decoder, Cache)     │
├─────────────────────────────────────────┤
│               SDL2                      │
│    (Window, Graphics, Audio, Events)    │
└─────────────────────────────────────────┘
```

## Component Overview

### Loader Module (`src/loader/`)

Responsible for reading original game files and converting them to usable formats.

| Component | Purpose |
|-----------|---------|
| `NEResourceExtractor` | Extracts resources from NE DLL files (.DAT) |
| `GrpArchive` | Decodes RGrp sprite archives (.GRP) |
| `SmackerPlayer` | Decodes Smacker video files (.SMK) |
| `AssetCache` | Caches converted assets for fast access |

### Engine Module (`src/engine/`)

Core game engine components built on SDL2.

| Component | Purpose |
|-----------|---------|
| `Renderer` | SDL2 rendering with 640x480 logical resolution |
| `AudioSystem` | SDL_mixer audio with positional sound |
| `InputSystem` | Keyboard/mouse input with action mapping |
| `Game` | Main game loop and state management |

### Game Module (`src/game/`)

Game-specific logic and mechanics.

| Component | Purpose |
|-----------|---------|
| `Entity` | Base class for all game objects |
| `Player` | Player character with physics and animation |
| `Room` | Single room/screen with entities and collision |
| `Area` | Floor containing multiple rooms |
| `GameBuilding` | Building (Easy/Medium/Hard) with 5 floors |
| `Puzzle` | Base class for 8 puzzle minigame types |

## Data Flow

### Asset Loading
```
Original File → Loader → Raw Data → Converter → Cache → Game Use
     │              │          │          │         │
   .DAT         NE Parser   Bitmap    BMP/PNG   Texture
   .GRP         GRP Parser  Sprite    RGBA      Sprite
   .SMK         SMK Parser  Frames    RGB       Video
```

### Game Loop
```
┌─────────────────────────────────────┐
│                                     │
│  ┌─────────┐                        │
│  │ Process │ ← SDL Events           │
│  │  Input  │                        │
│  └────┬────┘                        │
│       ↓                             │
│  ┌─────────┐                        │
│  │  Update │ ← Delta Time           │
│  │  State  │                        │
│  └────┬────┘                        │
│       ↓                             │
│  ┌─────────┐                        │
│  │  Render │ → Frame Buffer         │
│  │  Frame  │                        │
│  └────┬────┘                        │
│       ↓                             │
│  ┌─────────┐                        │
│  │ Present │ → Display              │
│  └────┬────┘                        │
│       │                             │
└───────┴─────────────────────────────┘
```

## Entity System

Entities are the building blocks of game objects:

```
Entity (base)
├── Player
├── PartEntity (collectibles)
├── DoorEntity (room transitions)
├── LadderEntity (climbing)
├── PlatformEntity (moving platforms)
├── ObstacleEntity (hazards)
└── TriggerEntity (invisible zones)
```

Each entity has:
- Position (x, y) and velocity
- Collision bounds
- Sprite/animation
- State flags (active, visible, solid)

## Puzzle System

8 puzzle types following the original game:

| Type | Description |
|------|-------------|
| Balance | Weight scales to balance |
| Electricity | Complete circuit paths |
| Energy | Transfer energy between nodes |
| Force | Apply forces to move objects |
| Gear | Connect gears for rotation |
| Jigsaw | Arrange picture pieces |
| Magnet | Use magnetic attraction/repulsion |
| SimpleMachine | Levers and pulleys |

## State Machine

Game states control flow:

```
         ┌──────────┐
         │  Title   │
         └────┬─────┘
              ↓
         ┌──────────┐
    ┌────│   Menu   │────┐
    │    └────┬─────┘    │
    │         ↓          │
    │    ┌──────────┐    │
    │    │ Gameplay │    │
    │    └────┬─────┘    │
    │         ↓          │
    │    ┌──────────┐    │
    │    │  Puzzle  │    │
    │    └──────────┘    │
    │                    │
    └────────────────────┘
```

## Threading Model

Single-threaded design for simplicity:
- Main thread handles all game logic
- SDL handles audio mixing internally
- Asset loading is synchronous (with caching)

## Memory Management

- Smart pointers (`unique_ptr`, `shared_ptr`) for ownership
- Entity lifetime managed by Room
- Asset cache uses reference counting for textures
