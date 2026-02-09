# Storybook Weaver Deluxe - Format Analysis

**Publisher:** MECC (Minnesota Educational Computing Corporation)
**Year:** 1994
**Platform:** Windows 3.1 (16-bit NE executables)

Storybook Weaver Deluxe is a creative writing tool for kids (ages 6-12) that lets users build illustrated stories by combining clip art scenes, characters, objects, backgrounds, sound effects, and music. It includes a text-to-speech engine, spell checker, thesaurus, and 40 pre-made "starter stories" that work like madlibs templates.

---

## ISO Structure

```
STORYBOOKWEAVER.iso (235.72 MB)
├── SBW.EXE              # Main application (NE 16-bit, 1.2 MB)
├── SBWED.DLL            # Editor module (NE DLL, 193 KB)
├── ANGLORES.DLL         # English string resources (NE DLL)
├── ESPANRES.DLL         # Spanish string resources (NE DLL)
├── SCENERY.RES          # Scene/background artwork (5.3 MB, MECC rsrcRSED)
├── RESOURCE/
│   ├── ADN0001.ADN      # Clip art data pack (8.6 MB, MECC RSRCDoug)
│   ├── ENG0001.ENG      # English text/labels (246 KB, MECC RSRCDoug)
│   ├── SPN0001.SPN      # Spanish text/labels (252 KB, MECC RSRCDoug)
│   ├── MUS0001.MUS      # Music index (plain text, 61 entries)
│   └── SND0001.SND      # Sound FX index (plain text, 101 entries)
├── TUNES/               # Audio assets
│   ├── *.WAV            # 120 sound effects (11kHz 8-bit mono PCM)
│   └── *.MID            # 39 MIDI music tracks (Type 1)
├── STARTR00-39.STS      # 40 starter story files
├── STRYSTRT.LST         # Starter story index (title,filename CSV)
├── *.BMP                # 12 UI bitmaps (8-bit, 256-color)
├── DOSDEMO/             # DOS predecessor demos (see below)
├── WINDEMO/             # Windows demos of other MECC titles
└── DOSTXT/              # Demo descriptions
```

---

## Asset Inventory

### Audio (159 files, 5.05 MB)

**120 WAV files** - 11,025 Hz, 8-bit unsigned, mono PCM:
- 101 sound effects: animal sounds, environmental, human reactions, mechanical
- 19 ambient loops (named `*_ns_8.wav`): rain, wind, jungle, factory, etc.

**39 MIDI files** - Standard MIDI Type 1, multi-track:
- Musical moods: adventure, african, asian, boogie, carousel, circus, farm, fiesta, future, jazz, mystery, native, parade, patriot, pixie, renaissance, rock, romance, scary, silly, spacy, spooky, suspense, western, etc.

### Clip Art (~663 catalogued objects)

From the DOS demo LST files, the clip art is organized into 8 categories:

| Category | Objects | Examples |
|----------|---------|----------|
| Animals 1 | 91 | badger, bat, bear, butterfly, camel, dolphin, eagle, elephant... |
| Animals 2 | 55 | alligator, cobra, crab, dinosaur, flamingo, lobster, octopus... |
| Nature | 82 | apple tree, boulder, cactus, campfire, cloud, fern, lake, moon... |
| People 1 | 126 | astronaut, ballerina, clown, cowboy, doctor, firefighter, knight... |
| People 2 | 91 | baby, chef, diver, fairy, genie, hiker, jester, mermaid, pirate... |
| Shelter | 55 | barn, bridge, castle, church, igloo, lighthouse, pyramid, tepee... |
| Things | 118 | airplane, anchor, balloon, bicycle, book, candle, crown, drum... |
| Vehicles | 45 | ambulance, bicycle, boat, bus, canoe, helicopter, rocket, train... |

### Starter Stories (40 files)

Pre-made story templates with fill-in-the-blank text, pre-placed scenery, and objects:

