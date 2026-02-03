# Super Solvers / TLC Game Format Analysis

## NE Resource Format Compatibility

All tested games use the Windows NE (New Executable) format for their .DAT resource files.

### File Format Detection
- Magic: `MZ` at offset 0x00, `NE` at offset 0x90
- Resource table follows NE header
- **Important**: Both offset AND length are in alignment units (512-byte sectors)

### Resource Type Mapping

| Type ID | Hex | SSG (Gizmos) | SSO (Outnum) | SSR (Reading) | TMS (MathSt) |
|---------|-----|--------------|--------------|---------------|--------------|
| CUSTOM_15 | 0x800F | Header (1) | Header (1) | Header (1) | Header (1) |
| CUSTOM_32513 | 0xFF01 | Sprites (9) | Sprites (36) | Sprites (201) | Sprites (427) |
| CUSTOM_32514 | 0xFF02 | Sprite Meta (8) | **WAV Audio** (1) | Sprite Meta (11) | Sprite Meta (5) |
| CUSTOM_32515 | 0xFF03 | Definitions (46) | Definitions (3) | Definitions (2) | Definitions (39) |
| CUSTOM_32516 | 0xFF04 | Rooms (15) | Rooms (4) | Rooms (1) | Rooms (1) |
| CUSTOM_32517 | 0xFF05 | Unknown (5) | Unknown (1) | Unknown (1) | Unknown (1) |
| CUSTOM_32518 | 0xFF06 | Unknown (84) | Unknown (24) | Unknown (7) | Unknown (1) |
| CUSTOM_32519 | 0xFF07 | WAV Audio (172) | - | - | WAV Audio (427) |

### Key Observations

1. **Consistent Types**: CUSTOM_32513-32518 are used across all games
2. **Audio Variation**:
   - SSG and TMS use CUSTOM_32519 for WAV audio
   - SSO uses CUSTOM_32514 for WAV audio (type reuse!)
3. **Room Data**: CUSTOM_32516 count matches game structure (SSG: 15 = 3x5 floors)

## Game File Structure

### Super Solvers: Gizmos & Gadgets (SSG)
```
SSGWINCD/
├── GIZMO.DAT      - Main game data (NE, rooms + sounds + definitions)
├── GIZMO16.DAT    - 16-color sprites (NE)
├── GIZMO256.DAT   - 256-color sprites (NE)
├── PUZZLE.DAT     - Puzzle definitions (NE)
├── PUZ16.DAT      - 16-color puzzle sprites (NE)
├── PUZ256.DAT     - 256-color puzzle sprites (NE)
├── AUTO*.DAT      - Racing section (NE)
├── PLANE*.DAT     - Airplane section (NE)
├── AE*.DAT        - Aircraft editor (NE)
├── FONT.DAT       - Fonts (NE)
├── *SPCH*.DAT     - Speech audio (NE)
└── MIDI/          - Standard MIDI files
```

### Super Solvers: OutNumbered (SSO)
```
SSOWINCD/
├── SSO1.DAT       - Main graphics (NE)
├── SSO2.DAT       - Additional graphics (NE)
├── SSO3.DAT       - Additional graphics (NE)
├── SND.DAT        - Sound effects (NE, WAV in CUSTOM_32514)
├── SPEECH.DAT     - Speech audio (NE)
├── SSOWINCD.DAT   - Game save/config
├── SSO.FON        - Game font
└── MIDI/          - Standard MIDI files
```

### Super Solvers: Spellbound (SSR)
```
SSRWINCD/
├── SSR1.DAT       - Main graphics (NE)
├── FB1.DAT, FB2.DAT - Additional data (NE?)
├── SFX.DAT        - Sound effects (NE)
├── SPEECH.DAT     - Speech audio (NE)
├── TASK.RSC       - Task/puzzle text data (custom format)
└── MIDI/          - Standard MIDI files
```

