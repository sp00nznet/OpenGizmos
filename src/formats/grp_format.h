#pragma once

#include <cstdint>

namespace opengg {

#pragma pack(push, 1)

// RGrp archive header
struct GrpHeader {
    char     magic[4];      // "RGrp"
    uint32_t padding;       // Usually 0
    uint32_t index1;        // First index value
    uint32_t offset1;       // First offset
    char     magic2[4];     // "RGrp" repeated
    uint32_t padding2;      // Usually 0
    uint32_t index2;        // Second index value
    uint32_t offset2;       // Second offset
};

// File entry in the archive
struct GrpFileEntry {
    char     name[13];      // Null-terminated filename (8.3 format)
    uint8_t  flags;         // Entry flags
    uint32_t offset;        // Offset to data
    uint32_t size;          // Uncompressed size
    uint32_t compressedSize; // Compressed size (if compressed)
};

// Sprite header (found within extracted data)
struct SpriteHeader {
    uint16_t width;
    uint16_t height;
    int16_t  hotspotX;
    int16_t  hotspotY;
    uint16_t flags;
    uint16_t paletteOffset;
};

// Animation sequence header
struct ASeqHeader {
    char     magic[4];      // "ASEQ"
    uint16_t frameCount;
    uint16_t defaultDelay;
    uint16_t flags;
};

// Animation frame info
struct ASeqFrame {
    uint16_t spriteIndex;
    int16_t  offsetX;
    int16_t  offsetY;
    uint16_t delay;
    uint16_t flags;
};

#pragma pack(pop)

// Compression types
constexpr uint8_t GRP_COMPRESSION_NONE = 0x00;
constexpr uint8_t GRP_COMPRESSION_RLE  = 0x01;
constexpr uint8_t GRP_COMPRESSION_LZ   = 0x02;

// RGrp magic
constexpr char GRP_MAGIC[4] = {'R', 'G', 'r', 'p'};

} // namespace opengg
