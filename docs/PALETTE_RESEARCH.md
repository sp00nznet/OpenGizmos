# Palette Research Notes

## Overview

The Learning Company's Super Solvers and Treasure series games use 256-color (8-bit) indexed palettes for their graphics. Each game stores its palette differently, making extraction a per-game research effort.

---

## Operation Neptune - SOLVED

**Status:** Palette extraction working

**Location:** `INSTALL/AUTO256.BMP`

**Format:** Standard Windows BMP with embedded 256-color palette
- Palette at BMP offset 0x36 (54 bytes)
- 256 colors × 4 bytes = 1024 bytes (BGRA format)
- Values are 8-bit (0-255)

**Alternative:** CUSTOM_32514 resources in RSC files contain doubled-byte palettes
- Format: `00 R 00 G 00 B` (6 bytes per color)
- Total size: 1536 bytes (256 × 6)
- Extract RGB by taking every other byte

**Extraction Command:**
```bash
asset_tool extract-rund NEP256.DLL palette.bin sprites/
```

---

## Treasure Mountain! (TMT) - PALETTE NEEDED

**Status:** Sprite extraction works, palette not found

### What Works
- RUND format sprite extraction from TMT256.DLL
- 760 sprites with correct dimensions
- Audio extraction from TM*SOUND.DLL files

### Palette Investigation

**Files Searched:**
| File | Size | Result |
|------|------|--------|
| TMT256.DLL | 6.4 MB | No standard palette found |
| TMT.DLL | 398 KB | LOGPALETTE signatures found but not actual color data |
| TMTWINCD.EXE | 184 KB | No embedded palette |
| TM*SOUND.DLL | varies | Audio only |

**Resource Analysis:**
- CUSTOM_15 (32769): Contains PMAP animation data, not palette
- CUSTOM_32514 (33767): Game metadata/lookup tables, not palette
- CUSTOM_32515: Game definitions, not palette
- No dedicated palette resource type found

**Pixel Index Analysis (Title Screen rund_34868_512x268.bmp):**

| Index | Hex | Pixel Count | Expected Color |
|-------|-----|-------------|----------------|
| 113 | 0x71 | 29,216 | Blue sky |
| 15 | 0x0F | 14,614 | Cyan banner |
| 4 | 0x04 | 12,065 | Black |
| 138 | 0x8A | 10,807 | Green (mountain) |
| 255 | 0xFF | 9,618 | White |
| 240 | 0xF0 | 7,458 | Blue variant |
| 140 | 0x8C | 6,108 | Green variant |
| 142 | 0x8E | 4,534 | Green variant |
| 169 | 0xA9 | 4,315 | Yellow/Orange (title) |
| 5 | 0x05 | 3,909 | Red (title outline) |

### How to Capture the Palette

The palette is likely set at runtime via VGA register programming. To capture it:

**Method 1: DOSBox Debugger**
```
1. Run TMT in DOSBox with debugger enabled
2. Break when title screen is displayed
3. Dump VGA DAC palette:
   - Write 0 to port 0x3C7 (read index)
   - Read 768 bytes from port 0x3C9
4. Save as tmt_palette.bin
```

**Method 2: Windows Debugger**
```
1. Break on SetPaletteEntries or RealizePalette
2. Dump the LOGPALETTE structure
3. Extract 256 PALETTEENTRY values (4 bytes each)
```

**Method 3: Memory Dump**
```
1. Run game to title screen
2. Search memory for VGA palette pattern:
   - Starts with black (00 00 00)
   - Contains expected color values
   - 768 bytes (256 × 3) or 1024 bytes (256 × 4)
```

**Expected Palette Format:**
- Raw VGA: 768 bytes, RGB triplets, 6-bit values (0-63)
- Windows: 1024 bytes, RGBX quads, 8-bit values (0-255)

**Once Captured:**
Save to `tmt_palette.bin` and extract with:
```bash
asset_tool extract-rund TMT256.DLL tmt_palette.bin sprites/
```

---

## OutNumbered! (SSO) - NEEDS REVERSE ENGINEERING

**Status:** Audio works, sprites need format RE

### What Works
- Audio extraction: 60 SFX + 68 speech files
- File format detection (NE resources)