### Super Solvers: Spellbound Wizards (SSS/SSBWINCD)
```
ASSETS/
├── WS107A.GRP     - Game graphics (RGrp archive)
├── WS109.GRP      - Game graphics (RGrp archive)
├── WS203.GRP      - Game graphics (RGrp archive)
├── WSCOMMON.GRP   - Common assets + audio (RGrp archive)
└── *P.GRP         - Palette variants
```
Note: Uses RGrp archive format instead of NE!

### Treasure MathStorm (TMS)
```
DATA/
├── TMSDATA.DAT    - Main game data (NE)
├── TMSANIM.DAT    - Animation data (NE?)
├── TMSSOUND.DAT   - Sound data (NE?)
└── *.SMK          - Smacker video files
```

### Operation Neptune (ONWINCD)
```
ONWINCD/
├── NEP256.DLL     - 256-color sprites (NE, 1158 RUND sprites)
├── SORTER.RSC     - Sorting puzzle sprites/data (NE, CUSTOM_32513/32514/32515)
├── COMMON.RSC     - Common assets + WAV audio (NE, custom types)
├── LABRNTH1.RSC   - Labyrinth level 1 maps (NE, custom types)
├── LABRNTH2.RSC   - Labyrinth level 2 maps (NE, custom types)
├── OT3.RSC        - Additional game data (NE, custom types)
├── READER1.RSC    - Reader puzzle data (NE)
├── READER2.RSC    - Reader puzzle data (NE)
├── AUTORUN.RSC    - Autorun/startup (NE)
├── WS*.GRP        - GRP archives containing WAV audio wrappers
└── WSCOMMON.GRP   - Common audio (GRP wrapper around single WAV)
INSTALL/
└── AUTO256.BMP    - 256-color palette source (602x400 8-bit BMP)
```

**Key Observations for Operation Neptune:**
- Uses NE format like other TLC games
- SORTER.RSC uses standard CUSTOM_32513/32514/32515 types
- Other RSC files use different custom type IDs (0x79xx range):
  - Type 63868-63918: READER puzzle sprites/data
  - Type 63968-63998: LABRNTH map data and sprites
  - Type 64068-64108: OT3 additional game data
  - Types 64168-64203: WAV audio resources (COMMON.RSC)
- Palette stored as doubled bytes (16-bit format) in small resources (1536 bytes)
- GRP files are simple WAV wrappers, not complex archives

**SORTER.RSC Sprite Analysis:**
| Resource ID | Size | Dimensions | Frames | Likely Purpose |
|-------------|------|------------|--------|----------------|
| 35283 | 184KB | 94x109 | 27 | Character animation |
| 35296 | 792KB | varies | many | Main sprite sheet |
| 35368 | 214KB | 64x58 | 26 | Sortable items |
| 35384 | 220KB | 55x47 | 36 | Sortable items |
| 35557 | 28KB | 43x43 | 4 | Icons/buttons |
| 42788-42810 | 0.5-4KB | small | few | UI elements |

**Palette Discovery (Operation Neptune):**
The palette data is stored in CUSTOM_32514 (sprite metadata) resources:
- Format: `00 R 00 G 00 B` (6 bytes per color, 1536 bytes total)
- Each sprite resource has its own palette in its matching CUSTOM_32514
- Extract by taking every other byte starting from index 1
- First 16 entries match standard VGA palette (black, red, green, etc.)

## Sprite Format (Common)

### Header Structure (CUSTOM_32513)

**Two header formats detected in Operation Neptune:**

**Format A (Little-Endian) - Resources 35271-35385, 42788-42810:**
```
Offset  Size  Description
0x00    2     Version (0x0001, little-endian)
0x02    2     Sprite count (little-endian)
0x04    10    Unknown/reserved (often 0x08 0x00 ...)
0x0E    N     Offset table (4 bytes per sprite, LE 32-bit)
```

**Format B (Big-Endian) - Resources 35296, 35322, 35323, etc:**
```
Offset  Size  Description
0x00    2     Version (0x0001, big-endian: 00 01)
0x02    2     Sprite count (big-endian: 00 NN)
0x04    12    Unknown/reserved
0x10    N     Offset table (4 bytes per sprite, LE 32-bit)
```