| # | Title | # | Title |
|---|-------|---|-------|
| 00 | The Island | 20 | The Wind |
| 01 | The Book | 21 | Under the Ocean |
| 02 | That's my face! | 22 | U.F.O. |
| 03 | The Doorway | 23 | The Submarine |
| 04 | Out of the Storm | 24 | The Car |
| 05 | The Fish | 25 | The Sea Serpent |
| 06 | The Harp | 26 | Halloween |
| 07 | The Kite | 27 | The Big Show |
| 08 | Mario's Dream | 28 | Planet C for Chaos |
| 09 | The Lion's Decision | 29 | Why I Want A Dog |
| 10 | The Visitor | 30 | The Day My Sister Went to College |
| 11 | The Wings | 31 | Terry the Tampering Tabby |
| 12 | Galactic Thrills | 32 | Summer Camp |
| 13 | The Day My Brother Turned into a Bear | 33 | The Game |
| 14 | The Dragon's Journey | 34 | Out West |
| 15 | The Club House | 35 | The Living Tree |
| 16 | The House | 36 | The Tower |
| 17 | The Chess Adventure | 37 | Sueno con flamencos (ES) |
| 18 | The Hunt | 38 | La pinata (ES) |
| 19 | The Drive | 39 | El cuento misterioso (ES) |

### UI Bitmaps (12 files)

| File | Dimensions | Purpose |
|------|-----------|---------|
| MAINMENU.BMP | 640x480 | Main menu background |
| MECC.BMP | 640x480 | MECC logo splash screen |
| SBWDLOGO.BMP | 511x259 | Setup/installer logo |
| CHAIR.BMP | 195x265 | UI decoration |
| TITLE1.BMP | 493x102 | Title text |
| TITLE2.BMP | 397x129 | Title text |
| DESK.BMP | 225x165 | UI decoration |
| PRINTER.BMP | 197x184 | Print UI element |
| DOOR.BMP | 148x132 | UI navigation |
| TITLE3.BMP | 118x29 | Small title text |
| MASTERPA.BMP | 8x9 | Tiling pattern |
| BLACK.BMP | 3x3 | Black fill |

All 8bpp with 256-color embedded palettes. Some use RLE8 compression.

---

## Proprietary Formats

### MECC Resource Archives

Two variants of MECC's proprietary resource container:

#### rsrcRSED (SCENERY.RES)
```
Offset  Size  Description
0x00    4     Zeros/flags
0x04    4     Total file size (big-endian)
0x08    4     Data size (big-endian)
0x0C    4     Offset/flags
...
0x37    N     Pascal string: length byte + name ("Scenery")
0x3C    8     Format tag: "rsrcRSED" (Resource Editor format)
0x44    8     Format tag repeated
...
0x84    4     "DLOG" marker (dialog/layout resources)
```

#### RSRCDoug (ADN, ENG, SPN)
```
Offset  Size  Description
0x00    4     Zeros/flags
0x04    4     Total file size (big-endian)
0x08    4     Data size (big-endian)
...
0x39    N     Pascal string: length byte + name (e.g., "Data 0001")
0x3C    8     Format tag: "RSRCDoug"
```

Both formats use big-endian size fields and length-prefixed name strings. The internal structure contains indexed resources (sprites, text, layout data) but the full directory/offset format has not been decoded yet.

### Story Files (.STS)

```
Offset  Size  Description
0x00    0x21  ASCII header: "SBW Deluxe (Windows) Version 1.0A" (null-padded)
0x21    0x0F  Zero padding
0x30    4     Page count (little-endian uint32)
0x38    4     Flags/settings
0x3C    2     Data length (little-endian uint16)
0x42    N     Title string (null-terminated)
0x50    N     Font descriptor blocks (21 bytes each)
...           Per-page data: scenery IDs, object placements (x,y,size,type,flags)
...           Story text (null-terminated strings)
```

File sizes range from 378 bytes (minimal) to 38,895 bytes (complex multi-page stories).

---