### What's Broken
- Sprite format differs from other TLC games
- Palette location unknown
- RLE compression uses different encoding
- Dimension detection fails (produces 80×257, 32×256 etc.)

### Technical Findings

**CUSTOM_15 Resource:**
- Contains ASEQ (Animation Sequence) data
- NOT a palette despite similar structure in other games
- Format: `ASEQ` magic followed by animation definitions

**CUSTOM_32514 Resource:**
- In SND.DAT: WAV audio (works)
- In SSO2.DAT: Game text/puzzle data (NOT sprite metadata)

**Sprite Header Differences:**
Other TLC games use:
```
FF VV CC - Repeat value VV, count CC times
00       - Row terminator
NN       - Literal pixel
```

SSO appears to use a different scheme that hasn't been decoded yet.

### TODO
1. Disassemble SSOWINCD.EXE to find sprite decompression routine
2. Compare sprite data byte patterns with known games
3. Find palette initialization code in executable
4. Test alternate RLE interpretations

---

## Gizmos & Gadgets (SSG) - SOLVED

**Status:** Palette extraction working

**Palette Location:** CUSTOM_32515 resources in *256.DAT files

**Format:** Doubled-byte palette (1536 bytes = 256 × 6)
- Same format as Operation Neptune
- Each color stored as `RR GG BB` (2 bytes per component)
- Extract by taking every other byte

**Extraction Results:**
- 1,893 sprites from 5 DAT files (GIZMO256, AUTO256, PLANE256, PUZ256, AE256)
- Correct colors using extracted palette

---

## Treasure Cove! (TCV) - PALETTE NEEDED

**Status:** Sprite extraction works, palette not found

**Sprite Extraction:**
- TCV256.DLL contains 1,508 RUND format sprites
- Same format as Operation Neptune

**Palette Status:**
- Palette likely set at runtime
- Need to capture via DOSBox or Windows debugger

---

## Treasure MathStorm! (TMS) - PALETTE NEEDED

**Status:** Sprite extraction works, palette not found

**Sprite Extraction:**
- TMSDATA.DAT contains 1,644 sprites
- Extracted via indexed method

**Palette Status:**
- Palette likely set at runtime
- Need to capture via DOSBox or Windows debugger

---

## VGA Palette Technical Reference

### DOS VGA Palette Ports
| Port | Direction | Purpose |
|------|-----------|---------|
| 0x3C6 | R/W | Pixel mask (usually 0xFF) |
| 0x3C7 | W | Read index (set before reading) |
| 0x3C8 | W | Write index (set before writing) |
| 0x3C9 | R/W | Palette data (R, G, B sequentially) |

### VGA 6-bit to 8-bit Conversion
```c
uint8_t convert_6bit_to_8bit(uint8_t val6) {
    return (val6 << 2) | (val6 >> 4);  // Or simply: val6 * 4
}
```

### Common Palette Signatures
```
Standard VGA 16-color (first 16 entries):
00 00 00  - Black
00 00 AA  - Blue
00 AA 00  - Green
00 AA AA  - Cyan
AA 00 00  - Red
AA 00 AA  - Magenta
AA 55 00  - Brown
AA AA AA  - Light Gray
55 55 55  - Dark Gray
55 55 FF  - Light Blue
55 FF 55  - Light Green
55 FF FF  - Light Cyan
FF 55 55  - Light Red
FF 55 FF  - Light Magenta
FF FF 55  - Yellow
FF FF FF  - White
```

---

## Contributing Palettes

If you capture a palette from one of the unsolved games:

1. Save as raw binary (768 or 1024 bytes)
2. Note the format (VGA 6-bit, Windows 8-bit, doubled-byte)
3. Include which game and version
4. Submit via pull request or issue

Palette files should be named: `{game}_palette.bin`
- `tmt_palette.bin` - Treasure Mountain!
- `tcv_palette.bin` - Treasure Cove!
- `tms_palette.bin` - Treasure MathStorm!
- `sso_palette.bin` - OutNumbered!
- ~~`ssg_palette.bin` - Gizmos & Gadgets~~ (SOLVED - in CUSTOM_32515)