**Important:** Sprite dimensions (width/height) are NOT stored in the header.
They must be determined from:
- External metadata (definition files)
- Auto-detection from RLE data (count row terminators for height)
- Known values from testing

### RLE Compression
```
FF <byte> <count>  - Repeat byte (count+1) times
00                 - Row terminator (advance to next row)
NN                 - Literal pixel (for any NN where 0 < NN < 0xFF)
```

### Verified Sprite Dimensions (SORTER.RSC)
| Resource | Sprites | Dimensions | Content |
|----------|---------|------------|---------|
| 35283 | 27 | 94x109 | Character animation |
| 35368 | 26 | 64x58 | Sortable items |
| 35384 | 36 | 55x47 | Sortable items |
| 35299 | 8 | 31x9 | Text/labels |
| 35557 | 4 | 43x43 | Icons |
| 35296 | 94 | varies | Main sprite sheet |

## Room/Level Structure (CUSTOM_32516)

### SSG Format (15 rooms)
```
Offset  Size  Description
0x00    2     Building ID (0-2)
0x02    2     Size/type flags
0x04    2     Grid width
0x06    2     Grid height
0x08+   var   Entity positions, sprite refs
```

### SSO Format (4 rooms)
Room data contains entity ID tables (16-bit values) terminated by 0xFFFF.

### TMS Format (tilemap)
Room data appears to be a tilemap with tile type indices (0x00-0x1F range).

### Operation Neptune Labyrinth Format
Each labyrinth RSC contains:
- Small resources (1536 bytes): Doubled-byte palette (768 RGB values stored as 16-bit)
- Large tilemap resources (130KB-260KB): 640x480 RLE-compressed screen images
- Sprite resources: Tile/object sprites (39-43 sprites per resource)

**Tilemap Header (Resources 63968, 63978, 63988, 63998):**
```
Offset  Size  Description
0x00    2     Version (0x0001)
0x02    2     Type (0x0001)
0x04    2     Flags (0x0008)
0x06    2     Unknown (0x000F)
0x08-0x1F     Reserved/zeros
0x20    4     Separator (0xFFFF0000)
0x24    2     Unknown (0x0004)
0x26    2     Width (0x0280 = 640)
0x28    2     Height (0x01E0 = 480)
0x2A+   var   RLE-compressed pixel data
```

**Sprite Resources (63969, 63970):**
```
Offset  Size  Description
0x00    2     Version (0x0001)
0x02    2     Sprite count (39 or 43)
0x04    10    Flags/reserved
0x0E    N     Offset table (4 bytes per sprite)
```

**RLE Compression Format:**
```
FF XX YY     - Repeat pixel XX for YY+1 times
00           - Row terminator (sprite RLE only)
NN           - Literal pixel (if NN != 0xFF and NN != 0x00)
```

**Verified Labyrinth Resources:**
| Resource | Type | Size | Content |
|----------|------|------|---------|
| 63968 | tilemap | 640x480 | Labyrinth 1 background |
| 63969 | sprites | 39 sprites | Labyrinth 1 tiles |
| 63970 | sprites | 43 sprites | Labyrinth 1 objects |
| 63978 | tilemap | 640x480 | Labyrinth transition |
| 63988 | tilemap | 640x480 | Labyrinth 2 background |
| 63998 | tilemap | 640x480 | Labyrinth 2 variation |

### READER Puzzle Format (Operation Neptune)

**READER1.RSC and READER2.RSC Resources:**

Reading comprehension puzzle assets stored as small overlay tilemaps.

| Resource | Type | Size | Content |
|----------|------|------|---------|
| 63868 | tilemap | 640x480 overlay | Reader puzzle background 1 |
| 63869 | sprite data | 229 bytes | Text/UI sprites |
| 63878 | tilemap | 640x480 overlay | Reader puzzle background 2 |
| 63888 | tilemap | 640x480 overlay | Reader puzzle background 3 |
| 63889 | sprite data | 400 bytes | Question sprites |
| 63898 | tilemap | 640x480 overlay | Reader puzzle background 4 |
| 63908 | tilemap | 640x480 overlay | Reader puzzle background 5 |
| 63918 | tilemap | 640x480 overlay | Reader puzzle background 6 |
| 63928 | tilemap | 640x480 overlay | Reader puzzle background 7 |
| 63929 | sprite data | 1038 bytes | Answer option sprites |
| 63938 | tilemap | 640x480 overlay | Reader puzzle background 8 |

