#pragma once

#include <cstdint>

namespace opengg {

// DOS MZ Header (truncated, we only need the NE offset)
#pragma pack(push, 1)
struct DOSHeader {
    uint16_t magic;         // 'MZ' = 0x5A4D
    uint16_t bytesOnLastPage;
    uint16_t pagesInFile;
    uint16_t relocations;
    uint16_t headerParagraphs;
    uint16_t minExtraParagraphs;
    uint16_t maxExtraParagraphs;
    uint16_t initialSS;
    uint16_t initialSP;
    uint16_t checksum;
    uint16_t initialIP;
    uint16_t initialCS;
    uint16_t relocTableOffset;
    uint16_t overlayNumber;
    uint16_t reserved[4];
    uint16_t oemId;
    uint16_t oemInfo;
    uint16_t reserved2[10];
    uint32_t neHeaderOffset;  // Offset 0x3C - pointer to NE header
};

// NE (New Executable) Header
struct NEHeader {
    uint16_t magic;             // 'NE' = 0x454E
    uint8_t  linkerVersion;
    uint8_t  linkerRevision;
    uint16_t entryTableOffset;
    uint16_t entryTableSize;
    uint32_t crc;
    uint16_t flags;
    uint16_t autoDataSegment;
    uint16_t heapSize;
    uint16_t stackSize;
    uint32_t csip;              // CS:IP entry point
    uint32_t sssp;              // SS:SP stack pointer
    uint16_t segmentCount;
    uint16_t moduleRefCount;
    uint16_t nonResidentNameTableSize;
    uint16_t segmentTableOffset;
    uint16_t resourceTableOffset;
    uint16_t residentNameTableOffset;
    uint16_t moduleRefTableOffset;
    uint16_t importNameTableOffset;
    uint32_t nonResidentNameTableOffset;
    uint16_t movableEntryCount;
    uint16_t alignmentShift;    // Log2 of segment alignment
    uint16_t resourceSegmentCount;
    uint8_t  targetOS;
    uint8_t  os2Flags;
    uint16_t returnThunksOffset;
    uint16_t segmentRefThunksOffset;
    uint16_t minCodeSwapSize;
    uint8_t  expectedWinVersion[2];
};

// Resource Table structures
struct NEResourceTypeInfo {
    uint16_t typeId;        // If high bit set, it's an integer ID; else offset to string
    uint16_t count;         // Number of resources of this type
    uint32_t reserved;
};

struct NEResourceNameInfo {
    uint16_t offset;        // Offset in alignment units from start of file
    uint16_t length;        // Length in alignment units (same as offset)
    uint16_t flags;
    uint16_t id;            // Resource ID (high bit = integer ID)
    uint32_t reserved;
};
#pragma pack(pop)

// Standard NE resource type IDs (prefixed to avoid Windows.h conflicts)
constexpr uint16_t NE_RT_CURSOR       = 0x8001;
constexpr uint16_t NE_RT_BITMAP       = 0x8002;
constexpr uint16_t NE_RT_ICON         = 0x8003;
constexpr uint16_t NE_RT_MENU         = 0x8004;
constexpr uint16_t NE_RT_DIALOG       = 0x8005;
constexpr uint16_t NE_RT_STRING       = 0x8006;
constexpr uint16_t NE_RT_FONTDIR      = 0x8007;
constexpr uint16_t NE_RT_FONT         = 0x8008;
constexpr uint16_t NE_RT_ACCELERATOR  = 0x8009;
constexpr uint16_t NE_RT_RCDATA       = 0x800A;
constexpr uint16_t NE_RT_GROUP_CURSOR = 0x800C;
constexpr uint16_t NE_RT_GROUP_ICON   = 0x800E;

// Magic values
constexpr uint16_t DOS_MAGIC = 0x5A4D;  // 'MZ'
constexpr uint16_t NE_MAGIC  = 0x454E;  // 'NE'

} // namespace opengg
