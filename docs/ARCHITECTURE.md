# Architecture

OpenGG is a modular engine for running multiple TLC educational games through a shared infrastructure.

---

## System Layers

```
+---------------------------------------------+
|            Game-Specific States              |
|  (Neptune, Gizmos, Labyrinth, Puzzles)       |
+---------------------------------------------+
|            Multi-Game Launcher               |
|  (GameSelection, GameLaunch, AssetViewer)    |
+---------------------------------------------+
|            Game Logic Layer                  |
|  (Player, Entities, Rooms, Puzzles)          |
+---------------------------------------------+
|            Engine Layer                      |
|  (Renderer, Audio, Input, GameLoop, Menu)    |
+---------------------------------------------+
|            Asset Layer                       |
|  (GameRegistry, AssetCache, NE/GRP Loaders) |
+---------------------------------------------+
|            SDL2                              |
|  (Window, Graphics, Audio, Events)           |
+---------------------------------------------+
```

---

## State Machine

The engine uses a stack-based state machine. States can be pushed (overlay), popped (return), or changed (replace).

### State Flow

```
TitleState
    |
    v
GameSelectionState        <-- grid of all available games
    |
    v
GameLaunchState           <-- per-game info panel with options
    |
    +---> AssetViewerState    (browse sprites, sounds, MIDI)
    |
    +---> NeptuneGameState    (Operation Neptune gameplay)
    |         |
    |         +---> LabyrinthGameState
    |         +---> SortingPuzzleState
    |         +---> ReaderPuzzleState
    |         +---> MathPuzzleState
    |
    +---> GameplayState       (Gizmos & Gadgets -- stub)
              |
              +---> PuzzleState (Balance, Gear, etc.)
```

---

## Key Components

### Game Registry (`game_registry.h`)

Discovers and catalogs all available games.

- Parses `extracted/all_games_manifest.json` (custom parser, no JSON library)
- Stores `GameInfo` structs with asset counts, paths, availability flags
- Searches multiple paths on startup: `C:/ggng/extracted`, `../extracted`, `./extracted`
- Provides typed accessors: `getSpritePath()`, `getWavPath()`, `getMidiPath()`

### Asset Cache (`asset_cache.h`)

Unified cache for both legacy and extracted assets.

**Legacy pipeline** (original game files):
```
.DAT file --> NE Parser --> Raw bitmap --> RLE decode --> SDL_Texture
.GRP file --> GRP Parser --> Sprite data --> RGBA --> SDL_Texture
```

**Extracted pipeline** (pre-extracted assets):
```
extracted/<gameId>/sprites/*.bmp  --> SDL_LoadBMP --> SDL_Texture
extracted/<gameId>/audio/wav/*.wav --> Mix_LoadWAV --> Mix_Chunk
extracted/<gameId>/audio/midi/*.mid --> Mix_LoadMUS --> Mix_Music
```

The extracted pipeline is preferred when available. Cache keys use the format `extracted:<gameId>:sprite:<name>` to avoid collisions.

### Engine Components (`src/engine/`)

| Component | File | Purpose |
|-----------|------|---------|
| `Game` | `game_loop.cpp` | Main loop, state stack, config, GameRegistry owner |
| `Renderer` | `renderer.cpp` | SDL2 rendering, 640x480 logical resolution, scaling |
| `AudioSystem` | `audio.cpp` | SDL_mixer audio, positional sound |
| `InputSystem` | `input.cpp` | Keyboard/mouse with action mapping |
| `MenuBar` | `menu.cpp` | Win32 native menu bar (File, Config, Debug, About) |
| `BitmapFont` | `font.cpp` | Bitmap font rendering from sprite sheets |

### Game Logic (`src/game/`)

| Component | Purpose |
|-----------|---------|
| `Entity` | Base class for game objects (position, velocity, collision, sprite) |
| `Player` | Player character with physics and animation |
| `Room` | Single screen with entities, collision bounds, exits |
| `Puzzle` | Base class for minigame types |
| `GameRegistry` | Multi-game discovery and metadata |

### Neptune Module (`src/neptune/`)

Complete game-specific implementation for Operation Neptune:

- `NeptuneGameState` -- main submarine navigation with oxygen/fuel management
- `LabyrinthGameState` -- maze exploration mini-game
- `SortingPuzzleState` -- categorization drag-and-drop puzzle
- `ReaderPuzzleState` -- reading comprehension multiple choice
- `MathPuzzleState` -- arithmetic problem solving

Neptune loads extracted assets from `extracted/on/` when available, falling back to placeholder graphics.

---

## Game Loop

```
+-------------------------------------+
|                                     |
|  +----------+                       |
|  |  Handle  | <-- SDL Events        |
|  |  Input   |                       |
|  +----+-----+                       |
|       |                             |
|       v                             |
|  +----------+                       |
|  |  Update  | <-- Delta Time        |
|  |  State   |                       |
|  +----+-----+                       |
|       |                             |
|       v                             |
|  +----------+                       |
|  |  Render  | --> Frame Buffer      |
|  |  Frame   |                       |
|  +----+-----+                       |
|       |                             |
|       v                             |
|  +----------+                       |
|  | Present  | --> Display           |
|  +----+-----+                       |
|       |                             |
+-------+-----------------------------+
```

Fixed timestep at 60 FPS. The `Game` class owns the state stack and ticks the active state each frame.

---

## Entity System

```
Entity (base)
+-- Player
+-- PartEntity      (collectibles)
+-- DoorEntity      (room transitions)
+-- LadderEntity    (climbing)
+-- PlatformEntity  (moving platforms)
+-- ObstacleEntity  (hazards)
+-- TriggerEntity   (invisible zones)
```

Each entity has position, velocity, collision bounds, sprite/animation, and state flags.

---

## Threading

Single-threaded. Main thread handles all game logic. SDL manages audio mixing internally. Asset loading is synchronous with caching.

---

## Memory Management

- `unique_ptr` / `shared_ptr` for ownership
- Entity lifetime managed by Room
- Asset cache uses reference counting for textures
- GameRegistry owned by Game, lives for the application lifetime
