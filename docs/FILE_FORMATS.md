# File Formats

Documentation of the original game's file formats.

## Overview

| Extension | Format | Purpose |
|-----------|--------|---------|
| `.DAT` | NE DLL | Graphics, fonts, speech resources |
| `.RSC` | NE DLL | Additional resources |
| `.GRP` | RGrp Archive | Sprite collections |
| `.SMK` | Smacker Video | Cutscenes and animations |
| `.MID` | Standard MIDI | Background music |

## NE DLL Resources (.DAT, .RSC)

The game stores graphics and other resources in 16-bit Windows NE (New Executable) DLLs.

### Structure

```
┌──────────────────────────┐
│   DOS MZ Header (64B)    │
│   - magic: 0x5A4D ("MZ") │
│   - NE offset at 0x3C    │
├──────────────────────────┤
│   DOS Stub Program       │
├──────────────────────────┤
│   NE Header (64B)        │
│   - magic: 0x454E ("NE") │
│   - resource table offset│
├──────────────────────────┤
│   Segment Table          │
├──────────────────────────┤
│   Resource Table         │
│   - alignment shift      │
│   - type entries         │
│   - name entries         │
├──────────────────────────┤
│   Resource Data          │
│   - bitmaps              │
│   - raw data             │
│   - fonts                │
└──────────────────────────┘
```

### Resource Types

| Type ID | Name | Description |
|---------|------|-------------|
| 0x8002 | RT_BITMAP | Windows DIB bitmaps |
| 0x8008 | RT_FONT | Bitmap fonts |
| 0x800A | RT_RCDATA | Raw data (palettes, etc.) |

### Key Files

| File | Contents |
|------|----------|
| `GIZMO.DAT` | 16-color graphics |
| `GIZMO256.DAT` | 256-color graphics |
| `PUZZLE.DAT` | Puzzle minigame graphics |
| `FONT.DAT` | Game fonts |
| `*SPCH.DAT` | Speech audio data |

## RGrp Archive (.GRP)

Custom archive format for sprite collections.

### Header Structure

```c
struct GrpHeader {
    char magic[4];      // "RGrp"
    uint32_t padding;   // Usually 0
    uint32_t index1;
    uint32_t offset1;
    char magic2[4];     // "RGrp" (repeated)
    uint32_t padding2;
    uint32_t index2;
    uint32_t offset2;
};

struct GrpFileEntry {
    char name[13];          // Null-terminated 8.3 filename
    uint8_t flags;          // Compression flags
    uint32_t offset;        // Data offset
    uint32_t size;          // Uncompressed size
    uint32_t compressedSize;
};
```

### Compression Types

| Flag | Type | Description |
|------|------|-------------|
| 0x00 | None | Raw data |
| 0x01 | RLE | Run-length encoding |
| 0x02 | LZ | LZ77-style compression |

### Sprite Format

```c
struct SpriteHeader {
    uint16_t width;
    uint16_t height;
    int16_t hotspotX;
    int16_t hotspotY;
    uint16_t flags;
    uint16_t paletteOffset;
};
// Followed by indexed pixel data (8-bit)
```

## Smacker Video (.SMK)

RAD Game Tools Smacker format for video playback.

### Header Structure

```c
struct SmkHeader {
    char signature[4];      // "SMK2" or "SMK4"
    uint32_t width;
    uint32_t height;
    uint32_t frameCount;
    int32_t frameRate;      // Negative = microseconds/frame
    uint32_t flags;
    uint32_t audioSize[7];  // Size per audio track
    uint32_t treesSize;     // Huffman trees size
    uint32_t mMapSize;
    uint32_t mClrSize;
    uint32_t fullSize;
    uint32_t typeSize;
    uint32_t audioRate[7];  // Audio config per track
    uint32_t dummy;
};
```

### Frame Structure

Each frame contains:
1. Optional palette update
2. Optional audio data (up to 7 tracks)
3. Video data (block-based with delta compression)

### Video Encoding

- 4x4 pixel blocks
- Motion vectors for unchanged regions
- Huffman-coded residuals

## MIDI Music (.MID)

Standard MIDI Type 1 files in `SSGWINCD/MIDI/` directory.

For playback, we use SDL_mixer with FluidSynth and a General MIDI soundfont.

## Game Data Structures

### Room Data

```c
struct RoomData {
    uint16_t flags;
    int16_t structuralEntityCounts[4];
    int16_t ladderEntityCounts[4];
    int16_t doorCount;
    int16_t partCount;
    int16_t obstacleCount;
    int16_t triggerCount;
    uint16_t backgroundId;
    uint16_t musicId;
    int16_t width;
    int16_t height;
    int16_t startX;
    int16_t startY;
};
```

### Actor/Entity Data

```c
struct ActorData {
    int16_t spriteId;
    int16_t animationId;
    int32_t positionX;      // Fixed-point
    int32_t positionY;
    int32_t velocityX;
    int32_t velocityY;
    int16_t state;
    int16_t health;
    int16_t direction;
    uint16_t flags;
    // ... (168 bytes total)
};
```

## Palette

The game uses a 256-color VGA palette with 6-bit color values (0-63).

To convert to 8-bit:
```c
uint8_t color8bit = (color6bit << 2) | (color6bit >> 4);
```

## Extracting Assets

Use the `asset_tool` utility:

```bash
# List all resources in a DAT file
asset_tool list-ne GIZMO256.DAT

# Extract to directory
asset_tool extract-ne GIZMO256.DAT ./extracted/

# List GRP contents
asset_tool list-grp SPRITES.GRP
```