## Text-to-Speech Engine

**ProVoice 2.0.4** by First Byte, Inc.

| File | Description |
|------|-------------|
| FBVNGN.EXE | Voice engine executable |
| FBVSPCH.DLL | Speech synthesis DLL |
| FBVTIMER.DLL | Timer utility DLL |
| EN11K8.DLL | English voice (11kHz 8-bit, 677 KB) |
| EF11K8.DLL | Female English voice (11kHz 8-bit, 1.6 MB) |
| SP11K8.DLL | Spanish voice (11kHz 8-bit, 634 KB) |
| SPD22K16.DLL | 22kHz engine (16 KB) |
| FBV_EN.DIC | English pronunciation dictionary |
| FBV_SP.DIC | Spanish pronunciation dictionary |

## Spell Checker / Thesaurus

**InfoSoft International** libraries:

| File | Description |
|------|-------------|
| ICDLLWIN.DLL | CorrectSpell engine |
| IENS9311.DAT | English spelling data |
| ISPF9420.DAT | Spanish spelling data |
| REDLLWIN.DLL | Roget's Electronic Thesaurus |
| RENTC300.DAT | English thesaurus data |
| RSPTF302.DAT | Spanish thesaurus data |

---

## DOS Demo Formats (DOSDEMO/SBWD)

The disc includes the original DOS version as a demo, which uses different formats:

### UNCS Sprite Archives (.ACT)
```
Offset  Size  Description
0x00    4     Magic: "UNCS" (Uncompressed Sprites?)
0x04    N     Offset/index table
...           Packed 256-color sprite data
```
48 ACT files, 3.87 MB total. Contains all clip art organized by category.

### UNCP Picture Archives (.PIC)
```
Offset  Size  Description
0x00    4     Magic: "UNCP"
0x04    N     Dimensions + index table
0x??    768   VGA palette (256 x RGB, 6-bit values 0-63)
...           320x200 pixel data
```
Contains full-screen 320x200 VGA images with embedded palettes.

### Compressed Audio (.CMP)
68 files with 4-way driver encoding:
- `A*.CMP` - AdLib FM synthesis
- `M*.CMP` - MIDI
- `T*.CMP` - Tandy 3-voice
- `I*.CMP` - Internal PC speaker

17 music pieces x 4 sound drivers = 68 files.

---

## Bundled Demos

The ISO also includes demos for other MECC products:

| Demo | Platform | Directory |
|------|----------|-----------|
| Amazon Trail | DOS + Windows | DOSDEMO/AMAZOND, WINDEMO/AMAZOND |
| Museum Madness | DOS | DOSDEMO/MUSEUMD |
| Oregon Trail | DOS + Windows | DOSDEMO/OTDOSD, WINDEMO/OREGOND |
| Oregon Trail II | Windows | WINDEMO/OTIID |
| Odell Down Under | Windows | WINDEMO/ODELLD |
| Troggle Trouble Math | Windows | WINDEMO/TROGGLED |
| Yukon Trail | Windows | WINDEMO/YUKOND |
| Storybook Weaver (DOS) | DOS | DOSDEMO/SBWD |
| My Own Stories | DOS | DOSDEMO/STORIESD |

---

## Implementation Notes

Unlike the Super Solvers games (which use NE DLL resources with known sprite formats), Storybook Weaver uses MECC's proprietary resource archive system. The main challenges are:

1. **Resource archive decoding** - The rsrcRSED and RSRCDoug formats need full reverse engineering to extract the embedded clip art sprites and scene backgrounds
2. **Story file parsing** - The .STS format is partially decoded but the object placement and scenery reference tables need more work
3. **Editor UI** - SBW is fundamentally different from action/puzzle games; it needs a creative tool interface (scene picker, object browser, text editor, sound selector)
4. **Audio is easy** - Standard WAV and MIDI files, simply indexed by text lists
5. **BMP assets are standard** - All Windows DIBs with embedded palettes, ready to use
