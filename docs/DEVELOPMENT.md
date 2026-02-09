# Development Status

Tracking progress across all supported games.

---

## Super Solvers: Gizmos & Gadgets (Primary Focus)

- [x] NE resource extractor
- [x] Sprite decoder with RLE decompression (1,893 sprites from 5 DAT files)
- [x] Doubled-byte palette extraction (CUSTOM_32515 resources)
- [x] SDL2 renderer and audio
- [x] Entity and room framework
- [x] Puzzle framework (Balance, Gear)
- [ ] Complete puzzle implementations (8 types)
- [ ] Vehicle building system
- [ ] Racing minigame
- [ ] Morty AI

## Operation Neptune

- [x] RSC file format decoder
- [x] Palette extraction (doubled-byte format)
- [x] SORTER.RSC sprites (495 sprites)
- [x] NEP256.DLL RUND sprites (1,158 sprites)
- [x] Labyrinth/Reader tilemap decoder (640x480)
- [ ] Puzzle implementations
- [ ] Submarine navigation

## Treasure Mountain!

- [x] TMT256.DLL RUND format sprites (760 sprites)
- [x] Correct sprite dimensions extracted
- [x] Audio extraction from TM*SOUND.DLL
- [ ] **Palette capture needed** - see [Palette Research](PALETTE_RESEARCH.md)
- [ ] Game logic implementation

## OutNumbered!

- [x] Audio extraction (60 SFX + 68 speech)
- [x] Sprite extraction (906 frames) - dimensions may be incorrect
- [ ] Palette capture needed
- [ ] Format verification needed

## Treasure Cove!

- [x] TCV256.DLL RUND sprites (1,508 sprites)
- [x] Audio in TC*SOUND.DLL files
- [ ] Palette capture needed
- [ ] Game logic implementation

## Treasure MathStorm!

- [x] TMSDATA.DAT sprites (1,644 sprites)
- [x] Audio in TMSSOUND.DAT
- [ ] Palette capture needed
- [ ] Game logic implementation

## Spellbound! / Reading

- [x] SSR1.DAT sprites (1,198 sprites)
- [x] Audio in SFX.DAT, SPEECH.DAT
- [ ] Palette verification needed
- [ ] Game logic implementation

## Storybook Weaver Deluxe

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
│   ├── MISSION_STRUCTURE.md  # Game progression docs
│   ├── STORYBOOK_WEAVER.md   # SBW format analysis
│   ├── ARCHITECTURE.md       # Engine architecture
│   ├── FILE_FORMATS.md       # General format overview
│   ├── BUILDING.md           # Build instructions
│   ├── DEVELOPMENT.md        # This file
│   └── CONTRIBUTING.md       # How to contribute
└── libs/                     # SDL2 libraries
```
