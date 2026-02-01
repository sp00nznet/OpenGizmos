# OpenGizmos

An open-source reimplementation of **Super Solvers: Gizmos & Gadgets** that runs on modern Windows (and potentially other platforms).

> **Note:** You must provide your own copy of the original game. OpenGizmos extracts and converts assets on-the-fly from a legitimate installation.

## Features

- **Modern SDL2 Engine** - Hardware-accelerated rendering at native resolution with scaling
- **Asset Extraction** - Reads directly from original game files (.DAT, .GRP, .SMK)
- **Full Input Support** - Keyboard, mouse, and customizable key bindings
- **Audio System** - Sound effects and MIDI music playback via SDL_mixer

## Quick Start

### Requirements
- Windows 10/11 (64-bit)
- Original copy of Super Solvers: Gizmos & Gadgets
- Visual Studio 2022 (for building from source)

### Pre-built Binaries
Download the latest release from the [Releases](../../releases) page.

### Building from Source

```bash
# Clone the repository
git clone https://github.com/sp00nznet/-OpenGizmos.git
cd -OpenGizmos

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
└── libs/                  # SDL2 libraries
```

## Documentation

- [Architecture Overview](docs/ARCHITECTURE.md)
- [File Formats](docs/FILE_FORMATS.md)
- [Building Guide](docs/BUILDING.md)

## Asset Tool

The `asset_tool` utility can inspect and extract assets from the original game:

```bash
# List resources in a .DAT file
asset_tool list-ne GIZMO256.DAT

# Extract all resources
asset_tool extract-ne GIZMO256.DAT output_dir/

# Validate game installation
asset_tool validate "C:\Games\Gizmos"
```

## Status

This is an early work-in-progress. Current milestone: **Proof of Concept**

- [x] Project setup with CMake + SDL2
- [x] NE resource extractor
- [x] GRP archive decoder
- [x] Smacker video decoder (basic)
- [x] SDL2 renderer
- [x] Audio system
- [x] Input system with action mapping
- [x] Entity system
- [x] Room/level framework
- [x] Puzzle framework (Balance, Gear implemented)
- [ ] Full game data loading
- [ ] Complete puzzle implementations
- [ ] Vehicle building
- [ ] Racing minigame
- [ ] Morty AI
- [ ] Save/load system

## Legal

OpenGizmos is a clean-room reimplementation. It does not contain any copyrighted assets from the original game. You must own a legitimate copy of Super Solvers: Gizmos & Gadgets to play.

This project is not affiliated with or endorsed by The Learning Company or any current rights holders.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- Original game by The Learning Company (1993)
- SDL2 library by the SDL Community
- Inspired by projects like OpenMW, DevilutionX, and OpenRA
