# OpenGizmos

An open-source reimplementation of **The Learning Company's Super Solvers series** that runs on modern systems.

> **Note:** You must provide your own copy of the original games. OpenGizmos extracts and converts assets on-the-fly from legitimate installations.

## Supported Games

| Game | Sprites | Audio | Status |
|------|---------|-------|--------|
| Super Solvers: Gizmos & Gadgets | ✓ | ✓ | Primary Focus |
| Super Solvers: Operation Neptune | ✓ 1158 | ✓ | In Progress |
| Super Solvers: Treasure Mountain! | ✓ 760 | ✓ | Asset Extraction Working |
| Super Solvers: OutNumbered! | ✗ | ✓ 128 | Needs Format RE |
| Super Solvers: Spellbound! | Partial | - | Research |
| Treasure MathStorm! | ✓ | - | Asset Extraction Working |
| Treasure Cove! | ✓ | - | Asset Extraction Working |

## Vision

Preserve and modernize the classic TLC educational games from the early 1990s by:
- Running natively on modern Windows, macOS, and Linux
- Supporting high-resolution displays with proper scaling
- Maintaining authentic gameplay while improving quality-of-life
- Documenting the original game formats for preservation

## Features

- **Modern SDL2 Engine** - Hardware-accelerated rendering at native resolution with scaling
- **Asset Extraction** - Reads directly from original game files (.DAT, .RSC, .GRP, .SMK)
- **Multi-Game Support** - Unified engine with game-specific logic modules
- **Full Input Support** - Keyboard, mouse, and customizable key bindings
- **Audio System** - Sound effects and MIDI music playback via SDL_mixer

## Quick Start

### Requirements
- Windows 10/11 (64-bit)
- Original copy of one or more supported games
- Visual Studio 2022 (for building from source)

### Pre-built Binaries
Download the latest release from the [Releases](../../releases) page.

### Building from Source

```bash
# Clone the repository
git clone https://github.com/sp00nznet/OpenGizmos.git
cd OpenGizmos

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

The executables will be in `build/Release/`:
- `opengg.exe` - Main game
- `asset_tool.exe` - Asset extraction utility

### Running

```bash
# Auto-detect game installation
opengg.exe

# Specify game path
opengg.exe --path "C:\Games\Gizmos"

# Fullscreen mode
opengg.exe --fullscreen

# Window scale (1-8x)
opengg.exe --scale 3
```

## Controls

| Action | Key |
|--------|-----|
| Move | Arrow Keys / WASD |
| Jump | Space |
| Interact | Enter / E |
| Pause | P / Escape |
| Inventory | Tab / I |

## Project Structure

```
opengg/
├── src/
│   ├── main.cpp           # Entry point
│   ├── loader/            # Asset loading & conversion
│   ├── engine/            # SDL2 engine (render, audio, input)
│   ├── game/              # Game logic (entities, rooms, puzzles)
│   └── formats/           # File format definitions
├── include/               # Public headers
├── docs/                  # Documentation
│   ├── GAME_FORMATS.md    # File format specifications
│   └── MISSION_STRUCTURE.md # Game progression documentation
└── libs/                  # SDL2 libraries
```

## Documentation

- [Game Formats](docs/GAME_FORMATS.md) - File format specifications for all games
- [Mission Structure](docs/MISSION_STRUCTURE.md) - Game progression and objectives

## Asset Tool

The `asset_tool` utility can inspect and extract assets from the original games:

```bash
# List resources in a .DAT or .RSC file
asset_tool list-ne GIZMO256.DAT
asset_tool list-ne SORTER.RSC

# Extract all resources
asset_tool extract-ne GIZMO256.DAT output_dir/

# Extract sprites with palette
asset_tool extract-indexed SORTER.RSC palette.pal sprites/

# Validate game installation
asset_tool validate "C:\Games\Gizmos"
```

## Status

This is an early work-in-progress.

### Gizmos & Gadgets
- [x] NE resource extractor
- [x] Sprite decoder with RLE decompression
- [x] SDL2 renderer and audio
- [x] Entity and room framework
- [x] Puzzle framework (Balance, Gear)
- [ ] Complete puzzle implementations (8 types)
- [ ] Vehicle building system
- [ ] Racing minigame
- [ ] Morty AI

### Operation Neptune
- [x] RSC file format decoder
- [x] Palette extraction (doubled-byte format)
- [x] Sprite extraction from SORTER.RSC (495 sprites)
- [x] NEP256.DLL RUND sprites (1158 sprites)
- [x] Labyrinth/Reader tilemap decoder (640x480)
- [ ] Puzzle implementations
- [ ] Submarine navigation

### Treasure Mountain! (TMT)
- [x] TMT256.DLL RUND format sprites (760 sprites)
- [x] Audio extraction from TM*SOUND.DLL
- [ ] Game logic implementation

### OutNumbered! (SSO)
- [x] Audio extraction (60 SFX + 68 speech)
- [ ] Sprite format differs from other games - needs reverse engineering
- [ ] Palette location unknown

### Other Games
- [x] Treasure MathStorm (TMS) - RUND sprites work
- [x] Treasure Cove - RUND sprites work
- [ ] Game-specific implementations

## Legal

OpenGizmos is a clean-room reimplementation. It does not contain any copyrighted assets from the original games. You must own a legitimate copy of the supported games to play.

This project is not affiliated with or endorsed by The Learning Company, SoftKey, or any current rights holders.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- Original games by The Learning Company (1990-1993)
- SDL2 library by the SDL Community
- Inspired by projects like OpenMW, DevilutionX, and OpenRA