**Format:** Same RLE compression as LABRNTH resources. Smaller file sizes indicate
these are overlay images with mostly transparent pixels.

### Operation Neptune 256-Color Sprite DLLs (NEP256.DLL)

The main 256-color sprites for Operation Neptune are stored in NEP256.DLL as RUND-format
resources (CUSTOM_32513 / 0xFF01). The palette is stored in AUTO256.BMP in the INSTALL folder.

**RUND Resource Structure:**
```
Offset  Size  Description
0x00    2     Width (little-endian)
0x02    2     Height (little-endian)
0x04    4     Magic "RUND" (0x52 0x55 0x4E 0x44)
0x08    var   RLE-compressed pixel data
```

**RUND RLE Compression Format:**
```
Byte >= 0x80:  RLE run - count = (byte & 0x7F), next byte is value to repeat
Byte < 0x80:   Literal run - count = byte, followed by 'count' literal bytes
```

Example decoding (8x4 sprite, 32 pixels):
```
03 C8 27 27      -> 3 literals: C8, 27, 27 (3 pixels)
85 C8            -> RLE: repeat C8 five times (5 pixels)
84 27            -> RLE: repeat 27 four times (4 pixels)
84 C8            -> RLE: repeat C8 four times (4 pixels)
84 27            -> RLE: repeat 27 four times (4 pixels)
85 C8            -> RLE: repeat C8 five times (5 pixels)
07 27 27 C8...   -> 7 literals (7 pixels)
Total: 3+5+4+4+4+5+7 = 32 pixels ✓
```

**NEP256.DLL Statistics:**
- Total sprites: 1158
- Resource type: CUSTOM_32513 (0xFF01)
- Dimensions range: 8x4 to 616x304 pixels
- Compression ratio: typically 0.15-0.50

**Palette Source:**
- File: INSTALL/AUTO256.BMP (602x400 8-bit BMP)
- Palette at BMP offset 0x36 (54 bytes), 256 colors × 4 bytes BGRA
- Index 0 typically used for transparency (set to magenta 255,0,255)

### Extraction Status Summary

| Game | Resource | Sprites | Status |
|------|----------|---------|--------|
| Operation Neptune | NEP256.DLL | 1158 | ✓ Extracted (RUND format) |
| Operation Neptune | SORTER.RSC | 495 | ✓ Extracted |
| Operation Neptune | LABRNTH*.RSC tilemaps | 4 | ✓ Decoded (640x480) |
| Operation Neptune | LABRNTH*.RSC sprites | 82 | ✓ Extracted |
| Operation Neptune | READER*.RSC tilemaps | 8 | ✓ Decoded (640x480) |
| Gizmos & Gadgets | GIZMO256.DAT | varies | Partial |
| Gizmos & Gadgets | PUZZLE.DAT | varies | Partial |

## Puzzle Types (SSG)

Resource IDs indicate puzzle category:
- 42xxx: Balance
- 43xxx: Electricity
- 44xxx: Energy
- 45xxx: Force
- 46xxx: Gear
- 47xxx: Jigsaw
- 48xxx: Magnet
- 49xxx: Simple Machine

## Implementation Priority

1. **High Compatibility**: SSG, SSO, SSR, TMS (all use NE format)
2. **Medium**: SSS/Spellbound Wizards (uses RGrp format)
3. **Research Needed**: Treasure Cove, older DOS versions

## Audio Handling

- **WAV**: Embedded in NE resources (CUSTOM_32514 or CUSTOM_32519)
- **MIDI**: Standard .MID files in MIDI/ subdirectory
- **Speech**: Large NE files with embedded WAV
