# OpenGizmos

A love letter to 90s educational gaming. OpenGizmos is an open-source engine that brings **The Learning Company** and **MECC** classics back to life on modern hardware.

> **You supply the nostalgia (and a legit copy of the games). We handle the rest.**

---

## Supported Games

### The Learning Company - Super Solvers & Treasure Series

| | Game | Sprites | Audio | Status |
|---|------|:-------:|:-----:|--------|
| :jigsaw: | **Gizmos & Gadgets** | 1,893 | Full | Primary Focus |
| :ocean: | **Operation Neptune** | 1,158 | Full | In Progress |
| :mountain: | **Treasure Mountain!** | 760 | Full | Sprites Extracted |
| :moneybag: | **OutNumbered!** | 906 | 128 | Partial |
| :star2: | **Spellbound!** | 1,198 | Full | Asset Extraction |
| :snowflake: | **Treasure MathStorm!** | 1,644 | Full | Asset Extraction |
| :shell: | **Treasure Cove!** | 1,508 | Full | Asset Extraction |

### MECC

| | Game | Assets | Audio | Status |
|---|------|:------:|:-----:|--------|
| :book: | **Storybook Weaver Deluxe** | ~663 clip art + 40 stories | 120 SFX + 39 MIDI | Asset Discovery |

**Total: 9,067+ sprite frames extracted across 8 games**

---

## What It Does

- **Reads original game files** directly -- no manual asset ripping needed
- **Runs natively** on modern Windows (macOS/Linux planned)
- **Hardware-accelerated** SDL2 rendering with proper scaling
- **Plays the music** -- MIDI and WAV audio via SDL_mixer
- **Documents everything** -- format specs for preservation

---

## Quick Start

### Requirements
- Windows 10/11 (64-bit)
- A copy of one or more supported games
- Visual Studio 2022 (to build from source)

### Build & Run

```bash
git clone https://github.com/sp00nznet/OpenGizmos.git
cd OpenGizmos

mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

```bash
# Launch (auto-detect game files)
opengg.exe

# Point to a specific game
opengg.exe --path "D:\Games\Gizmos"

# Fullscreen at 3x scale
opengg.exe --fullscreen --scale 3
```

### Asset Tool

Extract and inspect original game assets:

```bash
asset_tool list-ne GIZMO256.DAT
asset_tool extract-ne GIZMO256.DAT output/
asset_tool extract-rund NEP256.DLL palette.bin sprites/
```

---

## Documentation

| Doc | What's Inside |
|-----|---------------|
| [Building](docs/BUILDING.md) | Compilers, CMake options, IDE setup, troubleshooting |
| [Architecture](docs/ARCHITECTURE.md) | Engine layers, entity system, state machine, data flow |
| [Game Formats](docs/GAME_FORMATS.md) | NE resources, RLE compression, RUND sprites, room structures |
| [Storybook Weaver](docs/STORYBOOK_WEAVER.md) | MECC resource archives, story files, clip art catalog |
| [Palette Research](docs/PALETTE_RESEARCH.md) | Per-game palette locations and capture methods |
| [Mission Structure](docs/MISSION_STRUCTURE.md) | Game progression, puzzles, objectives for each title |
| [File Formats](docs/FILE_FORMATS.md) | NE DLL, GRP archive, Smacker video, MIDI specs |
| [Development](docs/DEVELOPMENT.md) | Per-game progress checklists and project structure |
| [Contributing](docs/CONTRIBUTING.md) | How to help -- palettes, formats, game logic, testing |

---

## Want to Help?

The biggest needs right now:

1. **Palette capture** -- Several games need VGA palettes dumped from runtime
2. **MECC format RE** -- Storybook Weaver's resource archives need cracking open
3. **Game logic** -- Puzzles, AI, progression systems
4. **Testing** -- Validate extraction across different game versions

See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for details.

---

## Legal

OpenGizmos is a clean-room reimplementation. No copyrighted assets are included. You must own legitimate copies of the supported games.

Not affiliated with The Learning Company, MECC, SoftKey, or any current rights holders.

**License:** [MIT](LICENSE)

---

*Inspired by preservation projects like OpenMW, DevilutionX, and OpenRA.*
*Original games by The Learning Company (1990-1994) and MECC (1994).*
*Built with [SDL2](https://www.libsdl.org/).*
