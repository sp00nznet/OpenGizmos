# OpenGG

An open-source engine for **The Learning Company** educational games from the 90s. OpenGG extracts, loads, and runs 8 classic TLC titles from a single unified launcher.

> **You supply a legit copy of the games. We handle the rest.**

---

## Supported Games

| Game | ID | Sprites | WAV | MIDI | Puzzles | Status |
|------|----|--------:|----:|-----:|--------:|--------|
| **Gizmos & Gadgets** | `ssg` | 4,010 | 1,096 | 36 | 530 | Primary Focus |
| **Operation Neptune** | `on` | 1,158 | 423 | 20 | 746 | In Progress |
| **Treasure MathStorm!** | `tms` | 1,671 | 884 | -- | 43 | Asset Extraction |
| **Treasure Cove!** | `tcv` | 1,508 | 2,704 | 26 | 874 | Asset Extraction |
| **Spellbound!** | `ssr` | 1,310 | 134 | 15 | 12 | Asset Extraction |
| **OutNumbered!** | `sso` | 852 | 130 | 16 | 67 | Asset Extraction |
| **Treasure Mountain!** | `tmt` | 760 | 2,861 | 20 | 478 | Asset Extraction |
| **Spellbound Wizards** | `ssb` | 471 | 1,638 | -- | 3 | Asset Extraction |

**11,740 sprites, 9,870 sound effects, 169 MIDI tracks extracted across 8 games.**

---

## What It Does

- **Multi-game launcher** -- select, browse, and play any of the 8 TLC titles from one window
- **Reads original game files** directly -- NE DLL resources, GRP archives, Smacker video
- **Loads pre-extracted assets** -- BMP sprites, WAV audio, MIDI music (no manual ripping)
- **Hardware-accelerated** SDL2 rendering at 640x480 with scaling
- **Native Windows menus** -- File, Config, Debug, Bot controls via Win32 menu bar
- **Asset viewer** -- browse sprites, play sounds, preview MIDI for any loaded game
- **Extensible** -- game-specific states (Neptune submarine, Gizmos puzzles) plug into a shared engine

---

## Quick Start

### Requirements

- Windows 10/11 (64-bit)
- Visual Studio 2022 (or CMake 3.16+ with any C++17 compiler)
- A copy of one or more supported games

### Build

```bash
git clone https://github.com/sp00nznet/opengg.git
cd opengg

mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Run

```bash
# Auto-discovers games in ../extracted/ or ./extracted/
opengg.exe

# Point to a specific game directory
opengg.exe --path "D:\Games\Gizmos"

# Fullscreen at 3x scale
opengg.exe --fullscreen --scale 3
```

### Extract Assets

```bash
# Standalone tool (no SDL needed)
asset_tool list-ne GIZMO256.DAT
asset_tool extract-ne GIZMO256.DAT output/
asset_tool extract-rund NEP256.DLL palette.bin sprites/
```

---

## How It Works

### Game Discovery

On startup, the engine scans for `extracted/all_games_manifest.json` -- a JSON file listing all 8 games with their asset counts and directory paths. The `GameRegistry` validates which games are actually present on disk and surfaces them in the launcher UI.

### State Flow

```
TitleState ──> GameSelectionState ──> GameLaunchState ──> Game-Specific State
                   (grid of games)     (info + options)    (Neptune, Gizmos, etc.)
                                            │
                                            └──> AssetViewerState
                                                  (browse sprites/audio)
```

### Extracted Asset Layout

```
extracted/
├── all_games_manifest.json
├── on/           # Operation Neptune
│   ├── sprites/  # BMP files
│   └── audio/
│       ├── wav/  # Sound effects
│       └── midi/ # Music tracks
├── ssg/          # Gizmos & Gadgets
│   ├── sprites/
│   └── audio/
│       ├── wav/
│       └── midi/
└── ...           # (same structure for all 8 games)
```

---

## Documentation

| Doc | Contents |
|-----|----------|
| [Architecture](docs/ARCHITECTURE.md) | Engine layers, state machine, multi-game system, data flow |
| [Building](docs/BUILDING.md) | Compilers, CMake options, IDE setup, troubleshooting |
| [Development](docs/DEVELOPMENT.md) | Per-game progress checklists, project structure, controls |
| [Game Formats](docs/GAME_FORMATS.md) | NE resources, RLE compression, RUND sprites, room structures |
| [File Formats](docs/FILE_FORMATS.md) | NE DLL, GRP archive, Smacker video, MIDI specs |
| [Palette Research](docs/PALETTE_RESEARCH.md) | Per-game palette locations and capture methods |
| [Mission Structure](docs/MISSION_STRUCTURE.md) | Game progression, puzzles, objectives per title |
| [Storybook Weaver](docs/STORYBOOK_WEAVER.md) | MECC resource archives, story files, clip art catalog |
| [Contributing](docs/CONTRIBUTING.md) | How to help -- palettes, formats, game logic, testing |

---

## Want to Help?

Biggest needs:

1. **Palette capture** -- several games need VGA palettes dumped from runtime
2. **Game logic** -- puzzles, AI, progression systems for each title
3. **Testing** -- validate extraction across different game versions
4. **New game states** -- implement gameplay for titles beyond Neptune/Gizmos

See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for details.

---

## Legal

OpenGG is a clean-room reimplementation. No copyrighted assets are included.
You must own legitimate copies of the supported games.

Not affiliated with The Learning Company, SoftKey, or any current rights holders.

**License:** [MIT](LICENSE)

---

*Inspired by [OpenMW](https://openmw.org), [DevilutionX](https://github.com/diasurgical/DevilutionX), and [OpenRA](https://www.openra.net).*
*Original games by The Learning Company (1990-1994).*
*Built with [SDL2](https://www.libsdl.org/).*
