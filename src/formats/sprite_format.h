#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace opengg {

// Super Solvers: Gizmos & Gadgets sprite format analysis
// Based on reverse engineering of GIZMO.DAT and related files

// File structure of GIZMO.DAT:
// - DOS/NE header (0x0000 - 0x2600)
// - NE resource table with small index records
// - Entity/placement data (0x4000+)
// - Text strings (0x50000+)
// - Graphics pixel data (0x60000+)
// - More graphics data (0x80000+)

// The NE resources (CUSTOM_32513, etc.) appear to be small index records,
// not the actual sprite pixel data. The graphics are stored as raw
// 256-color palette-indexed pixel data in the file.

// Graphics palette is stored in:
// - INSTALL/AUTO256.BMP at offset 54 (1024 bytes, BGRA format)
// - This is the standard game 256-color palette

// Entity record structure (found at 0x4000+)
// Size: 32 bytes per record
#pragma pack(push, 1)
struct EntityPlacement {
    uint16_t type;          // Entity type/sprite type
    uint16_t x;             // X position
    uint16_t y;             // Y position
    uint16_t width;         // Width in pixels
    uint16_t height;        // Height in pixels
    uint16_t reserved1;     // Usually 0
    uint16_t flags;         // Entity flags
    uint16_t spriteId1;     // Primary sprite ID
    uint16_t spriteId2;     // Secondary sprite ID (animation?)
    uint16_t reserved2;     // Usually 0 or 1
    uint8_t  padding[12];   // Remaining bytes
};

// NE resource index record (CUSTOM_32513 type)
// These are small index records that may reference graphics offsets
struct NEResourceIndex {
    uint32_t count;         // Number of items or sub-index
    uint32_t version;       // Always 1?
    uint8_t  data[40];      // Remaining data (mostly zeros)
};

// Standard Windows BITMAPINFOHEADER for any embedded bitmaps
struct BitmapHeader {
    uint32_t headerSize;    // Always 40 (0x28)
    int32_t  width;
    int32_t  height;
    uint16_t planes;        // Always 1
    uint16_t bitCount;      // 8 for 256-color
    uint32_t compression;   // 0 for uncompressed
    uint32_t imageSize;
    int32_t  xPelsPerMeter;
    int32_t  yPelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

// Key file offsets in GIZMO256.DAT
// File structure:
// - DOS/NE header and resources: 0x0000 - 0x3FFF
// - ASEQ animation sequence table: 0x3206+
// - LT lookup table entries: 0x4000+ (12 bytes each)
// - Sprite metadata/index: 0x5000+
// - Sprite header table: 0x60000+
// - RLE compressed sprite data: 0x70000+

constexpr uint32_t GIZMO_ASEQ_TABLE_OFFSET = 0x3206;
constexpr uint32_t GIZMO_LT_TABLE_OFFSET = 0x4000;
constexpr uint32_t GIZMO_SPRITE_META_OFFSET = 0x5000;
constexpr uint32_t GIZMO_SPRITE_HEADER_OFFSET = 0x60000;
constexpr uint32_t GIZMO_SPRITE_DATA_OFFSET = 0x70000;

// LT (Lookup Table) entry format (12 bytes)
// Maps animation IDs to resource types
struct LTEntry {
    char     marker[2];     // "LT"
    uint16_t zero;          // Always 0
    uint16_t size;          // 0x000C (12)
    uint16_t resType;       // 0xFF02 = CUSTOM_32514
    uint16_t resId;         // Resource ID
    char     endMarker[2];  // "DD"
};

// ASEQ (Animation Sequence) entry format (12 bytes)
// Defines animation sequences referencing sprite indices
struct ASEQEntry {
    char     marker[4];     // "ASEQ"
    uint16_t zero;          // Always 0
    uint16_t size;          // 0x000C (12)
    uint16_t resType;       // 0xFF01 = CUSTOM_32513
    uint16_t seqId;         // Sequence ID
};

// Sprite index table header (CUSTOM_32513 resources)
// Variable length - contains offsets into sprite data
struct SpriteIndexHeader {
    uint16_t spriteCount;   // Number of sprites
    uint16_t frameCount;    // Total animation frames
    uint16_t constant;      // Always 10 (0x0A)
    uint16_t field1;        // Usually 1
    uint16_t field2;        // Usually 1
    uint16_t reserved[3];   // Zeros
    // Followed by array of uint32_t offsets[spriteCount]
};

// RLE compression format:
// - 0xFF <byte> <count>: repeat <byte> <count> times
// - Single bytes (not 0xFF): literal pixel value

// Known resource type IDs (with high bit set for NE integer types)
constexpr uint16_t RES_TYPE_CUSTOM_15 = 0x800F;      // Possibly palette reference
constexpr uint16_t RES_TYPE_SPRITE_INDEX = 0xFF01;  // CUSTOM_32513 - sprite index tables
constexpr uint16_t RES_TYPE_SPRITE_META = 0xFF02;   // CUSTOM_32514 - sprite metadata
constexpr uint16_t RES_TYPE_CHAR_DATA = 0xFF03;     // CUSTOM_32515 (single chars)
constexpr uint16_t RES_TYPE_ENTITY = 0xFF04;        // CUSTOM_32516
constexpr uint16_t RES_TYPE_UNKNOWN5 = 0xFF05;      // CUSTOM_32517
constexpr uint16_t RES_TYPE_CHAR_INDEX = 0xFF06;    // CUSTOM_32518
constexpr uint16_t RES_TYPE_AUDIO = 0xFF07;         // CUSTOM_32519 (WAV files)

// Sprite decoder class
class SpriteDecoder {
public:
    // Load the 256-color game palette from AUTO256.BMP
    bool loadPalette(const std::string& gamePath);

    // Get raw graphics data from a DAT file at a specific offset
    std::vector<uint8_t> getRawGraphics(const std::string& datFile,
                                         uint32_t offset,
                                         uint32_t width,
                                         uint32_t height);

    // Decompress RLE-compressed sprite data
    // Format: FF <byte> <count> = repeat byte count times
    //         Other bytes = literal
    static std::vector<uint8_t> decompressRLE(const uint8_t* data, size_t dataSize,
                                               size_t expectedPixels);

    // Convert palette-indexed data to RGBA pixels
    std::vector<uint32_t> convertToRGBA(const std::vector<uint8_t>& indexed);

    // Get palette entry
    void getPaletteColor(uint8_t index, uint8_t& r, uint8_t& g, uint8_t& b) const;

    // Check if palette is loaded
    bool isPaletteLoaded() const { return paletteLoaded_; }

    // Get raw palette pointer
    const uint8_t (*getPalette() const)[4] { return palette_; }

private:
    uint8_t palette_[256][4]; // BGRA format from BMP
    bool paletteLoaded_ = false;
};

} // namespace opengg
