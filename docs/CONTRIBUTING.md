# Contributing

Contributions are welcome! Here's how you can help.

---

## Areas That Need Help

### 1. Palette Capture
Running games in a debugger to dump VGA palettes. Several games still need their palettes captured from runtime. See [PALETTE_RESEARCH.md](PALETTE_RESEARCH.md) for detailed instructions on capturing palettes via DOSBox, Windows debugger, or memory dump.

**Games needing palettes:**
- Treasure Mountain! (`tmt_palette.bin`)
- Treasure Cove! (`tcv_palette.bin`)
- Treasure MathStorm! (`tms_palette.bin`)
- OutNumbered! (`sso_palette.bin`)

### 2. Format Reverse Engineering
- OutNumbered sprite format differs from other Super Solvers games
- MECC resource archives (rsrcRSED / RSRCDoug) used by Storybook Weaver Deluxe
- UNCS/UNCP sprite archives from DOS-era MECC titles

### 3. Game Logic
Implementing puzzles, AI, and progression systems for each game.

### 4. Testing
Validating asset extraction across different game versions and releases.

### 5. Storybook Weaver
- Decoding the MECC proprietary resource format
- Building the story editor interface
- Implementing the clip art browser and scene compositor

---

## Submitting Palettes

If you capture a palette from one of the unsolved games:

1. Save as raw binary (768 or 1024 bytes)
2. Note the format (VGA 6-bit, Windows 8-bit, doubled-byte)
3. Include which game and version
4. Submit via pull request or issue

---

## Code Style

- C++17 standard
- 4-space indentation
- `camelCase` for functions, `snake_case` for files
- Header guards: `#pragma once`
