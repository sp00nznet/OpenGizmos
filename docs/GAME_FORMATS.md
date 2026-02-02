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

## Sprite Format (Common)

### Header Structure (CUSTOM_32513)
```
Offset  Size  Description
0x00    2     Sprite count
0x02    2     Version/flags
0x04    2     Width
0x06    2     Height
0x08    N     Offset table (4 bytes per sprite)
```

### RLE Compression
```
FF <byte> <count>  - Repeat byte (count+1) times
00                 - Row terminator or skip
NN                 - Literal pixel (NN < 0xFF)
```

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
