# Development Status

Progress tracking for all supported games.

---

## Engine Features

- [x] SDL2 renderer (640x480, hardware-accelerated, configurable scaling)
- [x] SDL_mixer audio (WAV, MIDI)
- [x] NE resource extractor (16-bit Windows DLL files)
- [x] GRP sprite archive decoder
- [x] Smacker video player (headers)
- [x] Bitmap font rendering
- [x] Win32 native menu bar (File, Config, Debug, About)
- [x] State machine (push/pop/change)
- [x] **Multi-game launcher** (GameSelectionState, GameLaunchState)
- [x] **Game registry** (manifest parsing, auto-discovery)
- [x] **Extracted asset loading** (BMP sprites, WAV audio, MIDI music)
- [x] **Per-game asset viewer** (browse any game's extracted assets)
- [x] Bot framework (observe, assist, autoplay, speedrun modes)
- [ ] Save/load system
- [ ] Fullscreen toggle from menu
- [ ] macOS / Linux support

---

## Per-Game Progress

### Super Solvers: Gizmos & Gadgets (`ssg`) -- Primary Focus

4,010 sprites | 1,096 WAV | 36 MIDI | 530 puzzle resources

- [x] NE resource extraction (5 DAT files)
- [x] Sprite decoder with RLE decompression
- [x] Doubled-byte palette extraction (CUSTOM_32515)
- [x] Entity and room framework
- [x] Puzzle framework (Balance, Gear)
- [ ] Complete puzzle implementations (8 types)
- [ ] Vehicle building system
- [ ] Racing minigame
- [ ] Morty AI

### Operation Neptune (`on`) -- In Progress

1,158 sprites | 423 WAV | 20 MIDI | 746 puzzle resources

- [x] RSC file format decoder
- [x] Palette extraction (doubled-byte format)
- [x] SORTER.RSC sprites (495 sprites)
- [x] NEP256.DLL RUND sprites (1,158 sprites)
- [x] Labyrinth/Reader tilemap decoder (640x480)
- [x] **NeptuneGameState** (submarine nav, oxygen/fuel, room system)
- [x] **Extracted asset loading** (sprites, WAV, MIDI from `extracted/on/`)
- [ ] Puzzle implementations (sorting, reader, math)
- [ ] Complete room/level data

### Treasure MathStorm! (`tms`)

1,671 sprites | 884 WAV | 43 puzzle resources | 5 video files

- [x] TMSDATA.DAT sprites
- [x] Audio in TMSSOUND.DAT
- [ ] Palette capture needed
- [ ] Game logic implementation

### Treasure Cove! (`tcv`)

1,508 sprites | 2,704 WAV | 26 MIDI | 874 puzzle resources

- [x] TCV256.DLL RUND sprites
- [x] Audio in TC*SOUND.DLL
- [ ] Palette capture needed
- [ ] Game logic implementation

### Spellbound! (`ssr`)

1,310 sprites | 134 WAV | 15 MIDI | 12 puzzle resources

- [x] SSR1.DAT sprites
- [x] Audio in SFX.DAT, SPEECH.DAT
- [ ] Palette verification needed
- [ ] Game logic implementation

### OutNumbered! (`sso`)

852 sprites | 130 WAV | 16 MIDI | 67 puzzle resources

- [x] Audio extraction (60 SFX + 68 speech)
- [x] Sprite extraction (906 frames)
- [ ] Palette capture needed
- [ ] Format verification needed

### Treasure Mountain! (`tmt`)

760 sprites | 2,861 WAV | 20 MIDI | 478 puzzle resources

- [x] TMT256.DLL RUND format sprites
- [x] Correct sprite dimensions extracted
- [x] Audio extraction from TM*SOUND.DLL
- [ ] Palette capture -- see [Palette Research](PALETTE_RESEARCH.md)
- [ ] Game logic implementation

### Spellbound Wizards (`ssb`)

471 sprites | 1,638 WAV | 3 puzzle resources | 1 video file

- [x] Sprite extraction
- [x] Audio extraction
- [ ] Game logic implementation

### Storybook Weaver Deluxe (MECC)

~663 clip art + 40 stories | 120 SFX + 39 MIDI

- [x] ISO structure mapped
- [x] Audio assets identified (120 WAV + 39 MIDI)
- [x] BMP assets identified (12 UI images)
- [x] Story file format (.STS) partially decoded
- [x] Starter stories catalogued (40 stories)
- [x] Clip art categories catalogued (~663 objects)
- [ ] MECC resource archive decoder (rsrcRSED / RSRCDoug formats)
- [ ] SCENERY.RES extraction (scene backgrounds)
- [ ] ADN0001.ADN extraction (clip art sprites)
- [ ] Story file (.STS) full parser
- [ ] Text-to-speech integration
- [ ] Story editor UI
- [ ] Print system

---

## Controls

| Action | Key |
|--------|-----|
| Navigate menus | Arrow Keys |
| Select | Enter |
| Back | Escape |
| Move (in-game) | Arrow Keys / WASD |
| Jump | Space |
| Interact | Enter / E |
| Pause | P / Escape |

---

## Source Layout

```
src/
+-- main.cpp                  # TitleState, GameSelectionState, GameLaunchState,
|                             # AssetViewerState, GameplayState
+-- loader/
|   +-- ne_resource.cpp       # NE DLL resource extractor
|   +-- grp_archive.cpp       # GRP sprite archive decoder
|   +-- smacker.cpp           # Smacker video decoder
|   +-- asset_cache.cpp       # Unified cache (legacy + extracted assets)
|   +-- sprite_decoder.cpp    # RLE sprite decompression
+-- engine/
|   +-- game_loop.cpp         # Game class, state stack, config, GameRegistry init
|   +-- renderer.cpp          # SDL2 rendering
|   +-- audio.cpp             # SDL_mixer audio
|   +-- input.cpp             # Keyboard/mouse input
|   +-- font.cpp              # Bitmap font
|   +-- menu.cpp              # Win32 native menu bar
|   +-- asset_viewer.cpp      # Debug asset browser
+-- game/
|   +-- game_registry.cpp     # Manifest parsing, game auto-discovery
|   +-- entity.cpp            # Base entity
|   +-- room.cpp              # Room/screen management
|   +-- puzzle.cpp            # Puzzle base class
|   +-- player.cpp            # Player character
+-- neptune/
|   +-- neptune_game.cpp      # All Neptune states (submarine, puzzles, labyrinth)
+-- bot/
|   +-- bot_manager.cpp       # Bot orchestration
|   +-- neptune_bot.cpp       # Neptune-specific bot logic
|   +-- gizmos_bot.cpp        # Gizmos-specific bot logic
|   +-- educational_bot.cpp   # Educational game bot
+-- tools/
    +-- asset_tool.cpp        # Standalone extraction CLI
```
