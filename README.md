# OpenGizmos

An open-source reimplementation of **The Learning Company's Super Solvers series** that runs on modern systems.

> **Note:** You must provide your own copy of the original games. OpenGizmos extracts and converts assets on-the-fly from legitimate installations.

---

## Supported Games

| Game | Sprites | Audio | Palette | Status |
|------|:-------:|:-----:|:-------:|--------|
| Super Solvers: Gizmos & Gadgets | Partial | Working | TBD | Primary Focus |
| Super Solvers: Operation Neptune | 1,158 | Working | Working | In Progress |
| Super Solvers: Treasure Mountain! | 760 | Working | **Needed** | Sprites Extracted |
| Super Solvers: OutNumbered! | Blocked | 128 | Blocked | Needs RE |
| Super Solvers: Spellbound! | Partial | - | - | Research |
| Treasure MathStorm! | Working | - | TBD | Asset Extraction |
| Treasure Cove! | Working | - | TBD | Asset Extraction |

### Legend
- **Working** - Fully functional
- **Partial** - Basic extraction works
- **Needed** - Format known, palette must be captured from running game
- **Blocked** - Format differs, needs reverse engineering
- **TBD** - Not yet investigated

---

## Vision

Preserve and modernize the classic TLC educational games from the early 1990s by:
- Running natively on modern Windows, macOS, and Linux
- Supporting high-resolution displays with proper scaling
- Maintaining authentic gameplay while improving quality-of-life
- Documenting the original game formats for preservation

---

## Features

- **Modern SDL2 Engine** - Hardware-accelerated rendering with native resolution scaling
- **Asset Extraction** - Reads directly from original game files (.DAT, .RSC, .DLL, .GRP)
- **Multi-Game Support** - Unified engine with game-specific logic modules
- **Full Input Support** - Keyboard, mouse, and customizable key bindings
- **Audio System** - Sound effects and MIDI music playback via SDL_mixer
- **Format Documentation** - Comprehensive reverse-engineering notes for preservation

---

## Quick Start

### Requirements
- Windows 10/11 (64-bit)
- Original copy of one or more supported games
- Visual Studio 2022 (for building from source)

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
- `opengg.exe` - Main game engine
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

---

## Asset Tool

The `asset_tool` utility extracts and converts assets from the original games:

```bash
# List resources in a file
asset_tool list-ne GIZMO256.DAT
asset_tool list-ne TMT256.DLL

# Extract all raw resources
asset_tool extract-ne GIZMO256.DAT output_dir/

# Extract RUND format sprites (Neptune, TMT, TMS)
asset_tool extract-rund NEP256.DLL palette.bin sprites/
asset_tool extract-rund TMT256.DLL palette.bin sprites/

# Extract indexed sprites with palette
asset_tool extract-indexed SORTER.RSC palette.pal sprites/

# Validate game installation
asset_tool validate "C:\Games\Gizmos"
```

---

## Controls

| Action | Key |
|--------|-----|
| Move | Arrow Keys / WASD |
| Jump | Space |
| Interact | Enter / E |
| Pause | P / Escape |
| Inventory | Tab / I |

---

## Project Structure

```
opengg/
├── src/
│   ├── main.cpp              # Entry point
│   ├── loader/               # Asset loading & conversion
│   ├── engine/               # SDL2 engine (render, audio, input)
│   ├── game/                 # Game logic (entities, rooms, puzzles)
│   └── formats/              # File format parsers
├── include/                  # Public headers
├── docs/
│   ├── GAME_FORMATS.md       # File format specifications
│   ├── PALETTE_RESEARCH.md   # Palette extraction research
│   └── MISSION_STRUCTURE.md  # Game progression docs
└── libs/                     # SDL2 libraries
```

---

## Documentation

| Document | Description |
|----------|-------------|
| [Game Formats](docs/GAME_FORMATS.md) | NE resource format, sprite compression, room structures |
| [Palette Research](docs/PALETTE_RESEARCH.md) | Per-game palette locations and capture methods |
| [Mission Structure](docs/MISSION_STRUCTURE.md) | Game progression and objectives |

---

## Development Status

### Gizmos & Gadgets (Primary Focus)
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
- [x] SORTER.RSC sprites (495 sprites)
- [x] NEP256.DLL RUND sprites (1,158 sprites)
- [x] Labyrinth/Reader tilemap decoder (640x480)
- [ ] Puzzle implementations
- [ ] Submarine navigation

### Treasure Mountain!
- [x] TMT256.DLL RUND format sprites (760 sprites)
- [x] Correct sprite dimensions extracted
- [x] Audio extraction from TM*SOUND.DLL
- [ ] **Palette capture needed** - see [Palette Research](docs/PALETTE_RESEARCH.md)
- [ ] Game logic implementation

### OutNumbered!
- [x] Audio extraction (60 SFX + 68 speech)
- [ ] Sprite format reverse engineering needed
- [ ] Palette location unknown
- [ ] RLE compression differs from other games

### Other Games
- [x] Treasure MathStorm - RUND sprites work
- [x] Treasure Cove - RUND sprites work
- [ ] Palettes need capture/research
- [ ] Game-specific implementations

---

## Contributing

Contributions are welcome! Areas that need help:

1. **Palette Capture** - Running games in debugger to dump VGA palettes
2. **Format RE** - OutNumbered sprite format differs from other games
3. **Game Logic** - Implementing puzzles, AI, progression systems
4. **Testing** - Validating extraction across different game versions

See [PALETTE_RESEARCH.md](docs/PALETTE_RESEARCH.md) for palette capture instructions.

---

## Legal

OpenGizmos is a clean-room reimplementation. It does not contain any copyrighted assets from the original games. You must own a legitimate copy of the supported games to play.

This project is not affiliated with or endorsed by The Learning Company, SoftKey, or any current rights holders.

---

## License

MIT License - see [LICENSE](LICENSE) for details.

---

## Acknowledgments

- Original games by The Learning Company (1990-1994)
- SDL2 library by the SDL Community
- Inspired by preservation projects like OpenMW, DevilutionX, and OpenRA
