#include "ne_resource.h"
#include "grp_archive.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstring>
#include <map>
#include <algorithm>

namespace fs = std::filesystem;
using namespace opengg;

// Helper function to load palette from either raw .pal file (1024 bytes) or BMP with embedded palette
bool loadPalette(const std::string& palettePath, uint8_t palette[256][4]) {
    std::ifstream palFile(palettePath, std::ios::binary | std::ios::ate);
    if (!palFile) {
        // Use grayscale fallback
        std::cerr << "Warning: Could not load palette from " << palettePath << ", using grayscale\n";
        for (int i = 0; i < 256; ++i) {
            palette[i][0] = palette[i][1] = palette[i][2] = i;
            palette[i][3] = 0;
        }
        return false;
    }

    auto fileSize = palFile.tellg();
    palFile.seekg(0);

    // Check if it's a BMP file (starts with "BM")
    char magic[2];
    palFile.read(magic, 2);

    if (magic[0] == 'B' && magic[1] == 'M') {
        // BMP file - seek past header to palette
        palFile.seekg(54);
        palFile.read(reinterpret_cast<char*>(palette), 1024);
        std::cout << "Loaded palette from BMP: " << palettePath << "\n";
    } else if (fileSize == 1024) {
        // Raw 1024-byte palette file
        palFile.seekg(0);
        palFile.read(reinterpret_cast<char*>(palette), 1024);
        std::cout << "Loaded raw palette: " << palettePath << "\n";
    } else if (fileSize >= 768) {
        // Possibly RGB palette (768 bytes = 256 * 3)
        palFile.seekg(0);
        uint8_t rgb[768];
        palFile.read(reinterpret_cast<char*>(rgb), 768);
        for (int i = 0; i < 256; ++i) {
            palette[i][0] = rgb[i * 3 + 2];  // B
            palette[i][1] = rgb[i * 3 + 1];  // G
            palette[i][2] = rgb[i * 3 + 0];  // R
            palette[i][3] = 0;
        }
        std::cout << "Loaded RGB palette: " << palettePath << "\n";
    } else {
        std::cerr << "Warning: Unknown palette format (size=" << fileSize << "), using grayscale\n";
        for (int i = 0; i < 256; ++i) {
            palette[i][0] = palette[i][1] = palette[i][2] = i;
            palette[i][3] = 0;
        }
        return false;
    }

    return true;
}

void printUsage(const char* progName) {
    std::cout << "OpenGizmos Asset Tool\n\n";
    std::cout << "Usage: " << progName << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  list-ne <file>           List resources in NE file (.DAT, .RSC)\n";
    std::cout << "  extract-ne <file> <out>  Extract all resources from NE file\n";
    std::cout << "  list-grp <file>          List files in GRP archive\n";
    std::cout << "  extract-grp <file> <out> Extract all files from GRP archive\n";
    std::cout << "  info <gamepath>          Show game file information\n";
    std::cout << "  validate <gamepath>      Validate game installation\n";
    std::cout << "  analyze-sprites <file>   Analyze sprite format in NE file\n";
    std::cout << "  analyze-ne <file>        Analyze NE file structure\n";
    std::cout << "  analyze-entities <file>  Analyze entity records and sprite tables\n";
    std::cout << "  analyze-aseq <file>      Analyze ASEQ animation sequences\n";
    std::cout << "  analyze-sprite-res <file> Analyze sprite resource format\n";
    std::cout << "  analyze-raw <file>       Analyze raw file structure\n";
    std::cout << "  analyze-rle <file>       Analyze RLE compression format\n";
    std::cout << "  analyze-index <file>     Analyze sprite index records\n";
    std::cout << "  deep-analyze <file>      Deep sprite format analysis\n";
    std::cout << "  trace-offsets <file>     Trace sprite offsets from indices\n";
    std::cout << "  extract-sprite <file> <palette> <offset> <w> <h> <out>\n";
    std::cout << "                           Extract sprite at offset\n";
    std::cout << "  extract-all <file> <palette> <outdir>\n";
    std::cout << "                           Extract all detected sprites\n";
    std::cout << "  analyze-meta <file>      Analyze sprite metadata resources\n";
    std::cout << "  extract-v2 <file> <palette> <outdir>\n";
    std::cout << "                           Extract sprites using improved algorithm\n";
    std::cout << "  extract-raw <file> <palette> <outdir>\n";
    std::cout << "                           Extract sprites as raw data (no RLE)\n";
    std::cout << "  test-dims <file> <palette> <outdir>\n";
    std::cout << "                           Test extraction with fixed dimensions\n";
    std::cout << "  find-width <file> <palette> <outdir>\n";
    std::cout << "                           Find correct sprite width by testing 8-80\n";
    std::cout << "  extract-single <file> <palette> <offset> <out>\n";
    std::cout << "                           Extract single sprite at offset using header dims\n";
    std::cout << "  extract-indexed <file> <palette> <outdir>\n";
    std::cout << "                           Extract all sprites using index metadata\n";
    std::cout << "  extract-rund <file> <palette> <outdir>\n";
    std::cout << "                           Extract RUND format sprites (Treasure games)\n";
    std::cout << "  extract-labyrinth <file> <outdir>\n";
    std::cout << "                           Extract Operation Neptune labyrinth tilemaps\n";
    std::cout << "  extract-labyrinth-sprites <file> <palette> <outdir>\n";
    std::cout << "                           Extract Operation Neptune labyrinth sprites\n";
    std::cout << "  extract-dims <file> <palette> <offset> <w> <h> [header=0|1] <out>\n";
    std::cout << "                           Extract sprite with specified dimensions\n";
    std::cout << "  test-rle <file> <palette> <offset> <w> <h> <outdir>\n";
    std::cout << "                           Test different RLE formats\n";
    std::cout << "\n";
}

void listNE(const std::string& path) {
    NEResourceExtractor ne;

    if (!ne.open(path)) {
        std::cerr << "Error: " << ne.getLastError() << "\n";
        return;
    }

    auto resources = ne.listResources();
    std::cout << "Found " << resources.size() << " resources in " << path << "\n\n";

    std::cout << "Type\t\tID\tSize\tOffset\n";
    std::cout << "----\t\t--\t----\t------\n";

    for (const auto& res : resources) {
        std::cout << res.typeName << "\t";
        if (res.typeName.length() < 8) std::cout << "\t";
        std::cout << res.id << "\t";
        std::cout << res.size << "\t";
        std::cout << "0x" << std::hex << res.offset << std::dec << "\n";
    }
}

void extractNE(const std::string& path, const std::string& outDir) {
    NEResourceExtractor ne;

    if (!ne.open(path)) {
        std::cerr << "Error: " << ne.getLastError() << "\n";
        return;
    }

    fs::create_directories(outDir);

    auto resources = ne.listResources();
    int extracted = 0;

    for (const auto& res : resources) {
        std::string filename = res.typeName + "_" + std::to_string(res.id);

        // Add extension based on type
        if (res.typeId == NE_RT_BITMAP) {
            filename += ".bmp";
        } else if (res.typeId == NE_RT_RCDATA) {
            filename += ".dat";
        } else {
            filename += ".bin";
        }

        std::string outPath = outDir + "/" + filename;

        if (res.typeId == NE_RT_BITMAP) {
            if (ne.extractBitmap(res.id, outPath)) {
                extracted++;
                std::cout << "Extracted: " << filename << "\n";
            }
        } else {
            auto data = ne.extractResource(res);
            if (!data.empty()) {
                std::ofstream outFile(outPath, std::ios::binary);
                outFile.write(reinterpret_cast<char*>(data.data()), data.size());
                extracted++;
                std::cout << "Extracted: " << filename << "\n";
            }
        }
    }

    std::cout << "\nExtracted " << extracted << " resources.\n";
}

void listGRP(const std::string& path) {
    GrpArchive grp;

    if (!grp.open(path)) {
        std::cerr << "Error: " << grp.getLastError() << "\n";
        return;
    }

    auto files = grp.listFiles();
    std::cout << "Found " << files.size() << " files in " << path << "\n\n";

    std::cout << "Name\t\t\tSize\n";
    std::cout << "----\t\t\t----\n";

    for (const auto& name : files) {
        auto* entry = grp.getEntry(name);
        std::cout << name;
        if (name.length() < 8) std::cout << "\t";
        if (name.length() < 16) std::cout << "\t";
        std::cout << "\t" << (entry ? entry->size : 0) << "\n";
    }
}

void extractGRP(const std::string& path, const std::string& outDir) {
    GrpArchive grp;

    if (!grp.open(path)) {
        std::cerr << "Error: " << grp.getLastError() << "\n";
        return;
    }

    fs::create_directories(outDir);

    auto files = grp.listFiles();
    int extracted = 0;

    for (const auto& name : files) {
        auto data = grp.extract(name);
        if (!data.empty()) {
            std::string outPath = outDir + "/" + name;
            std::ofstream outFile(outPath, std::ios::binary);
            outFile.write(reinterpret_cast<char*>(data.data()), data.size());
            extracted++;
            std::cout << "Extracted: " << name << "\n";
        }
    }

    std::cout << "\nExtracted " << extracted << " files.\n";
}

void showInfo(const std::string& gamePath) {
    std::cout << "Game Path: " << gamePath << "\n\n";

    // Check for key files
    std::vector<std::pair<std::string, std::string>> keyFiles = {
        {"SSGWIN32.EXE", "Main executable"},
        {"SSGWINCD/GIZMO.DAT", "16-color graphics"},
        {"SSGWINCD/GIZMO256.DAT", "256-color graphics"},
        {"SSGWINCD/PUZZLE.DAT", "Puzzle graphics"},
        {"SSGWINCD/FONT.DAT", "Fonts"},
        {"MOVIES/INTRO.SMK", "Intro video"},
    };

    std::cout << "File Status:\n";
    std::cout << "------------\n";

    for (const auto& [file, desc] : keyFiles) {
        std::string fullPath = gamePath + "/" + file;
        bool exists = fs::exists(fullPath);

        std::cout << (exists ? "[OK]  " : "[--]  ");
        std::cout << desc << " (" << file << ")";

        if (exists) {
            auto size = fs::file_size(fullPath);
            std::cout << " - " << size << " bytes";
        }

        std::cout << "\n";
    }

    // List DAT files
    std::cout << "\nDAT Files Found:\n";
    std::string datDir = gamePath + "/SSGWINCD";
    if (fs::exists(datDir)) {
        for (const auto& entry : fs::directory_iterator(datDir)) {
            if (entry.path().extension() == ".DAT") {
                NEResourceExtractor ne;
                if (ne.open(entry.path().string())) {
                    auto resources = ne.listResources();
                    std::cout << "  " << entry.path().filename().string();
                    std::cout << " - " << resources.size() << " resources\n";
                }
            }
        }
    }

    // List GRP files
    std::cout << "\nGRP Files Found:\n";
    std::string assetDir = gamePath + "/ASSETS";
    if (fs::exists(assetDir)) {
        for (const auto& entry : fs::directory_iterator(assetDir)) {
            if (entry.path().extension() == ".GRP") {
                GrpArchive grp;
                if (grp.open(entry.path().string())) {
                    auto files = grp.listFiles();
                    std::cout << "  " << entry.path().filename().string();
                    std::cout << " - " << files.size() << " files\n";
                }
            }
        }
    }

    // List SMK files
    std::cout << "\nVideo Files Found:\n";
    std::string movieDir = gamePath + "/MOVIES";
    if (fs::exists(movieDir)) {
        for (const auto& entry : fs::directory_iterator(movieDir)) {
            if (entry.path().extension() == ".SMK") {
                auto size = fs::file_size(entry.path());
                std::cout << "  " << entry.path().filename().string();
                std::cout << " - " << (size / 1024) << " KB\n";
            }
        }
    }
}

void analyzeNEStructure(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return;
    }

    // Read DOS header
    uint16_t mzMagic;
    file.read(reinterpret_cast<char*>(&mzMagic), 2);
    if (mzMagic != 0x5A4D) {
        std::cerr << "Not a valid MZ executable\n";
        return;
    }

    // Get NE header offset
    file.seekg(0x3C);
    uint32_t neOffset;
    file.read(reinterpret_cast<char*>(&neOffset), 4);

    std::cout << "NE header at offset: 0x" << std::hex << neOffset << std::dec << "\n";

    // Read NE header
    file.seekg(neOffset);
    uint16_t neMagic;
    file.read(reinterpret_cast<char*>(&neMagic), 2);
    if (neMagic != 0x454E) {
        std::cerr << "Not a valid NE executable\n";
        return;
    }

    // Read key NE header fields
    file.seekg(neOffset + 0x1C); // Segment count
    uint16_t segmentCount;
    file.read(reinterpret_cast<char*>(&segmentCount), 2);

    file.seekg(neOffset + 0x22); // Segment table offset
    uint16_t segTableOffset;
    file.read(reinterpret_cast<char*>(&segTableOffset), 2);

    file.seekg(neOffset + 0x24); // Resource table offset
    uint16_t resTableOffset;
    file.read(reinterpret_cast<char*>(&resTableOffset), 2);

    file.seekg(neOffset + 0x32); // Alignment shift
    uint16_t alignShift;
    file.read(reinterpret_cast<char*>(&alignShift), 2);

    std::cout << "Segment count: " << segmentCount << "\n";
    std::cout << "Segment table offset: 0x" << std::hex << (neOffset + segTableOffset) << std::dec << "\n";
    std::cout << "Resource table offset: 0x" << std::hex << (neOffset + resTableOffset) << std::dec << "\n";
    std::cout << "Alignment shift: " << alignShift << " (unit = " << (1 << alignShift) << " bytes)\n\n";

    // Parse segment table
    std::cout << "=== Segment Table ===\n";
    std::cout << "Seg#  FileOff     FileLen   Flags     MinAlloc\n";

    file.seekg(neOffset + segTableOffset);
    size_t totalSegmentSize = 0;

    for (int i = 0; i < segmentCount; ++i) {
        uint16_t segOffset, segLength, segFlags, segMinAlloc;
        file.read(reinterpret_cast<char*>(&segOffset), 2);
        file.read(reinterpret_cast<char*>(&segLength), 2);
        file.read(reinterpret_cast<char*>(&segFlags), 2);
        file.read(reinterpret_cast<char*>(&segMinAlloc), 2);

        uint32_t actualOffset = static_cast<uint32_t>(segOffset) << alignShift;
        uint32_t actualLength = segLength ? segLength : 0x10000;

        totalSegmentSize += actualLength;

        printf("%4d  0x%08X  %8u  0x%04X    %u\n",
               i + 1, actualOffset, actualLength, segFlags, segMinAlloc);

        // For large segments, peek at content
        if (actualLength > 10000) {
            auto currentPos = file.tellg();
            file.seekg(actualOffset);
            uint8_t peek[32];
            file.read(reinterpret_cast<char*>(peek), 32);
            std::cout << "      First 32 bytes: ";
            for (int j = 0; j < 32; ++j) {
                printf("%02X ", peek[j]);
            }
            std::cout << "\n";
            file.seekg(currentPos);
        }
    }

    std::cout << "\nTotal segment data: " << totalSegmentSize << " bytes\n";

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    std::cout << "File size: " << fileSize << " bytes\n";
}

void analyzeSprites(const std::string& path) {
    NEResourceExtractor ne;

    if (!ne.open(path)) {
        std::cerr << "Error: " << ne.getLastError() << "\n";
        return;
    }

    auto resources = ne.listResources();
    std::cout << "Analyzing sprites in " << path << "\n";
    std::cout << "Found " << resources.size() << " resources\n\n";

    // Group by type
    std::map<uint16_t, std::vector<Resource>> byType;
    for (const auto& res : resources) {
        byType[res.typeId].push_back(res);
    }

    std::cout << "Resource types found:\n";
    for (const auto& [typeId, resList] : byType) {
        std::cout << "  Type 0x" << std::hex << typeId << std::dec
                  << " (" << resList[0].typeName << "): " << resList.size() << " resources\n";

        // Show size range
        size_t minSize = SIZE_MAX, maxSize = 0;
        for (const auto& res : resList) {
            minSize = std::min(minSize, (size_t)res.size);
            maxSize = std::max(maxSize, (size_t)res.size);
        }
        std::cout << "    Size range: " << minSize << " - " << maxSize << " bytes\n";
    }
    std::cout << "\n";

    // Check for palette (type 0x800F or look in CUSTOM_15)
    auto paletteIt = byType.find(0x800F);
    if (paletteIt != byType.end() && !paletteIt->second.empty()) {
        std::cout << "=== Checking CUSTOM_15 (likely palette) ===\n";
        auto& palRes = paletteIt->second[0];
        auto palData = ne.extractResource(palRes);
        std::cout << "Resource size: " << palData.size() << " bytes\n";

        if (palData.size() >= 768) {
            std::cout << "Could be 256-color palette (768 bytes = 256 * RGB)\n";
        } else if (palData.size() >= 1024) {
            std::cout << "Could be 256-color palette with alpha (1024 bytes = 256 * RGBA)\n";
        }

        // Print first few colors
        std::cout << "First 16 entries:\n";
        for (int i = 0; i < std::min(16, (int)palData.size() / 4); ++i) {
            int offset = i * 4;
            if (offset + 3 < (int)palData.size()) {
                printf("  %2d: R=%3d G=%3d B=%3d A=%3d\n", i,
                       palData[offset], palData[offset+1],
                       palData[offset+2], palData[offset+3]);
            }
        }
        std::cout << "\n";
    }

    // Analyze larger sprites from CUSTOM_32514 (0xFF02)
    std::cout << "=== Analyzing CUSTOM_32514 (0xFF02) - likely main sprites ===\n";
    auto it = byType.find(0xFF02);
    if (it != byType.end()) {
        // Find resources with sizes that suggest actual sprites (>100 bytes)
        std::vector<Resource> largeRes;
        for (const auto& res : it->second) {
            if (res.size > 100) {
                largeRes.push_back(res);
            }
        }

        std::cout << "Resources > 100 bytes: " << largeRes.size() << "\n\n";

        // Analyze first 5 large ones
        int count = 0;
        for (const auto& res : largeRes) {
            if (count++ >= 5) break;

            auto data = ne.extractResource(res);
            if (data.empty()) continue;

            std::cout << "Resource #" << res.id << " (" << data.size() << " bytes):\n";
            std::cout << "  First 48 bytes: ";
            for (size_t i = 0; i < std::min(data.size(), size_t(48)); ++i) {
                printf("%02X ", data[i]);
            }
            std::cout << "\n";

            // Try different header interpretations
            if (data.size() >= 8) {
                // Interpretation 1: 2-byte width, 2-byte height at start
                uint16_t w1 = data[0] | (data[1] << 8);
                uint16_t h1 = data[2] | (data[3] << 8);
                std::cout << "  Int1 (w16,h16 @ 0): " << w1 << "x" << h1;
                if (w1 > 0 && w1 <= 640 && h1 > 0 && h1 <= 480) {
                    std::cout << " [VALID]";
                    size_t expected = w1 * h1;
                    std::cout << " expected=" << expected << " ratio=" << (data.size() * 100 / expected) << "%";
                }
                std::cout << "\n";

                // Interpretation 2: Might have flags/type at start
                uint16_t w2 = data[2] | (data[3] << 8);
                uint16_t h2 = data[4] | (data[5] << 8);
                std::cout << "  Int2 (w16,h16 @ 2): " << w2 << "x" << h2;
                if (w2 > 0 && w2 <= 640 && h2 > 0 && h2 <= 480) {
                    std::cout << " [VALID]";
                }
                std::cout << "\n";

                // Interpretation 3: Check for offset table (common in sprite sheets)
                uint32_t off1 = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                std::cout << "  First 4 bytes as offset: " << off1 << "\n";
            }
            std::cout << "\n";
        }
    }

    // Also analyze CUSTOM_32515 (0xFF03)
    std::cout << "=== Analyzing CUSTOM_32515 (0xFF03) ===\n";
    it = byType.find(0xFF03);
    if (it != byType.end()) {
        for (const auto& res : it->second) {
            auto data = ne.extractResource(res);
            if (data.empty()) continue;

            std::cout << "Resource #" << res.id << " (" << data.size() << " bytes):\n";
            std::cout << "  First 64 bytes: ";
            for (size_t i = 0; i < std::min(data.size(), size_t(64)); ++i) {
                printf("%02X ", data[i]);
                if ((i + 1) % 32 == 0) std::cout << "\n                  ";
            }
            std::cout << "\n\n";
        }
    }
}

void extractSprite(const std::string& path, const std::string& palettePath, uint32_t offset, int width, int height, const std::string& outPath) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << "\n";
        return;
    }

    // Load palette
    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    // Read sprite data
    file.seekg(offset);
    std::vector<uint8_t> rawData(width * height * 2);  // Extra space for RLE
    file.read(reinterpret_cast<char*>(rawData.data()), rawData.size());

    std::cout << "First 32 bytes at offset 0x" << std::hex << offset << std::dec << ":\n  ";
    for (int i = 0; i < 32; ++i) {
        printf("%02X ", rawData[i]);
    }
    std::cout << "\n";

    // Try RLE decompression (FF xx count format)
    std::vector<uint8_t> pixels;
    pixels.reserve(width * height);

    size_t i = 0;
    while (pixels.size() < (size_t)(width * height) && i < rawData.size()) {
        if (rawData[i] == 0xFF && i + 2 < rawData.size()) {
            // RLE: FF <byte> <count>
            uint8_t byte = rawData[i + 1];
            uint8_t count = rawData[i + 2];
            if (count == 0) count = 1;  // Prevent infinite loop
            for (int j = 0; j < count && pixels.size() < (size_t)(width * height); ++j) {
                pixels.push_back(byte);
            }
            i += 3;
        } else {
            // Literal byte
            pixels.push_back(rawData[i]);
            i++;
        }
    }

    std::cout << "Decompressed " << pixels.size() << " pixels (expected " << (width * height) << ")\n";

    // If we didn't get enough pixels, try raw (no compression)
    if (pixels.size() < (size_t)(width * height)) {
        std::cout << "Trying raw format (no compression)...\n";
        pixels.clear();
        for (int j = 0; j < width * height && j < (int)rawData.size(); ++j) {
            pixels.push_back(rawData[j]);
        }
    }

    // Create BMP file
    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to create output file\n";
        return;
    }

    // BMP header
    int rowSize = (width + 3) & ~3;  // Align to 4 bytes
    int imageSize = rowSize * height;
    int fileSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {
        'B', 'M',                          // Magic
        (uint8_t)(fileSize), (uint8_t)(fileSize >> 8), (uint8_t)(fileSize >> 16), (uint8_t)(fileSize >> 24),
        0, 0, 0, 0,                         // Reserved
        54 + 1024, 0, 0, 0,                 // Offset to pixel data
        40, 0, 0, 0,                        // Header size
        (uint8_t)(width), (uint8_t)(width >> 8), (uint8_t)(width >> 16), (uint8_t)(width >> 24),
        (uint8_t)(height), (uint8_t)(height >> 8), (uint8_t)(height >> 16), (uint8_t)(height >> 24),
        1, 0,                               // Planes
        8, 0,                               // Bits per pixel
        0, 0, 0, 0,                         // Compression
        (uint8_t)(imageSize), (uint8_t)(imageSize >> 8), (uint8_t)(imageSize >> 16), (uint8_t)(imageSize >> 24),
        0, 0, 0, 0,                         // X pels/meter
        0, 0, 0, 0,                         // Y pels/meter
        0, 1, 0, 0,                         // Colors used (256)
        0, 0, 0, 0                          // Important colors
    };

    out.write(reinterpret_cast<char*>(bmpHeader), 54);
    out.write(reinterpret_cast<char*>(palette), 1024);

    // Write pixel data (BMP is bottom-up)
    std::vector<uint8_t> row(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            size_t srcIdx = y * width + x;
            row[x] = (srcIdx < pixels.size()) ? pixels[srcIdx] : 0;
        }
        out.write(reinterpret_cast<char*>(row.data()), rowSize);
    }

    std::cout << "Saved to " << outPath << "\n";
}

// Scan for sprite dimensions encoded before pixel data
struct DetectedSprite {
    uint32_t offset;
    int16_t width;
    int16_t height;
    bool hasRLE;
};

std::vector<DetectedSprite> detectSprites(const std::vector<uint8_t>& data, uint32_t baseOffset) {
    std::vector<DetectedSprite> sprites;

    for (size_t i = 0; i + 8 < data.size(); ++i) {
        // Look for patterns that indicate sprite headers
        // Pattern 1: width(2), height(2) followed by pixel-like data
        int16_t w = data[i] | (data[i + 1] << 8);
        int16_t h = data[i + 2] | (data[i + 3] << 8);

        // Check for reasonable dimensions
        if (w > 8 && w <= 320 && h > 8 && h <= 240) {
            // Check if following data looks like RLE or raw pixels
            bool hasFF = false;
            int uniqueBytes = 0;
            bool seen[256] = {false};

            for (size_t j = i + 4; j < std::min(i + 104, data.size()); ++j) {
                if (data[j] == 0xFF) hasFF = true;
                if (!seen[data[j]]) {
                    seen[data[j]] = true;
                    uniqueBytes++;
                }
            }

            // If we have variety of bytes and FF markers, likely sprite data
            if (uniqueBytes > 10 || hasFF) {
                DetectedSprite sprite;
                sprite.offset = baseOffset + static_cast<uint32_t>(i);
                sprite.width = w;
                sprite.height = h;
                sprite.hasRLE = hasFF;
                sprites.push_back(sprite);

                // Skip past this sprite to avoid overlapping detections
                i += 100;
            }
        }
    }

    return sprites;
}

void extractAllSprites(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file: " << datPath << "\n";
        return;
    }

    // Load palette
    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    // Create output directory
    fs::create_directories(outDir);

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();

    std::cout << "File size: " << fileSize << " bytes\n\n";

    // Read the sprite header area at 0x60000
    // This contains structured data about sprites
    std::cout << "=== Extracting Sprites from Header Table ===\n";

    file.seekg(0x60000);
    std::vector<uint8_t> headerArea(0x10000);  // Read 64KB
    file.read(reinterpret_cast<char*>(headerArea.data()), headerArea.size());

    // Parse header entries
    // Format appears to be: count(2), field(2), 0x0A(2), more fields, then offsets
    int spriteCount = 0;

    // Look for sprite definitions by scanning for structured patterns
    for (size_t offset = 0; offset < headerArea.size() - 32; offset += 4) {
        // Check if this looks like a sprite header
        uint16_t w1 = headerArea[offset] | (headerArea[offset + 1] << 8);
        uint16_t w2 = headerArea[offset + 2] | (headerArea[offset + 3] << 8);
        uint16_t w3 = headerArea[offset + 4] | (headerArea[offset + 5] << 8);

        // Look for pattern: small count, small count, 0x000A (10)
        if (w1 > 0 && w1 < 100 && w2 > 0 && w2 < 200 && w3 == 10) {
            // This might be a sprite index header
            uint16_t numSprites = w1;
            uint16_t numFrames = w2;

            // Read offsets that follow
            std::vector<uint32_t> offsets;
            size_t tableStart = offset + 18;  // Skip header fields

            for (int i = 0; i < numSprites && tableStart + i * 4 + 4 <= headerArea.size(); ++i) {
                size_t idx = tableStart + i * 4;
                uint32_t off = headerArea[idx] | (headerArea[idx + 1] << 8) |
                              (headerArea[idx + 2] << 16) | (headerArea[idx + 3] << 24);
                if (off > 0 && off < fileSize) {
                    offsets.push_back(off);
                }
            }

            if (!offsets.empty() && offsets.size() >= 3) {
                printf("Sprite table at 0x%06zX: %d sprites, %d frames, %zu offsets\n",
                       0x60000 + offset, numSprites, numFrames, offsets.size());

                // Extract first few sprites from this table
                for (size_t i = 0; i < std::min(offsets.size(), size_t(3)); ++i) {
                    printf("  Offset %zu: 0x%08X\n", i, offsets[i]);
                }
            }
        }
    }

    // Now try to extract individual sprites by scanning the data area
    std::cout << "\n=== Extracting Sprites from Data Area ===\n";

    // Read sprite data starting at 0x70000
    file.seekg(0x70000);
    std::vector<uint8_t> spriteData(std::min(size_t(0x100000), fileSize - 0x70000));
    file.read(reinterpret_cast<char*>(spriteData.data()), spriteData.size());

    // Try to extract sprites at known good offsets
    // Based on earlier analysis, data at 0x70000+ is RLE compressed
    std::vector<std::tuple<uint32_t, int, int>> knownSprites = {
        {0x0, 64, 64},      // Test at start of data
        {0x1000, 64, 64},
        {0x2000, 64, 64},
        {0x3000, 64, 64},
        {0x4000, 80, 80},
        {0x5000, 80, 80},
    };

    for (const auto& [dataOff, width, height] : knownSprites) {
        if (dataOff + 1000 > spriteData.size()) continue;

        // Decompress RLE
        std::vector<uint8_t> pixels;
        size_t expectedPixels = width * height;
        pixels.reserve(expectedPixels);

        size_t i = dataOff;
        while (pixels.size() < expectedPixels && i < spriteData.size()) {
            if (spriteData[i] == 0xFF && i + 2 < spriteData.size()) {
                uint8_t byte = spriteData[i + 1];
                uint8_t count = spriteData[i + 2];
                if (count == 0) count = 1;
                for (int j = 0; j < count && pixels.size() < expectedPixels; ++j) {
                    pixels.push_back(byte);
                }
                i += 3;
            } else {
                pixels.push_back(spriteData[i]);
                i++;
            }
        }

        // Pad if needed
        while (pixels.size() < expectedPixels) {
            pixels.push_back(0);
        }

        // Save as BMP
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/sprite_%06X_%dx%d.bmp",
                 outDir.c_str(), 0x70000 + dataOff, width, height);

        std::ofstream out(filename, std::ios::binary);
        if (!out) continue;

        // BMP header
        int rowSize = (width + 3) & ~3;
        int imageSize = rowSize * height;
        int bmpFileSize = 54 + 1024 + imageSize;

        uint8_t bmpHeader[54] = {
            'B', 'M',
            (uint8_t)(bmpFileSize), (uint8_t)(bmpFileSize >> 8),
            (uint8_t)(bmpFileSize >> 16), (uint8_t)(bmpFileSize >> 24),
            0, 0, 0, 0,
            54 + 4, 0, 0, 0,  // Offset to pixels (54 header + 1024 palette)
            40, 0, 0, 0,      // DIB header size
            (uint8_t)(width), (uint8_t)(width >> 8), (uint8_t)(width >> 16), (uint8_t)(width >> 24),
            (uint8_t)(height), (uint8_t)(height >> 8), (uint8_t)(height >> 16), (uint8_t)(height >> 24),
            1, 0,             // Planes
            8, 0,             // Bits per pixel
            0, 0, 0, 0,       // Compression
            (uint8_t)(imageSize), (uint8_t)(imageSize >> 8),
            (uint8_t)(imageSize >> 16), (uint8_t)(imageSize >> 24),
            0, 0, 0, 0,       // X pels/meter
            0, 0, 0, 0,       // Y pels/meter
            0, 1, 0, 0,       // Colors used (256)
            0, 0, 0, 0        // Important colors
        };

        // Fix offset to pixel data
        uint32_t pixelOffset = 54 + 1024;
        bmpHeader[10] = pixelOffset & 0xFF;
        bmpHeader[11] = (pixelOffset >> 8) & 0xFF;

        out.write(reinterpret_cast<char*>(bmpHeader), 54);
        out.write(reinterpret_cast<char*>(palette), 1024);

        // Write pixel data (BMP is bottom-up)
        std::vector<uint8_t> row(rowSize, 0);
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                row[x] = pixels[y * width + x];
            }
            out.write(reinterpret_cast<char*>(row.data()), rowSize);
        }

        spriteCount++;
        std::cout << "Saved: " << filename << "\n";
    }

    std::cout << "\nExtracted " << spriteCount << " sprites to " << outDir << "\n";

    // Now scan for sprites with embedded dimensions
    std::cout << "\n=== Scanning for Sprites with Embedded Dimensions ===\n";

    // Read more data from the sprite area
    file.seekg(0x50000);
    std::vector<uint8_t> scanData(std::min(size_t(0x30000), fileSize - 0x50000));
    file.read(reinterpret_cast<char*>(scanData.data()), scanData.size());

    auto detectedSprites = detectSprites(scanData, 0x50000);
    std::cout << "Detected " << detectedSprites.size() << " potential sprites\n";

    // Extract all detected sprites (up to 100)
    int extracted = 0;
    for (const auto& sprite : detectedSprites) {
        if (extracted >= 100) break;

        // Read sprite data
        file.seekg(sprite.offset + 4);  // Skip width/height header
        size_t dataSize = sprite.width * sprite.height * 2;  // Extra for RLE
        std::vector<uint8_t> sprData(dataSize);
        file.read(reinterpret_cast<char*>(sprData.data()), dataSize);

        // Decompress
        std::vector<uint8_t> pixels;
        size_t expectedPixels = sprite.width * sprite.height;
        pixels.reserve(expectedPixels);

        size_t i = 0;
        while (pixels.size() < expectedPixels && i < sprData.size()) {
            if (sprData[i] == 0xFF && i + 2 < sprData.size()) {
                uint8_t byte = sprData[i + 1];
                uint8_t count = sprData[i + 2];
                if (count == 0) count = 1;
                for (int j = 0; j < count && pixels.size() < expectedPixels; ++j) {
                    pixels.push_back(byte);
                }
                i += 3;
            } else {
                pixels.push_back(sprData[i]);
                i++;
            }
        }

        while (pixels.size() < expectedPixels) {
            pixels.push_back(0);
        }

        // Save
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/detected_%06X_%dx%d.bmp",
                 outDir.c_str(), sprite.offset, sprite.width, sprite.height);

        std::ofstream out(filename, std::ios::binary);
        if (!out) continue;

        int rowSize = (sprite.width + 3) & ~3;
        int imageSize = rowSize * sprite.height;
        int bmpFileSize = 54 + 1024 + imageSize;

        uint8_t bmpHeader[54] = {
            'B', 'M',
            (uint8_t)(bmpFileSize), (uint8_t)(bmpFileSize >> 8),
            (uint8_t)(bmpFileSize >> 16), (uint8_t)(bmpFileSize >> 24),
            0, 0, 0, 0,
            54 + 4, 0, 0, 0,
            40, 0, 0, 0,
            (uint8_t)(sprite.width), (uint8_t)(sprite.width >> 8),
            (uint8_t)(sprite.width >> 16), (uint8_t)(sprite.width >> 24),
            (uint8_t)(sprite.height), (uint8_t)(sprite.height >> 8),
            (uint8_t)(sprite.height >> 16), (uint8_t)(sprite.height >> 24),
            1, 0, 8, 0,
            0, 0, 0, 0,
            (uint8_t)(imageSize), (uint8_t)(imageSize >> 8),
            (uint8_t)(imageSize >> 16), (uint8_t)(imageSize >> 24),
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0, 0
        };

        uint32_t pixelOffset = 54 + 1024;
        bmpHeader[10] = pixelOffset & 0xFF;
        bmpHeader[11] = (pixelOffset >> 8) & 0xFF;

        out.write(reinterpret_cast<char*>(bmpHeader), 54);
        out.write(reinterpret_cast<char*>(palette), 1024);

        std::vector<uint8_t> row(rowSize, 0);
        for (int y = sprite.height - 1; y >= 0; --y) {
            for (int x = 0; x < sprite.width; ++x) {
                size_t srcIdx = y * sprite.width + x;
                row[x] = (srcIdx < pixels.size()) ? pixels[srcIdx] : 0;
            }
            out.write(reinterpret_cast<char*>(row.data()), rowSize);
        }

        std::cout << "Saved: " << filename << "\n";
        extracted++;
    }

    std::cout << "\nTotal extracted: " << (spriteCount + extracted) << " sprites\n";
}

void extractRealSprites(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    // Load palette
    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    fs::create_directories(outDir);

    NEResourceExtractor ne;
    if (!ne.open(datPath)) {
        std::cerr << "Failed to open NE\n";
        return;
    }

    auto resources = ne.listResources();

    // Find large CUSTOM_32513 index resources
    std::vector<Resource> indexRes;
    for (const auto& res : resources) {
        if (res.typeId == 0xFF01 && res.size > 40) {
            indexRes.push_back(res);
        }
    }

    std::cout << "Found " << indexRes.size() << " sprite index resources\n\n";

    // Base address for sprite data
    const uint32_t SPRITE_BASE = 0x10000;

    int totalExtracted = 0;

    for (const auto& idxRes : indexRes) {
        auto idxData = ne.extractResource(idxRes);
        if (idxData.size() < 24) continue;

        // Parse header
        uint16_t spriteCount = idxData[0] | (idxData[1] << 8);
        uint16_t frameCount = idxData[2] | (idxData[3] << 8);

        if (spriteCount == 0 || spriteCount > 1000) continue;

        // Parse offsets starting at byte 18
        std::vector<uint32_t> offsets;
        for (size_t i = 18; i + 3 < idxData.size() && offsets.size() < spriteCount + 1; i += 4) {
            uint32_t off = idxData[i] | (idxData[i+1] << 8) |
                          (idxData[i+2] << 16) | (idxData[i+3] << 24);
            if (off < 0x400000) {  // Reasonable limit
                offsets.push_back(off);
            }
        }

        if (offsets.size() < 2) continue;

        // Extract sprites using the offsets
        for (size_t i = 0; i < offsets.size() - 1 && totalExtracted < 50; ++i) {
            uint32_t spriteOff = SPRITE_BASE + offsets[i];
            uint32_t nextOff = SPRITE_BASE + offsets[i + 1];
            uint32_t spriteSize = nextOff - spriteOff;

            if (spriteSize < 10 || spriteSize > 100000) continue;

            // Read sprite data
            file.seekg(spriteOff);
            std::vector<uint8_t> spriteData(spriteSize);
            file.read(reinterpret_cast<char*>(spriteData.data()), spriteSize);

            // Try to find dimensions in the sprite header
            // Common patterns: width at byte 0, height at byte 2
            // Or there might be a marker first

            int width = 0, height = 0;

            // Check first few bytes for dimension-like values
            if (spriteData.size() >= 4) {
                uint16_t w1 = spriteData[0] | (spriteData[1] << 8);
                uint16_t h1 = spriteData[2] | (spriteData[3] << 8);

                // Check if these look like dimensions
                if (w1 > 0 && w1 <= 400 && h1 > 0 && h1 <= 400) {
                    // Validate: raw size would be w*h, compressed should be smaller
                    size_t rawSize = w1 * h1;
                    if (spriteSize < rawSize * 2) {
                        width = w1;
                        height = h1;
                    }
                }
            }

            // If no valid dimensions found, try to estimate from data size
            if (width == 0 || height == 0) {
                // Estimate based on compressed data size (assume ~50% compression)
                size_t estPixels = spriteSize * 2;
                int estDim = (int)sqrt((double)estPixels);
                width = height = std::max(16, std::min(estDim, 200));
            }

            // RLE decompress
            std::vector<uint8_t> pixels;
            size_t expectedPixels = width * height;
            pixels.reserve(expectedPixels);

            size_t dataStart = 0;  // Try from beginning (no header skip for now)

            for (size_t j = dataStart; pixels.size() < expectedPixels && j < spriteData.size(); ) {
                if (spriteData[j] == 0xFF && j + 2 < spriteData.size()) {
                    uint8_t byte = spriteData[j + 1];
                    uint8_t count = spriteData[j + 2];
                    if (count == 0) count = 1;
                    for (int k = 0; k < count && pixels.size() < expectedPixels; ++k) {
                        pixels.push_back(byte);
                    }
                    j += 3;
                } else {
                    pixels.push_back(spriteData[j]);
                    j++;
                }
            }

            while (pixels.size() < expectedPixels) {
                pixels.push_back(0);
            }

            // Save as BMP
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/spr_%05d_%03zu_%dx%d.bmp",
                     outDir.c_str(), idxRes.id, i, width, height);

            std::ofstream out(filename, std::ios::binary);
            if (!out) continue;

            int rowSize = (width + 3) & ~3;
            int imageSize = rowSize * height;
            int bmpSize = 54 + 1024 + imageSize;

            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = width;
            *(int32_t*)&bmpHeader[22] = height;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;
            *(uint32_t*)&bmpHeader[46] = 256;

            out.write(reinterpret_cast<char*>(bmpHeader), 54);
            out.write(reinterpret_cast<char*>(palette), 1024);

            std::vector<uint8_t> row(rowSize, 0);
            for (int y = height - 1; y >= 0; --y) {
                for (int x = 0; x < width; ++x) {
                    row[x] = pixels[y * width + x];
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }

            std::cout << "Extracted: " << filename << " (data offset 0x"
                      << std::hex << spriteOff << std::dec << ", "
                      << spriteSize << " bytes)\n";
            totalExtracted++;
        }
    }

    std::cout << "\nTotal extracted: " << totalExtracted << " sprites\n";
}

void traceSpritesOffsets(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();

    NEResourceExtractor ne;
    if (!ne.open(path)) {
        std::cerr << "Failed to open NE: " << ne.getLastError() << "\n";
        return;
    }

    std::cout << "=== Tracing Sprite Offsets ===\n\n";

    auto resources = ne.listResources();

    // Find CUSTOM_32513 resources (sprite index tables)
    std::vector<Resource> indexRes;
    for (const auto& res : resources) {
        if (res.typeId == 0xFF01 && res.size > 20) {
            indexRes.push_back(res);
        }
    }

    std::cout << "Found " << indexRes.size() << " large CUSTOM_32513 index resources\n\n";

    // Sort by size descending
    std::sort(indexRes.begin(), indexRes.end(), [](const Resource& a, const Resource& b) {
        return a.size > b.size;
    });

    // Examine the largest index tables
    for (size_t i = 0; i < std::min(size_t(3), indexRes.size()); ++i) {
        const auto& res = indexRes[i];
        auto data = ne.extractResource(res);

        std::cout << "=== Index Resource #" << res.id << " (" << data.size() << " bytes) ===\n";

        // Parse header
        if (data.size() < 20) continue;

        uint16_t field1 = data[0] | (data[1] << 8);
        uint16_t field2 = data[2] | (data[3] << 8);
        uint16_t constant = data[4] | (data[5] << 8);  // Usually 0x000A (10)
        uint16_t field4 = data[6] | (data[7] << 8);
        uint16_t field5 = data[8] | (data[9] << 8);

        printf("Header: %d, %d, %d, %d, %d\n", field1, field2, constant, field4, field5);

        // The offset table starts at byte 18 (after 18-byte header)
        std::cout << "Offset table:\n";
        std::vector<uint32_t> offsets;

        for (size_t j = 18; j + 3 < data.size(); j += 4) {
            uint32_t off = data[j] | (data[j+1] << 8) | (data[j+2] << 16) | (data[j+3] << 24);
            if (off > 0 && off < fileSize * 10) {  // Reasonable offset
                offsets.push_back(off);
                if (offsets.size() <= 20) {
                    printf("  [%2zu] 0x%08X (%u)\n", offsets.size() - 1, off, off);
                }
            }
        }

        if (offsets.size() > 20) {
            printf("  ... (%zu total offsets)\n", offsets.size());
        }

        // Try to find where these offsets point
        // They might be relative to some base address
        if (!offsets.empty()) {
            std::cout << "\nTrying to locate sprite data using offsets...\n";

            // Try different base addresses
            std::vector<uint32_t> bases = {0, 0x10000, 0x20000, 0x30000, 0x40000, 0x50000};

            for (uint32_t base : bases) {
                uint32_t testAddr = base + offsets[0];
                if (testAddr < fileSize) {
                    file.seekg(testAddr);
                    uint8_t sample[32];
                    file.read(reinterpret_cast<char*>(sample), 32);

                    printf("Base 0x%05X + offset[0] = 0x%08X: ", base, testAddr);
                    for (int k = 0; k < 16; ++k) printf("%02X ", sample[k]);
                    printf("\n");
                }
            }
        }

        std::cout << "\n";
    }

    // Now examine the segment data directly
    std::cout << "=== Examining NE Segment Data ===\n";

    // The segment table might tell us where actual data is
    file.seekg(0x3C);
    uint32_t neOffset;
    file.read(reinterpret_cast<char*>(&neOffset), 4);

    file.seekg(neOffset + 0x22);  // Segment table offset
    uint16_t segTableOff;
    file.read(reinterpret_cast<char*>(&segTableOff), 2);

    file.seekg(neOffset + 0x1C);  // Segment count
    uint16_t segCount;
    file.read(reinterpret_cast<char*>(&segCount), 2);

    file.seekg(neOffset + 0x32);  // Alignment shift
    uint16_t alignShift;
    file.read(reinterpret_cast<char*>(&alignShift), 2);

    std::cout << "NE header at 0x" << std::hex << neOffset << std::dec << "\n";
    std::cout << "Segments: " << segCount << ", align shift: " << alignShift << "\n\n";

    // Read segment table
    file.seekg(neOffset + segTableOff);
    for (int s = 0; s < segCount; ++s) {
        uint16_t segOff, segLen, segFlags, segMinAlloc;
        file.read(reinterpret_cast<char*>(&segOff), 2);
        file.read(reinterpret_cast<char*>(&segLen), 2);
        file.read(reinterpret_cast<char*>(&segFlags), 2);
        file.read(reinterpret_cast<char*>(&segMinAlloc), 2);

        uint32_t actualOff = static_cast<uint32_t>(segOff) << alignShift;
        uint32_t actualLen = segLen ? segLen : 0x10000;

        printf("Segment %d: offset=0x%08X, size=%u, flags=0x%04X\n",
               s + 1, actualOff, actualLen, segFlags);

        // Peek at segment content
        if (actualOff > 0 && actualOff < fileSize) {
            auto savePos = file.tellg();
            file.seekg(actualOff);
            uint8_t peek[32];
            file.read(reinterpret_cast<char*>(peek), 32);
            printf("  Data: ");
            for (int k = 0; k < 32; ++k) printf("%02X ", peek[k]);
            printf("\n");
            file.seekg(savePos);
        }
    }
}

void deepAnalyzeSprites(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << "\n";
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();

    std::cout << "=== Deep Sprite Format Analysis ===\n";
    std::cout << "File: " << path << "\n";
    std::cout << "Size: " << fileSize << " bytes\n\n";

    // Read a large chunk of data from the sprite area
    file.seekg(0x50000);
    std::vector<uint8_t> data(0x10000);  // 64KB
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    // Look for repeating structure patterns
    std::cout << "=== Analyzing data at 0x50000 ===\n";
    std::cout << "First 256 bytes:\n";
    for (int row = 0; row < 16; ++row) {
        printf("  %04X: ", row * 16);
        for (int col = 0; col < 16; ++col) {
            printf("%02X ", data[row * 16 + col]);
        }
        printf(" | ");
        for (int col = 0; col < 16; ++col) {
            char c = data[row * 16 + col];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("\n");
    }

    // Look for common sprite header patterns
    // Many sprite formats use: width(2), height(2), then data
    // Or: magic(2/4), width(2), height(2), flags, then data
    std::cout << "\n=== Looking for sprite header patterns ===\n";

    // Check if there's a master index at the start
    uint16_t first4Words[4];
    memcpy(first4Words, data.data(), 8);
    printf("First 4 words: %04X %04X %04X %04X\n",
           first4Words[0], first4Words[1], first4Words[2], first4Words[3]);

    // Look for offsets that might point to sprite data
    std::cout << "\n=== Looking for offset table ===\n";
    // Check if the data contains offsets (4-byte values that point within the file)
    for (size_t i = 0; i < 64; i += 4) {
        uint32_t val = data[i] | (data[i+1] << 8) | (data[i+2] << 16) | (data[i+3] << 24);
        if (val > 0x1000 && val < fileSize) {
            printf("  Offset at +%04zX: 0x%08X (%u)\n", i, val, val);
        }
    }

    // Now look at the CUSTOM_32514 resource data more carefully
    // These resources are at specific offsets and contain sprite metadata
    std::cout << "\n=== Examining CUSTOM_32514 resource content ===\n";

    NEResourceExtractor ne;
    if (!ne.open(path)) {
        std::cerr << "Failed to open as NE: " << ne.getLastError() << "\n";
        return;
    }

    auto resources = ne.listResources();

    // Find CUSTOM_32514 resources
    std::vector<Resource> spriteRes;
    for (const auto& res : resources) {
        if (res.typeId == 0xFF02) {
            spriteRes.push_back(res);
        }
    }

    std::cout << "Found " << spriteRes.size() << " CUSTOM_32514 resources\n\n";

    // Examine the first few in detail
    for (size_t i = 0; i < std::min(size_t(5), spriteRes.size()); ++i) {
        const auto& res = spriteRes[i];
        auto resData = ne.extractResource(res);

        std::cout << "Resource #" << res.id << " (size=" << resData.size()
                  << ", file_offset=0x" << std::hex << res.offset << std::dec << "):\n";

        // Dump all bytes
        std::cout << "  Data: ";
        for (size_t j = 0; j < resData.size(); ++j) {
            printf("%02X ", resData[j]);
            if ((j + 1) % 16 == 0 && j + 1 < resData.size()) {
                std::cout << "\n        ";
            }
        }
        std::cout << "\n\n";
    }

    // Now look at where the actual pixel data might be
    std::cout << "=== Examining potential sprite data areas ===\n";

    // Check multiple offsets to find where actual graphics are
    std::vector<uint32_t> testOffsets = {0x10000, 0x20000, 0x30000, 0x40000,
                                          0x50000, 0x60000, 0x70000, 0x80000};

    for (uint32_t off : testOffsets) {
        if (off >= fileSize) continue;

        file.seekg(off);
        uint8_t sample[64];
        file.read(reinterpret_cast<char*>(sample), 64);

        // Count unique bytes and look for patterns
        int unique = 0;
        int ffCount = 0;
        int zeroCount = 0;
        bool seen[256] = {false};

        for (int j = 0; j < 64; ++j) {
            if (!seen[sample[j]]) {
                seen[sample[j]] = true;
                unique++;
            }
            if (sample[j] == 0xFF) ffCount++;
            if (sample[j] == 0x00) zeroCount++;
        }

        printf("0x%05X: unique=%2d FF=%2d zeros=%2d  ", off, unique, ffCount, zeroCount);

        // Check for specific patterns
        if (sample[0] == 'L' && sample[1] == 'T') {
            printf("[LT TABLE]");
        } else if (sample[0] == 'A' && sample[1] == 'S' && sample[2] == 'E' && sample[3] == 'Q') {
            printf("[ASEQ]");
        } else if (ffCount > 10) {
            printf("[RLE DATA?]");
        } else if (zeroCount > 40) {
            printf("[SPARSE/HEADER]");
        } else if (unique > 30) {
            printf("[VARIED DATA]");
        }

        // Show first 32 bytes
        printf("\n  ");
        for (int j = 0; j < 32; ++j) {
            printf("%02X ", sample[j]);
        }
        printf("\n");
    }

    // Try to find where actual graphics start by looking for high-entropy data
    std::cout << "\n=== Scanning for graphics data start ===\n";

    for (uint32_t scanOff = 0x1000; scanOff < std::min((uint32_t)fileSize, 0x100000u); scanOff += 0x1000) {
        file.seekg(scanOff);
        uint8_t sample[256];
        file.read(reinterpret_cast<char*>(sample), 256);

        // Calculate entropy-like metric
        int unique = 0;
        bool seen[256] = {false};
        for (int j = 0; j < 256; ++j) {
            if (!seen[sample[j]]) {
                seen[sample[j]] = true;
                unique++;
            }
        }

        // High unique count with no obvious structure = likely graphics
        if (unique > 100) {
            printf("High entropy at 0x%05X: %d unique bytes\n", scanOff, unique);
        }
    }
}

void analyzeSpriteIndex(const std::string& path) {
    NEResourceExtractor ne;

    if (!ne.open(path)) {
        std::cerr << "Error: " << ne.getLastError() << "\n";
        return;
    }

    auto resources = ne.listResources();

    std::cout << "=== Analyzing Sprite Index (CUSTOM_32513) ===\n\n";

    // Group resources by type
    std::map<uint16_t, std::vector<Resource>> byType;
    for (const auto& res : resources) {
        byType[res.typeId].push_back(res);
    }

    // Look at CUSTOM_32513 (0xFF01) which should be sprite indices
    auto it = byType.find(0xFF01);
    if (it == byType.end()) {
        std::cout << "No CUSTOM_32513 resources found\n";
        return;
    }

    std::cout << "Found " << it->second.size() << " CUSTOM_32513 resources\n\n";

    // Group by size to understand different record types
    std::map<size_t, std::vector<Resource>> bySize;
    for (const auto& res : it->second) {
        bySize[res.size].push_back(res);
    }

    std::cout << "Size distribution:\n";
    for (const auto& [size, resList] : bySize) {
        std::cout << "  " << size << " bytes: " << resList.size() << " resources\n";
    }
    std::cout << "\n";

    // Analyze the larger records (189, 138, etc.) which likely contain sprite tables
    std::cout << "=== Analyzing Larger Index Records ===\n";

    std::vector<Resource> sorted = it->second;
    std::sort(sorted.begin(), sorted.end(), [](const Resource& a, const Resource& b) {
        return a.size > b.size;
    });

    int count = 0;
    for (const auto& res : sorted) {
        if (res.size < 20) continue;  // Skip small records
        if (count++ >= 5) break;

        auto data = ne.extractResource(res);
        if (data.empty()) continue;

        std::cout << "Resource #" << res.id << " (" << data.size() << " bytes):\n";

        // Show raw data
        std::cout << "  Raw: ";
        for (size_t i = 0; i < std::min(data.size(), size_t(64)); ++i) {
            printf("%02X ", data[i]);
            if ((i + 1) % 16 == 0 && i < 63) std::cout << "\n       ";
        }
        std::cout << "\n";

        // Try to parse as sprite table
        // Common sprite header: width(2), height(2), offset(4) or similar
        if (data.size() >= 8) {
            std::cout << "  Parsed as 16-bit words:\n    ";
            for (size_t i = 0; i + 1 < std::min(data.size(), size_t(32)); i += 2) {
                int16_t val = data[i] | (data[i + 1] << 8);
                printf("%6d ", val);
                if ((i + 2) % 16 == 0) std::cout << "\n    ";
            }
            std::cout << "\n";
        }

        // Try parsing as 4-byte records
        if (data.size() >= 16) {
            std::cout << "  As 4-byte records (could be x,y or w,h pairs):\n";
            for (size_t i = 0; i + 3 < std::min(data.size(), size_t(48)); i += 4) {
                int16_t val1 = data[i] | (data[i + 1] << 8);
                int16_t val2 = data[i + 2] | (data[i + 3] << 8);
                printf("    [%zu]: %d, %d\n", i / 4, val1, val2);
            }
        }

        std::cout << "\n";
    }

    // Look at 2-byte records (most common)
    std::cout << "=== Analyzing 2-byte Index Records ===\n";
    for (const auto& [size, resList] : bySize) {
        if (size != 2) continue;

        std::cout << "First 20 records of size 2:\n";
        int shown = 0;
        for (const auto& res : resList) {
            if (shown++ >= 20) break;

            auto data = ne.extractResource(res);
            if (data.size() >= 2) {
                uint16_t val = data[0] | (data[1] << 8);
                printf("  ID %5d: value = %5d (0x%04X)\n", res.id, val, val);
            }
        }
        break;
    }
}

void analyzeRLEFormat(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << "\n";
        return;
    }

    std::cout << "=== Analyzing RLE/Compression Format ===\n\n";

    // Look at data around 0x50000 where sprites appear to be
    std::vector<uint32_t> checkOffsets = {0x50000, 0x51000, 0x52000, 0x53000, 0x54000, 0x55000};

    for (uint32_t off : checkOffsets) {
        file.seekg(off);
        uint8_t data[256];
        file.read(reinterpret_cast<char*>(data), 256);

        std::cout << "At 0x" << std::hex << off << std::dec << ":\n";

        // Count FF occurrences and analyze patterns
        int ffCount = 0;
        std::vector<std::tuple<int, uint8_t, uint8_t>> rleOps;  // offset, byte, count

        for (int i = 0; i < 254; ++i) {
            if (data[i] == 0xFF) {
                ffCount++;
                rleOps.push_back({i, data[i + 1], data[i + 2]});
            }
        }

        std::cout << "  FF count: " << ffCount << "\n";
        if (!rleOps.empty()) {
            std::cout << "  First 5 FF patterns:\n";
            for (int i = 0; i < std::min(5, (int)rleOps.size()); ++i) {
                auto& [pos, byte, count] = rleOps[i];
                printf("    @%d: FF %02X %02X (repeat 0x%02X %d times)\n",
                       pos, byte, count, byte, count);
            }
        }

        // Show first 64 bytes
        std::cout << "  Data: ";
        for (int i = 0; i < 64; ++i) {
            printf("%02X ", data[i]);
            if ((i + 1) % 32 == 0) std::cout << "\n        ";
        }
        std::cout << "\n\n";
    }

    // Look at what's before a sprite to find headers
    std::cout << "=== Looking for Sprite Headers ===\n";

    // Check 0x60000 area which showed structured data
    file.seekg(0x60000);
    uint8_t header[64];
    file.read(reinterpret_cast<char*>(header), 64);

    std::cout << "Data at 0x60000 (possible sprite table):\n  ";
    for (int i = 0; i < 64; ++i) {
        printf("%02X ", header[i]);
        if ((i + 1) % 16 == 0) std::cout << "\n  ";
    }
    std::cout << "\n";

    // Parse as potential sprite header
    std::cout << "Parsed as sprite header:\n";
    uint16_t f1 = header[0] | (header[1] << 8);
    uint16_t f2 = header[2] | (header[3] << 8);
    uint16_t f3 = header[4] | (header[5] << 8);
    uint16_t f4 = header[6] | (header[7] << 8);
    uint16_t f5 = header[8] | (header[9] << 8);
    printf("  Fields: %d %d %d %d %d\n", f1, f2, f3, f4, f5);
}

void analyzeRawFileStructure(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << "\n";
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();

    std::cout << "=== Raw File Structure Analysis ===\n";
    std::cout << "File size: " << fileSize << " bytes (0x" << std::hex << fileSize << std::dec << ")\n\n";

    // The NE structure ends early, most of the file is raw data
    // Let's map out what's in the file by scanning for patterns

    std::cout << "=== Scanning for Data Section Boundaries ===\n";

    // Look at key offsets
    std::vector<uint32_t> checkOffsets = {
        0x3000,   // ASEQ table area
        0x4000,   // LT table area
        0x5000,   // End of LT / start of something else
        0x10000,  // 64KB boundary
        0x20000,  // 128KB
        0x30000,
        0x40000,
        0x50000,
        0x60000,  // Potential graphics start
        0x70000,
        0x80000,
        0x90000,
        0xA0000,
        0x100000, // 1MB
        0x200000, // 2MB
        0x300000, // 3MB
    };

    for (uint32_t off : checkOffsets) {
        if (off >= fileSize) continue;

        file.seekg(off);
        uint8_t sample[64];
        file.read(reinterpret_cast<char*>(sample), 64);

        // Analyze the sample
        int uniqueBytes = 0;
        int zeroCount = 0;
        bool seen[256] = {false};
        for (int i = 0; i < 64; ++i) {
            if (!seen[sample[i]]) {
                seen[sample[i]] = true;
                uniqueBytes++;
            }
            if (sample[i] == 0) zeroCount++;
        }

        // Check for text
        bool hasText = false;
        for (int i = 0; i < 60; ++i) {
            if (sample[i] >= 0x20 && sample[i] < 0x7F) {
                bool isPrintable = true;
                for (int j = 0; j < 4 && i + j < 64; ++j) {
                    if (sample[i+j] < 0x20 || sample[i+j] >= 0x7F) {
                        isPrintable = false;
                        break;
                    }
                }
                if (isPrintable) {
                    hasText = true;
                    break;
                }
            }
        }

        printf("0x%06X: unique=%2d zeros=%2d ", off, uniqueBytes, zeroCount);

        // Show first 32 bytes
        for (int i = 0; i < 32; ++i) {
            printf("%02X ", sample[i]);
        }

        // Classify
        if (zeroCount > 50) {
            std::cout << "[EMPTY/SPARSE]";
        } else if (uniqueBytes < 10 && !hasText) {
            std::cout << "[STRUCTURED]";
        } else if (hasText) {
            std::cout << "[TEXT/CODE]";
        } else if (uniqueBytes > 30) {
            std::cout << "[GRAPHICS?]";
        }

        std::cout << "\n";
    }

    // Look for specific markers that indicate section boundaries
    std::cout << "\n=== Looking for Section Markers ===\n";

    file.seekg(0);
    std::vector<uint8_t> fullFile(std::min(fileSize, (size_t)0x400000));
    file.read(reinterpret_cast<char*>(fullFile.data()), fullFile.size());

    // Find runs of similar data (indicating section boundaries)
    std::cout << "Large zero runs (potential section padding):\n";
    int zeroRunStart = -1;
    int zeroRunLen = 0;

    for (size_t i = 0; i < fullFile.size(); ++i) {
        if (fullFile[i] == 0) {
            if (zeroRunStart < 0) {
                zeroRunStart = i;
                zeroRunLen = 1;
            } else {
                zeroRunLen++;
            }
        } else {
            if (zeroRunLen >= 256) {  // 256+ consecutive zeros
                printf("  0x%06X - 0x%06X (%d bytes)\n",
                       zeroRunStart, (int)(zeroRunStart + zeroRunLen), zeroRunLen);
            }
            zeroRunStart = -1;
            zeroRunLen = 0;
        }
    }

    // Look at the data right after NE overhead (around 0x4B00)
    std::cout << "\n=== Data Analysis After NE Structure ===\n";

    // Check what comes after the last resource table entry
    // The actual graphics likely start at some aligned offset

    // Try to identify sprite data by looking for patterns
    std::cout << "\nLooking for sprite-like structures (width/height pairs):\n";

    for (size_t i = 0x50000; i < std::min((size_t)0x60000, fullFile.size()); i += 2) {
        uint16_t w = fullFile[i] | (fullFile[i+1] << 8);
        if (i + 3 < fullFile.size()) {
            uint16_t h = fullFile[i+2] | (fullFile[i+3] << 8);
            // Look for reasonable sprite dimensions
            if (w > 8 && w <= 320 && h > 8 && h <= 200) {
                size_t expectedSize = (size_t)w * h;
                // Check if data after looks like pixels (variety of values)
                if (i + 4 + expectedSize <= fullFile.size()) {
                    int uniq = 0;
                    bool seen[256] = {false};
                    for (size_t j = 0; j < std::min((size_t)100, expectedSize); ++j) {
                        if (!seen[fullFile[i + 4 + j]]) {
                            seen[fullFile[i + 4 + j]] = true;
                            uniq++;
                        }
                    }
                    if (uniq > 15) {  // Likely pixel data
                        printf("  0x%06zX: %dx%d (expected %zu bytes, uniq=%d)\n",
                               i, w, h, expectedSize, uniq);
                    }
                }
            }
        }
    }
}

void analyzeSpriteResource(const std::string& path) {
    NEResourceExtractor ne;

    if (!ne.open(path)) {
        std::cerr << "Error: " << ne.getLastError() << "\n";
        return;
    }

    auto resources = ne.listResources();

    std::cout << "=== Analyzing Sprite Resources ===\n\n";

    // Group resources by type
    std::map<uint16_t, std::vector<Resource>> byType;
    for (const auto& res : resources) {
        byType[res.typeId].push_back(res);
    }

    // Show all types
    std::cout << "Resource types found:\n";
    for (const auto& [typeId, resList] : byType) {
        std::cout << "  Type 0x" << std::hex << typeId << std::dec
                  << " (" << resList[0].typeName << "): " << resList.size() << " resources\n";
    }
    std::cout << "\n";

    // Look at CUSTOM_32514 (0xFF02) resources in detail
    std::cout << "=== CUSTOM_32514 (0xFF02) Sprite Data Analysis ===\n";
    auto it = byType.find(0xFF02);
    if (it != byType.end()) {
        std::cout << "Found " << it->second.size() << " CUSTOM_32514 resources\n\n";

        // Show ID range
        uint16_t minId = 0xFFFF, maxId = 0;
        for (const auto& res : it->second) {
            minId = std::min(minId, res.id);
            maxId = std::max(maxId, res.id);
        }
        std::cout << "ID range: " << minId << " - " << maxId;
        std::cout << " (0x" << std::hex << minId << " - 0x" << maxId << std::dec << ")\n\n";

        // Examine first 5 resources
        int count = 0;
        for (const auto& res : it->second) {
            if (count++ >= 10) break;

            auto data = ne.extractResource(res);
            if (data.empty()) continue;

            std::cout << "Resource #" << res.id << " (0x" << std::hex << res.id << std::dec
                      << "), size=" << data.size() << " bytes, offset=0x"
                      << std::hex << res.offset << std::dec << "\n";

            // Show first 48 bytes
            std::cout << "  Header: ";
            for (size_t i = 0; i < std::min(data.size(), size_t(48)); ++i) {
                printf("%02X ", data[i]);
                if ((i + 1) % 16 == 0 && i < 47) std::cout << "\n          ";
            }
            std::cout << "\n";

            // Try to parse as sprite header
            if (data.size() >= 8) {
                // Common sprite formats have width/height at start
                uint16_t w1 = data[0] | (data[1] << 8);
                uint16_t h1 = data[2] | (data[3] << 8);
                int16_t sw1 = static_cast<int16_t>(w1);
                int16_t sh1 = static_cast<int16_t>(h1);

                std::cout << "  Possible dimensions (bytes 0-3): " << sw1 << " x " << sh1 << "\n";

                // Check if it could be RLE or raw
                size_t expectedRaw = (size_t)std::abs(sw1) * std::abs(sh1);
                if (expectedRaw > 0 && expectedRaw < 500000) {
                    float ratio = (float)(data.size() - 4) / expectedRaw;
                    std::cout << "  If " << std::abs(sw1) << "x" << std::abs(sh1)
                              << " raw: expected " << expectedRaw << " bytes, got "
                              << (data.size() - 4) << " (" << (int)(ratio * 100) << "%)\n";
                }
            }
            std::cout << "\n";
        }

        // Look at some larger resources
        std::cout << "\n=== Larger CUSTOM_32514 Resources ===\n";
        std::vector<Resource> sorted = it->second;
        std::sort(sorted.begin(), sorted.end(), [](const Resource& a, const Resource& b) {
            return a.size > b.size;
        });

        count = 0;
        for (const auto& res : sorted) {
            if (count++ >= 5) break;

            auto data = ne.extractResource(res);
            if (data.empty()) continue;

            std::cout << "Resource #" << res.id << ", size=" << data.size() << " bytes\n";
            std::cout << "  First 32: ";
            for (size_t i = 0; i < std::min(data.size(), size_t(32)); ++i) {
                printf("%02X ", data[i]);
            }
            std::cout << "\n";

            // Parse potential sprite header
            if (data.size() >= 8) {
                int16_t w = data[0] | (data[1] << 8);
                int16_t h = data[2] | (data[3] << 8);
                std::cout << "  Dims: " << w << " x " << h << "\n";
            }
            std::cout << "\n";
        }
    }

    // Also look at CUSTOM_32513 (0xFF01)
    std::cout << "\n=== CUSTOM_32513 (0xFF01) Index Data Analysis ===\n";
    it = byType.find(0xFF01);
    if (it != byType.end()) {
        std::cout << "Found " << it->second.size() << " CUSTOM_32513 resources\n\n";

        int count = 0;
        for (const auto& res : it->second) {
            if (count++ >= 10) break;

            auto data = ne.extractResource(res);
            if (data.empty()) continue;

            std::cout << "Resource #" << res.id << ", size=" << data.size() << ": ";
            for (size_t i = 0; i < std::min(data.size(), size_t(32)); ++i) {
                printf("%02X ", data[i]);
            }
            std::cout << "\n";
        }
    }
}

void analyzeASEQ(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << "\n";
        return;
    }

    std::cout << "=== Analyzing ASEQ (Animation Sequence) Table ===\n\n";

    // ASEQ table starts at 0x3206
    file.seekg(0x3206);

    std::cout << "ASEQ entries starting at 0x3206:\n";
    std::cout << "Offset    | Raw Data (12 bytes)                    | Interpretation\n";
    std::cout << "----------|----------------------------------------|---------------\n";

    for (int i = 0; i < 30; ++i) {
        uint32_t offset = 0x3206 + i * 12;
        file.seekg(offset);

        uint8_t entry[12];
        file.read(reinterpret_cast<char*>(entry), 12);

        // Check for ASEQ marker
        if (entry[0] != 'A' || entry[1] != 'S' || entry[2] != 'E' || entry[3] != 'Q') {
            std::cout << "Non-ASEQ entry at 0x" << std::hex << offset << std::dec << "\n";
            break;
        }

        printf("0x%06X  | ", offset);
        for (int j = 0; j < 12; ++j) {
            printf("%02X ", entry[j]);
        }
        printf("| ");

        // Parse ASEQ entry
        // Bytes 0-3: "ASEQ"
        // Bytes 4-5: might be count or type
        // Bytes 6-7: might be frame count
        // Bytes 8-9: might be ID or offset
        // Bytes 10-11: might be flags
        uint16_t field1 = entry[4] | (entry[5] << 8);
        uint16_t field2 = entry[6] | (entry[7] << 8);
        uint16_t field3 = entry[8] | (entry[9] << 8);
        uint16_t field4 = entry[10] | (entry[11] << 8);

        printf("F1=%04X F2=%04X F3=%04X F4=%04X\n", field1, field2, field3, field4);
    }

    // Look at what's right before 0x3206 to find the table header
    std::cout << "\n=== Checking area before ASEQ table (0x3000-0x3206) ===\n";
    file.seekg(0x3000);
    uint8_t preTable[0x206];
    file.read(reinterpret_cast<char*>(preTable), 0x206);

    // Look for any structure
    std::cout << "First 64 bytes at 0x3000:\n  ";
    for (int i = 0; i < 64; ++i) {
        printf("%02X ", preTable[i]);
        if ((i + 1) % 16 == 0) std::cout << "\n  ";
    }
    std::cout << "\n";

    // Find the header right before ASEQ entries
    std::cout << "Last 32 bytes before 0x3206:\n  ";
    for (int i = 0x1E6; i < 0x206; ++i) {
        printf("%02X ", preTable[i]);
    }
    std::cout << "\n";

    // Now look at the LT entries and see what Field3 (FF02) means
    std::cout << "\n=== LT Entry Analysis - Detailed ===\n";
    file.seekg(0x4000);

    std::cout << "First 20 LT entries:\n";
    std::cout << "Offset    | Marker | Zero  | Size? | Type  | ID    | End\n";
    std::cout << "----------|--------|-------|-------|-------|-------|-----\n";

    for (int i = 0; i < 20; ++i) {
        uint8_t entry[12];
        file.read(reinterpret_cast<char*>(entry), 12);

        printf("0x%06X  | %c%c     | %02X%02X  | %02X%02X  | %02X%02X  | %02X%02X  | %c%c\n",
               0x4000 + i * 12,
               entry[0], entry[1],
               entry[2], entry[3],
               entry[4], entry[5],
               entry[6], entry[7],
               entry[8], entry[9],
               entry[10], entry[11]);
    }

    // Now let's see what's at 0x3800 area
    std::cout << "\n=== Resource Reference Table at 0x3800 ===\n";
    file.seekg(0x3800);

    std::cout << "Data at 0x3800 (checking for resource references):\n";
    for (int i = 0; i < 10; ++i) {
        uint8_t entry[12];
        file.read(reinterpret_cast<char*>(entry), 12);

        uint16_t type = entry[0] | (entry[1] << 8);
        uint32_t value1 = entry[2] | (entry[3] << 8) | (entry[4] << 16) | (entry[5] << 24);
        uint16_t field2 = entry[6] | (entry[7] << 8);
        uint16_t field3 = entry[8] | (entry[9] << 8);

        printf("  0x%04X: type=%04X val1=%08X f2=%04X f3=%04X [",
               0x3800 + i * 12, type, value1, field2, field3);
        for (int j = 0; j < 12; ++j) printf("%02X ", entry[j]);
        printf("]\n");
    }

    // Analyze the sprite ID references
    std::cout << "\n=== Cross-referencing LT IDs with NE Resources ===\n";

    // Read some LT IDs
    file.seekg(0x4000);
    std::vector<uint16_t> ltIds;
    for (int i = 0; i < 20; ++i) {
        uint8_t entry[12];
        file.read(reinterpret_cast<char*>(entry), 12);
        if (entry[0] == 'L' && entry[1] == 'T') {
            uint16_t id = entry[8] | (entry[9] << 8);
            ltIds.push_back(id);
        }
    }

    std::cout << "First 20 LT IDs: ";
    for (auto id : ltIds) {
        printf("0x%04X ", id);
    }
    std::cout << "\n";

    // These IDs (0x1034, 0x1035...) should correspond to resources in the NE file
    // Let's cross-reference with what NE resources we have
    std::cout << "\nThese IDs likely reference CUSTOM_32514 (0xFF02) resources.\n";
    std::cout << "The LT table maps: LT entry index -> sprite resource ID\n";
}

void analyzeEntities(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << "\n";
        return;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    std::cout << "File size: " << fileSize << " bytes\n\n";

    // Check for "LT" lookup table entries at various offsets
    std::cout << "=== Scanning for Lookup Table (LT) entries ===\n";

    // Look for LT pattern starting at 0x4000
    file.seekg(0x4000);
    uint8_t buffer[16];
    int ltCount = 0;
    std::vector<std::pair<uint32_t, uint16_t>> ltEntries; // offset, ID

    for (uint32_t offset = 0x4000; offset < std::min((size_t)0x10000, fileSize); offset += 12) {
        file.seekg(offset);
        file.read(reinterpret_cast<char*>(buffer), 12);

        // Check for "LT" marker
        if (buffer[0] == 'L' && buffer[1] == 'T') {
            uint16_t id = buffer[8] | (buffer[9] << 8);
            ltEntries.push_back({offset, id});
            ltCount++;

            if (ltCount <= 10 || ltCount % 50 == 0) {
                printf("  LT entry at 0x%04X: ID=0x%04X (%d)  ", offset, id, id);
                for (int i = 0; i < 12; ++i) printf("%02X ", buffer[i]);
                printf("\n");
            }
        }
    }
    std::cout << "Found " << ltCount << " LT entries\n\n";

    // Analyze LT entry structure in detail
    if (!ltEntries.empty()) {
        std::cout << "=== LT Entry Structure Analysis ===\n";
        file.seekg(ltEntries[0].first);
        uint8_t entry[16];
        file.read(reinterpret_cast<char*>(entry), 16);

        std::cout << "First LT entry breakdown:\n";
        std::cout << "  [0-1]  Marker: '" << (char)entry[0] << (char)entry[1] << "'\n";
        std::cout << "  [2-3]  Field1: 0x" << std::hex << (entry[2] | (entry[3] << 8)) << std::dec << "\n";
        std::cout << "  [4-5]  Field2: 0x" << std::hex << (entry[4] | (entry[5] << 8)) << std::dec << "\n";
        std::cout << "  [6-7]  Field3: 0x" << std::hex << (entry[6] | (entry[7] << 8)) << std::dec << "\n";
        std::cout << "  [8-9]  ID:     0x" << std::hex << (entry[8] | (entry[9] << 8)) << std::dec << "\n";
        std::cout << "  [10-11] DD?:   " << (char)entry[10] << (char)entry[11] << "\n";

        // Check ID range
        uint16_t minId = 0xFFFF, maxId = 0;
        for (const auto& [off, id] : ltEntries) {
            minId = std::min(minId, id);
            maxId = std::max(maxId, id);
        }
        std::cout << "\nID range: 0x" << std::hex << minId << " - 0x" << maxId << std::dec;
        std::cout << " (" << minId << " - " << maxId << ")\n\n";
    }

    // Look for entity placement records
    std::cout << "=== Scanning for Entity Records ===\n";

    // Try to find structured data that looks like entity placements
    // Entity records might have: type(2), x(2), y(2), width(2), height(2), ...

    // Check area around 0x5000 for different data format
    file.seekg(0x5000);
    std::cout << "Data at 0x5000:\n";
    uint8_t dataBlock[64];
    file.read(reinterpret_cast<char*>(dataBlock), 64);
    std::cout << "  ";
    for (int i = 0; i < 64; ++i) {
        printf("%02X ", dataBlock[i]);
        if ((i + 1) % 16 == 0) std::cout << "\n  ";
    }
    std::cout << "\n";

    // Look for patterns that might be coordinate/dimension data
    std::cout << "\n=== Looking for coordinate/dimension patterns ===\n";
    file.seekg(0x5000);

    // Read 256 bytes and look for repeating structures
    uint8_t scanBuf[512];
    file.read(reinterpret_cast<char*>(scanBuf), 512);

    // Check for 0xFFFF boundaries (common in game data)
    std::cout << "0xFFFF markers found at offsets (relative to 0x5000):\n  ";
    int ffCount = 0;
    for (int i = 0; i < 510; i += 2) {
        if (scanBuf[i] == 0xFF && scanBuf[i+1] == 0xFF) {
            printf("0x%03X ", i);
            ffCount++;
            if (ffCount % 16 == 0) std::cout << "\n  ";
        }
    }
    std::cout << "\n\n";

    // Try to find actual sprite data sections
    std::cout << "=== Looking for sprite pixel data ===\n";

    // Graphics typically start around 0x60000 based on earlier analysis
    std::vector<uint32_t> checkOffsets = {0x60000, 0x70000, 0x80000, 0x90000, 0xA0000};

    for (uint32_t checkOff : checkOffsets) {
        if (checkOff >= fileSize) continue;

        file.seekg(checkOff);
        uint8_t sample[32];
        file.read(reinterpret_cast<char*>(sample), 32);

        std::cout << "At 0x" << std::hex << checkOff << std::dec << ": ";
        for (int i = 0; i < 32; ++i) {
            printf("%02X ", sample[i]);
        }

        // Check if this looks like pixel data (variety of values, no obvious structure)
        int uniqueValues = 0;
        bool seen[256] = {false};
        for (int i = 0; i < 32; ++i) {
            if (!seen[sample[i]]) {
                seen[sample[i]] = true;
                uniqueValues++;
            }
        }
        std::cout << " (unique: " << uniqueValues << ")\n";
    }

    // Try to correlate LT IDs with file offsets
    if (!ltEntries.empty()) {
        std::cout << "\n=== Attempting to map LT IDs to data offsets ===\n";

        // The ID values (0x1034, etc.) might be:
        // 1. Direct offsets (unlikely - values too small)
        // 2. Index into another table
        // 3. Segment:offset pairs
        // 4. Resource IDs

        uint16_t firstId = ltEntries[0].second;
        std::cout << "First ID: 0x" << std::hex << firstId << std::dec << "\n";
        std::cout << "Trying interpretations:\n";
        std::cout << "  As offset: 0x" << std::hex << firstId << " = " << std::dec << firstId << " bytes\n";
        std::cout << "  As offset x16: 0x" << std::hex << (firstId * 16) << " = " << std::dec << (firstId * 16) << " bytes\n";
        std::cout << "  As offset x256: 0x" << std::hex << (firstId * 256) << " = " << std::dec << (firstId * 256) << " bytes\n";

        // Check if the low byte might be an index and high byte a bank/section
        uint8_t lowByte = firstId & 0xFF;
        uint8_t highByte = (firstId >> 8) & 0xFF;
        std::cout << "  High byte: 0x" << std::hex << (int)highByte << ", Low byte: 0x" << (int)lowByte << std::dec << "\n";

        // If IDs are sequential, they might just be sprite indices
        bool sequential = true;
        for (size_t i = 1; i < ltEntries.size() && i < 10; ++i) {
            if (ltEntries[i].second != ltEntries[i-1].second + 1) {
                sequential = false;
                break;
            }
        }
        std::cout << "  IDs appear to be " << (sequential ? "sequential" : "non-sequential") << "\n";
    }

    // Look for a potential sprite offset table
    std::cout << "\n=== Searching for sprite offset table ===\n";

    // Check common locations for offset tables
    std::vector<uint32_t> tableSearchOffsets = {0x2000, 0x2600, 0x3000, 0x3800};

    for (uint32_t searchOff : tableSearchOffsets) {
        if (searchOff >= fileSize) continue;

        file.seekg(searchOff);
        uint32_t values[8];
        file.read(reinterpret_cast<char*>(values), 32);

        std::cout << "At 0x" << std::hex << searchOff << std::dec << ":\n  ";
        bool looksLikeOffsets = true;
        for (int i = 0; i < 8; ++i) {
            printf("0x%08X ", values[i]);
            // Valid offsets would be within file and reasonably aligned
            if (values[i] > fileSize || (values[i] != 0 && values[i] < 0x100)) {
                looksLikeOffsets = false;
            }
        }
        std::cout << (looksLikeOffsets ? " [POTENTIAL OFFSETS]" : "") << "\n";
    }

    // Additional: look for ASEQ (animation sequence) or other markers
    std::cout << "\n=== Searching for format markers ===\n";
    file.seekg(0);
    std::vector<uint8_t> fullFile(std::min(fileSize, size_t(0x100000)));
    file.read(reinterpret_cast<char*>(fullFile.data()), fullFile.size());

    // Common markers to look for
    std::vector<std::pair<std::string, std::string>> markers = {
        {"ASEQ", "Animation sequence"},
        {"RIFF", "RIFF container"},
        {"BM", "Bitmap"},
        {"MZ", "DOS executable"},
        {"NE", "NE header"},
        {"SPRT", "Sprite data"},
        {"GRPH", "Graphics"}
    };

    for (const auto& [marker, desc] : markers) {
        for (size_t i = 0; i < fullFile.size() - marker.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < marker.size(); ++j) {
                if (fullFile[i + j] != (uint8_t)marker[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                std::cout << "Found '" << marker << "' (" << desc << ") at 0x"
                          << std::hex << i << std::dec << "\n";
            }
        }
    }
}

bool validateGame(const std::string& gamePath) {
    std::cout << "Validating game installation at: " << gamePath << "\n\n";

    std::vector<std::string> requiredFiles = {
        "SSGWIN32.EXE",
        "SSGWINCD/GIZMO.DAT",
    };

    std::vector<std::string> optionalFiles = {
        "SSGWINCD/GIZMO256.DAT",
        "SSGWINCD/PUZZLE.DAT",
        "SSGWINCD/FONT.DAT",
        "MOVIES/INTRO.SMK",
    };

    bool valid = true;

    std::cout << "Required Files:\n";
    for (const auto& file : requiredFiles) {
        std::string fullPath = gamePath + "/" + file;
        bool exists = fs::exists(fullPath);
        std::cout << (exists ? "  [OK] " : "  [MISSING] ") << file << "\n";
        if (!exists) valid = false;
    }

    std::cout << "\nOptional Files:\n";
    for (const auto& file : optionalFiles) {
        std::string fullPath = gamePath + "/" + file;
        bool exists = fs::exists(fullPath);
        std::cout << (exists ? "  [OK] " : "  [--] ") << file << "\n";
    }

    // Try to open a DAT file
    std::cout << "\nFile Format Validation:\n";

    std::string gizmoDat = gamePath + "/SSGWINCD/GIZMO.DAT";
    if (fs::exists(gizmoDat)) {
        NEResourceExtractor ne;
        if (ne.open(gizmoDat)) {
            auto resources = ne.listResources();
            std::cout << "  [OK] GIZMO.DAT is valid NE format (" << resources.size() << " resources)\n";
        } else {
            std::cout << "  [FAIL] GIZMO.DAT: " << ne.getLastError() << "\n";
            valid = false;
        }
    }

    std::cout << "\n" << (valid ? "Validation PASSED" : "Validation FAILED") << "\n";
    return valid;
}

// Comprehensive sprite dimension analysis
void analyzeSpriteMetadata(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();

    NEResourceExtractor ne;
    if (!ne.open(path)) {
        std::cerr << "Failed to open NE: " << ne.getLastError() << "\n";
        return;
    }

    std::cout << "=== Sprite Metadata Analysis ===\n\n";

    auto resources = ne.listResources();

    // Find CUSTOM_32513 (0xFF01) index resources and CUSTOM_32514 (0xFF02) metadata resources
    std::map<uint16_t, Resource> indexResources;
    std::map<uint16_t, Resource> metaResources;

    for (const auto& res : resources) {
        if (res.typeId == 0xFF01) {
            indexResources[res.id] = res;
        } else if (res.typeId == 0xFF02) {
            metaResources[res.id] = res;
        }
    }

    std::cout << "Found " << indexResources.size() << " CUSTOM_32513 (index) resources\n";
    std::cout << "Found " << metaResources.size() << " CUSTOM_32514 (metadata) resources\n\n";

    // Examine metadata resources for dimension info
    std::cout << "=== Examining CUSTOM_32514 Metadata Resources ===\n";

    int examined = 0;
    for (const auto& [id, res] : metaResources) {
        if (examined >= 10) break;

        auto data = ne.extractResource(res);
        if (data.size() < 4) continue;

        printf("\nMetadata #%d (%zu bytes):\n", id, data.size());

        // Hex dump first 64 bytes
        size_t dumpLen = std::min(size_t(64), data.size());
        for (size_t i = 0; i < dumpLen; i += 16) {
            printf("  %04zX: ", i);
            for (size_t j = 0; j < 16 && i + j < dumpLen; ++j) {
                printf("%02X ", data[i + j]);
            }
            printf("\n");
        }

        // Try to parse as dimension data
        // Common patterns: (width, height) as 16-bit values
        if (data.size() >= 4) {
            uint16_t w1 = data[0] | (data[1] << 8);
            uint16_t h1 = data[2] | (data[3] << 8);
            printf("  As dimensions (bytes 0-3): %d x %d\n", w1, h1);
        }

        examined++;
    }

    // Now look at corresponding index resources and trace to actual data
    std::cout << "\n=== Correlating Index with Sprite Data ===\n";

    const uint32_t SPRITE_BASE = 0x10000;

    examined = 0;
    for (const auto& [id, idxRes] : indexResources) {
        if (examined >= 5) break;
        if (idxRes.size < 30) continue;

        auto idxData = ne.extractResource(idxRes);

        // Parse index header
        uint16_t spriteCount = idxData[0] | (idxData[1] << 8);
        uint16_t frameCount = idxData[2] | (idxData[3] << 8);

        if (spriteCount == 0 || spriteCount > 100) continue;

        printf("\nIndex #%d: %d sprites, %d frames\n", id, spriteCount, frameCount);

        // Parse offsets and examine sprite data at each offset
        std::vector<uint32_t> offsets;
        for (size_t i = 18; i + 3 < idxData.size() && offsets.size() < (size_t)spriteCount + 1; i += 4) {
            uint32_t off = idxData[i] | (idxData[i+1] << 8) |
                          (idxData[i+2] << 16) | (idxData[i+3] << 24);
            if (off < 0x400000) {
                offsets.push_back(off);
            }
        }

        // Examine first 3 sprites in this index
        for (size_t i = 0; i < std::min(size_t(3), offsets.size()); ++i) {
            uint32_t spriteAddr = SPRITE_BASE + offsets[i];
            if (spriteAddr >= fileSize) continue;

            file.seekg(spriteAddr);
            uint8_t header[32];
            file.read(reinterpret_cast<char*>(header), 32);

            printf("  Sprite %zu at 0x%X: ", i, spriteAddr);
            for (int j = 0; j < 16; ++j) printf("%02X ", header[j]);
            printf("\n");

            // Try different dimension interpretations
            uint16_t v0 = header[0] | (header[1] << 8);
            uint16_t v1 = header[2] | (header[3] << 8);
            uint8_t b0 = header[0];
            uint8_t b1 = header[1];
            uint8_t b2 = header[2];
            uint8_t b3 = header[3];

            printf("    As uint16 LE: %d, %d | As bytes: %d, %d, %d, %d\n",
                   v0, v1, b0, b1, b2, b3);

            // Check if bytes 0-1 look like small reasonable dimensions
            if (b0 > 0 && b0 <= 128 && b1 > 0 && b1 <= 128) {
                printf("    * Possible dimensions (byte 0, byte 1): %d x %d\n", b0, b1);
            }
        }

        examined++;
    }

    // Scan sprite data area for patterns
    std::cout << "\n=== Scanning Sprite Data Patterns ===\n";

    file.seekg(SPRITE_BASE);
    std::vector<uint8_t> spriteArea(0x10000);  // First 64K after base
    file.read(reinterpret_cast<char*>(spriteArea.data()), spriteArea.size());

    // Look for RLE patterns (0xFF byte byte) and try to find sprite boundaries
    std::cout << "Looking for sprite boundaries based on RLE patterns...\n";

    int rleSpriteCount = 0;
    size_t lastSpriteStart = 0;

    for (size_t i = 4; i < spriteArea.size() - 10 && rleSpriteCount < 20; ) {
        // Look for potential sprite header followed by RLE data
        uint8_t b0 = spriteArea[i];
        uint8_t b1 = spriteArea[i + 1];
        uint8_t b2 = spriteArea[i + 2];
        uint8_t b3 = spriteArea[i + 3];

        // Check if this could be start of a sprite
        // Pattern: small bytes (possible dimensions) followed by pixel data or RLE
        bool looksLikeHeader = (b0 > 0 && b0 <= 200 && b1 > 0 && b1 <= 200);
        bool hasRleNearby = false;

        // Check if RLE pattern appears soon after
        for (size_t j = 4; j < 20 && i + j + 2 < spriteArea.size(); ++j) {
            if (spriteArea[i + j] == 0xFF) {
                hasRleNearby = true;
                break;
            }
        }

        if (looksLikeHeader && hasRleNearby) {
            if (i - lastSpriteStart > 50) {  // Reasonable sprite size
                printf("  Potential sprite at 0x%zX: %dx%d? Data: ",
                       SPRITE_BASE + i, b0, b1);
                for (int k = 0; k < 12; ++k) printf("%02X ", spriteArea[i + k]);
                printf("\n");
                lastSpriteStart = i;
                rleSpriteCount++;
            }
        }
        i++;
    }
}

// Better sprite extraction using metadata correlation
void extractSpritesV2(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();

    // Load palette
    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    fs::create_directories(outDir);

    NEResourceExtractor ne;
    if (!ne.open(datPath)) {
        std::cerr << "Failed to open NE: " << ne.getLastError() << "\n";
        return;
    }

    auto resources = ne.listResources();
    const uint32_t SPRITE_BASE = 0x10000;

    // Build map of index resources (minimum 20 bytes for header + at least 1 offset)
    std::map<uint16_t, Resource> indexResources;
    for (const auto& res : resources) {
        if (res.typeId == 0xFF01 && res.size >= 22) {
            indexResources[res.id] = res;
        }
    }

    std::cout << "Processing " << indexResources.size() << " index resources...\n\n";

    int totalExtracted = 0;
    int skippedNoSprites = 0, skippedBadOffsets = 0, skippedTooSmall = 0;

    for (const auto& [id, idxRes] : indexResources) {
        auto idxData = ne.extractResource(idxRes);
        if (idxData.size() < 22) { skippedTooSmall++; continue; }

        uint16_t spriteCount = idxData[0] | (idxData[1] << 8);
        if (spriteCount == 0 || spriteCount > 200) { skippedNoSprites++; continue; }

        // Parse offsets
        std::vector<uint32_t> offsets;
        for (size_t i = 18; i + 3 < idxData.size() && offsets.size() <= (size_t)spriteCount; i += 4) {
            uint32_t off = idxData[i] | (idxData[i+1] << 8) |
                          (idxData[i+2] << 16) | (idxData[i+3] << 24);
            if (off < 0x400000) {
                offsets.push_back(off);
            }
        }

        if (offsets.empty()) { skippedBadOffsets++; continue; }

        // For single-sprite indices, add a dummy end offset
        if (offsets.size() == 1) {
            // Look for size hint in header bytes 6-9 (after constant)
            uint32_t sizeHint = 0;
            if (idxData.size() >= 10) {
                sizeHint = idxData[6] | (idxData[7] << 8);
                // Validate - should be reasonable sprite data size
                if (sizeHint < 50 || sizeHint > 20000) {
                    sizeHint = 2000;  // Default assumption
                }
            } else {
                sizeHint = 2000;
            }
            offsets.push_back(offsets[0] + sizeHint);
        }

        // Extract each sprite
        for (size_t s = 0; s + 1 < offsets.size() && totalExtracted < 1000; ++s) {
            uint32_t spriteAddr = SPRITE_BASE + offsets[s];
            uint32_t nextAddr = SPRITE_BASE + offsets[s + 1];

            if (spriteAddr >= fileSize) continue;
            if (nextAddr > fileSize) nextAddr = fileSize;
            if (nextAddr <= spriteAddr) continue;

            uint32_t spriteSize = nextAddr - spriteAddr;
            if (spriteSize < 10 || spriteSize > 50000) continue;

            // Read sprite data
            file.seekg(spriteAddr);
            std::vector<uint8_t> spriteData(spriteSize);
            file.read(reinterpret_cast<char*>(spriteData.data()), spriteSize);

            // Improved dimension detection:
            // Pattern 1: First two bytes are width and height (both should be reasonable < 200)
            // Pattern 2: No header, dimensions stored elsewhere

            int width = 0, height = 0;
            int dataStart = 0;

            uint8_t b0 = spriteData[0];
            uint8_t b1 = spriteData[1];

            // Check if first two bytes look like dimensions (not palette indices)
            // Palette indices for game graphics are typically 0x80-0xCF range
            // Dimensions would be smaller values like 8, 16, 32, etc.
            bool b0_looks_like_dim = (b0 >= 4 && b0 <= 128);
            bool b1_looks_like_dim = (b1 >= 4 && b1 <= 128);
            bool b0_looks_like_palette = (b0 >= 0x80 || b0 <= 2);
            bool b1_looks_like_palette = (b1 >= 0x80 || b1 <= 2);

            if (b0_looks_like_dim && b1_looks_like_dim && !b0_looks_like_palette && !b1_looks_like_palette) {
                // Both bytes look like dimensions
                width = b0;
                height = b1;
                dataStart = 2;

                // Validate: decompressed size should be reasonable compared to compressed
                size_t expectedPixels = (size_t)width * height;
                // RLE compression typically achieves 30-70% compression
                // So compressed size should be at least expectedPixels * 0.2
                if (expectedPixels > spriteSize * 10 || expectedPixels < spriteSize / 10) {
                    // Dimensions don't match data size, likely wrong
                    width = 0;
                    height = 0;
                }
            }

            // If dimension detection failed, try to estimate from data size
            if (width == 0 || height == 0) {
                dataStart = 0;
                // Estimate based on compressed data - assume ~40% compression ratio
                size_t estPixels = (size_t)(spriteSize * 2.5);

                // Try common sprite sizes that fit
                int commonSizes[] = {16, 24, 32, 48, 64, 80, 96, 128};
                int bestW = 32, bestH = 32;
                int bestDiff = INT_MAX;

                for (int w : commonSizes) {
                    for (int h : commonSizes) {
                        int pixels = w * h;
                        int diff = abs((int)estPixels - pixels);
                        if (diff < bestDiff && pixels <= estPixels * 2 && pixels >= estPixels / 4) {
                            bestDiff = diff;
                            bestW = w;
                            bestH = h;
                        }
                    }
                }
                width = bestW;
                height = bestH;
            }

            // RLE decompress:
            // Format: FF XX YY = repeat byte XX, YY times
            // Everything else is literal
            std::vector<uint8_t> pixels;
            size_t expectedPixels = (size_t)width * height;
            pixels.reserve(expectedPixels);

            for (size_t j = dataStart; pixels.size() < expectedPixels && j < spriteData.size(); ) {
                if (spriteData[j] == 0xFF && j + 2 < spriteData.size()) {
                    uint8_t byte = spriteData[j + 1];
                    uint8_t count = spriteData[j + 2];
                    if (count == 0) count = 1;
                    for (int k = 0; k < count && pixels.size() < expectedPixels; ++k) {
                        pixels.push_back(byte);
                    }
                    j += 3;
                } else {
                    pixels.push_back(spriteData[j]);
                    j++;
                }
            }

            // Pad if necessary
            while (pixels.size() < expectedPixels) {
                pixels.push_back(0);
            }

            // Save as BMP
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/idx%05d_spr%03zu_%dx%d.bmp",
                     outDir.c_str(), id, s, width, height);

            std::ofstream out(filename, std::ios::binary);
            if (!out) continue;

            int rowSize = (width + 3) & ~3;
            int imageSize = rowSize * height;
            int bmpSize = 54 + 1024 + imageSize;

            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = width;
            *(int32_t*)&bmpHeader[22] = height;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;
            *(uint32_t*)&bmpHeader[46] = 256;

            out.write(reinterpret_cast<char*>(bmpHeader), 54);
            out.write(reinterpret_cast<char*>(palette), 1024);

            std::vector<uint8_t> row(rowSize, 0);
            for (int y = height - 1; y >= 0; --y) {
                for (int x = 0; x < width; ++x) {
                    size_t idx = (size_t)y * width + x;
                    row[x] = (idx < pixels.size()) ? pixels[idx] : 0;
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }

            totalExtracted++;
        }
    }

    std::cout << "\nExtracted " << totalExtracted << " sprites to " << outDir << "\n";
    std::cout << "Skipped: " << skippedTooSmall << " too small, "
              << skippedNoSprites << " invalid sprite count, "
              << skippedBadOffsets << " insufficient offsets\n";
}

// Extract sprites as raw data (no RLE decompression) to test if RLE is the problem
void extractSpritesRaw(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();

    // Load palette
    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    fs::create_directories(outDir);

    NEResourceExtractor ne;
    if (!ne.open(datPath)) {
        std::cerr << "Failed to open NE\n";
        return;
    }

    auto resources = ne.listResources();
    const uint32_t SPRITE_BASE = 0x10000;

    std::map<uint16_t, Resource> indexResources;
    for (const auto& res : resources) {
        if (res.typeId == 0xFF01 && res.size >= 22) {
            indexResources[res.id] = res;
        }
    }

    std::cout << "Extracting RAW sprites (no RLE)...\n\n";
    int totalExtracted = 0;

    for (const auto& [id, idxRes] : indexResources) {
        auto idxData = ne.extractResource(idxRes);
        if (idxData.size() < 22) continue;

        uint16_t spriteCount = idxData[0] | (idxData[1] << 8);
        if (spriteCount == 0 || spriteCount > 200) continue;

        std::vector<uint32_t> offsets;
        for (size_t i = 18; i + 3 < idxData.size() && offsets.size() <= (size_t)spriteCount; i += 4) {
            uint32_t off = idxData[i] | (idxData[i+1] << 8) | (idxData[i+2] << 16) | (idxData[i+3] << 24);
            if (off < 0x400000) offsets.push_back(off);
        }

        if (offsets.empty()) continue;
        if (offsets.size() == 1) {
            offsets.push_back(offsets[0] + 2000);
        }

        for (size_t s = 0; s + 1 < offsets.size() && totalExtracted < 100; ++s) {
            uint32_t spriteAddr = SPRITE_BASE + offsets[s];
            uint32_t nextAddr = SPRITE_BASE + offsets[s + 1];

            if (spriteAddr >= fileSize) continue;
            if (nextAddr > fileSize) nextAddr = fileSize;
            if (nextAddr <= spriteAddr) continue;

            uint32_t spriteSize = nextAddr - spriteAddr;
            if (spriteSize < 10 || spriteSize > 50000) continue;

            file.seekg(spriteAddr);
            std::vector<uint8_t> spriteData(spriteSize);
            file.read(reinterpret_cast<char*>(spriteData.data()), spriteSize);

            // Check first two bytes for dimensions
            int width = spriteData[0];
            int height = spriteData[1];
            int dataStart = 2;

            // If dimensions look wrong, try without header
            if (width < 4 || width > 200 || height < 4 || height > 200) {
                // Try to find reasonable dimensions from data size
                // Assume each pixel is 1 byte (no compression)
                int dim = (int)sqrt((double)spriteSize);
                width = height = std::max(16, std::min(dim, 128));
                dataStart = 0;
            }

            // Read raw pixels (NO RLE)
            std::vector<uint8_t> pixels;
            size_t expectedPixels = (size_t)width * height;

            for (size_t j = dataStart; j < spriteData.size() && pixels.size() < expectedPixels; ++j) {
                pixels.push_back(spriteData[j]);
            }

            while (pixels.size() < expectedPixels) {
                pixels.push_back(0);
            }

            // Save as BMP
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/raw_%05d_%03zu_%dx%d.bmp",
                     outDir.c_str(), id, s, width, height);

            std::ofstream out(filename, std::ios::binary);
            if (!out) continue;

            int rowSize = (width + 3) & ~3;
            int imageSize = rowSize * height;
            int bmpSize = 54 + 1024 + imageSize;

            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = width;
            *(int32_t*)&bmpHeader[22] = height;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;
            *(uint32_t*)&bmpHeader[46] = 256;

            out.write(reinterpret_cast<char*>(bmpHeader), 54);
            out.write(reinterpret_cast<char*>(palette), 1024);

            std::vector<uint8_t> row(rowSize, 0);
            for (int y = height - 1; y >= 0; --y) {
                for (int x = 0; x < width; ++x) {
                    size_t idx = (size_t)y * width + x;
                    row[x] = (idx < pixels.size()) ? pixels[idx] : 0;
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }

            totalExtracted++;
        }
    }

    std::cout << "Extracted " << totalExtracted << " raw sprites to " << outDir << "\n";
}

// Test extraction with multiple fixed dimensions to find correct format
void testDimensions(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    fs::create_directories(outDir);

    // Test offsets - use known sprite data locations
    std::vector<uint32_t> testOffsets = {0x10000, 0x1053B, 0x10062, 0x102D9};

    // Try different fixed dimensions
    int testDims[][2] = {{16,16}, {24,24}, {32,32}, {48,48}, {64,64}, {11,17}, {17,11}, {32,48}, {48,32}};

    for (uint32_t offset : testOffsets) {
        file.seekg(offset);
        std::vector<uint8_t> data(4096);
        file.read(reinterpret_cast<char*>(data.data()), data.size());

        printf("\n=== Testing offset 0x%05X ===\n", offset);
        printf("First 16 bytes: ");
        for (int i = 0; i < 16; ++i) printf("%02X ", data[i]);
        printf("\n");

        for (auto& dim : testDims) {
            int width = dim[0], height = dim[1];

            // Decompress with RLE
            std::vector<uint8_t> pixels;
            size_t expectedPixels = width * height;
            pixels.reserve(expectedPixels);

            // Try both with and without 2-byte header skip
            for (int headerSkip = 0; headerSkip <= 2; headerSkip += 2) {
                pixels.clear();

                for (size_t j = headerSkip; pixels.size() < expectedPixels && j < data.size(); ) {
                    if (data[j] == 0xFF && j + 2 < data.size()) {
                        uint8_t byte = data[j + 1];
                        uint8_t count = data[j + 2];
                        if (count == 0) count = 1;
                        for (int k = 0; k < count && pixels.size() < expectedPixels; ++k) {
                            pixels.push_back(byte);
                        }
                        j += 3;
                    } else {
                        pixels.push_back(data[j]);
                        j++;
                    }
                }

                while (pixels.size() < expectedPixels) pixels.push_back(0);

                // Save BMP
                char filename[256];
                snprintf(filename, sizeof(filename), "%s/test_%05X_skip%d_%dx%d.bmp",
                         outDir.c_str(), offset, headerSkip, width, height);

                std::ofstream out(filename, std::ios::binary);
                if (!out) continue;

                int rowSize = (width + 3) & ~3;
                int imageSize = rowSize * height;
                int bmpSize = 54 + 1024 + imageSize;

                uint8_t bmpHeader[54] = {'B', 'M'};
                *(uint32_t*)&bmpHeader[2] = bmpSize;
                *(uint32_t*)&bmpHeader[10] = 54 + 1024;
                *(uint32_t*)&bmpHeader[14] = 40;
                *(int32_t*)&bmpHeader[18] = width;
                *(int32_t*)&bmpHeader[22] = height;
                *(uint16_t*)&bmpHeader[26] = 1;
                *(uint16_t*)&bmpHeader[28] = 8;
                *(uint32_t*)&bmpHeader[34] = imageSize;
                *(uint32_t*)&bmpHeader[46] = 256;

                out.write(reinterpret_cast<char*>(bmpHeader), 54);
                out.write(reinterpret_cast<char*>(palette), 1024);

                std::vector<uint8_t> row(rowSize, 0);
                for (int y = height - 1; y >= 0; --y) {
                    for (int x = 0; x < width; ++x) {
                        row[x] = pixels[y * width + x];
                    }
                    out.write(reinterpret_cast<char*>(row.data()), rowSize);
                }
            }
        }
    }

    std::cout << "Created test images in " << outDir << "\n";
    std::cout << "Compare visually to find correct dimensions!\n";
}

// Width finder - creates many images with different widths to find correct one
void findWidth(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    fs::create_directories(outDir);

    // Read a chunk of sprite data
    uint32_t offset = 0x1053B;  // Known sprite with 0B 11 header
    file.seekg(offset);
    std::vector<uint8_t> data(8192);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    std::cout << "Testing sprite at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "First 20 bytes: ";
    for (int i = 0; i < 20; ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    // Decompress ALL the data first (without knowing dimensions)
    std::vector<uint8_t> decompressed;
    decompressed.reserve(16384);

    int dataStart = 2;  // Skip 0B 11 header
    for (size_t j = dataStart; j < data.size() && decompressed.size() < 16384; ) {
        if (data[j] == 0xFF && j + 2 < data.size()) {
            uint8_t byte = data[j + 1];
            uint8_t count = data[j + 2];
            if (count == 0) count = 1;
            for (int k = 0; k < count; ++k) {
                decompressed.push_back(byte);
            }
            j += 3;
        } else {
            decompressed.push_back(data[j]);
            j++;
        }
    }

    std::cout << "Decompressed " << decompressed.size() << " pixels\n\n";

    // Now render with different widths
    for (int width = 8; width <= 80; width++) {
        int height = std::min(80, (int)(decompressed.size() / width));
        if (height < 8) continue;

        size_t expectedPixels = width * height;

        char filename[256];
        snprintf(filename, sizeof(filename), "%s/width_%02d.bmp", outDir.c_str(), width);

        std::ofstream out(filename, std::ios::binary);
        if (!out) continue;

        int rowSize = (width + 3) & ~3;
        int imageSize = rowSize * height;
        int bmpSize = 54 + 1024 + imageSize;

        uint8_t bmpHeader[54] = {'B', 'M'};
        *(uint32_t*)&bmpHeader[2] = bmpSize;
        *(uint32_t*)&bmpHeader[10] = 54 + 1024;
        *(uint32_t*)&bmpHeader[14] = 40;
        *(int32_t*)&bmpHeader[18] = width;
        *(int32_t*)&bmpHeader[22] = height;
        *(uint16_t*)&bmpHeader[26] = 1;
        *(uint16_t*)&bmpHeader[28] = 8;
        *(uint32_t*)&bmpHeader[34] = imageSize;
        *(uint32_t*)&bmpHeader[46] = 256;

        out.write(reinterpret_cast<char*>(bmpHeader), 54);
        out.write(reinterpret_cast<char*>(palette), 1024);

        std::vector<uint8_t> row(rowSize, 0);
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                size_t idx = y * width + x;
                row[x] = (idx < decompressed.size()) ? decompressed[idx] : 0;
            }
            out.write(reinterpret_cast<char*>(row.data()), rowSize);
        }
    }

    std::cout << "Created width test images (width_08.bmp to width_80.bmp)\n";
    std::cout << "Look through them to find which width shows a clear sprite!\n";
}

// Estimate dimensions by decompressing RLE and counting actual pixels per row
std::pair<int,int> estimateDimensions(const std::vector<uint8_t>& data, uint32_t startOffset, uint32_t spriteSize) {
    // Decompress RLE to count pixels per row and find max width needed
    int maxRowPixels = 0;
    int currentRowPixels = 0;
    int rowCount = 0;
    size_t endOffset = std::min((size_t)(startOffset + spriteSize), data.size());

    for (size_t pos = startOffset; pos < endOffset; ) {
        uint8_t byte = data[pos++];

        if (byte == 0xFF && pos + 1 < endOffset) {
            // RLE: skip value, read count
            pos++;  // value
            uint8_t count = data[pos++] + 1;
            currentRowPixels += count;
        } else if (byte == 0x00) {
            // Row terminator
            if (currentRowPixels > maxRowPixels) maxRowPixels = currentRowPixels;
            currentRowPixels = 0;
            rowCount++;
        } else {
            // Literal pixel
            currentRowPixels++;
        }
    }
    // Handle last row if no terminator
    if (currentRowPixels > maxRowPixels) maxRowPixels = currentRowPixels;
    if (currentRowPixels > 0) rowCount++;

    if (rowCount == 0) return {32, 32};
    if (maxRowPixels == 0) maxRowPixels = 32;

    // Round max width UP to common sprite widths
    static const int commonWidths[] = {16, 24, 32, 40, 48, 55, 64, 80, 94, 96, 128, 160, 192, 256};
    int closestW = 256;  // Default to max if nothing fits
    for (int w : commonWidths) {
        if (w >= maxRowPixels) {
            closestW = w;
            break;
        }
    }

    return {closestW, rowCount};
}

void extractIndexedSprites(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    uint8_t defaultPalette[256][4] = {};
    loadPalette(palettePath, defaultPalette);

    // Set palette index 0 to magenta (transparency indicator)
    defaultPalette[0][0] = 255;  // Blue
    defaultPalette[0][1] = 0;    // Green
    defaultPalette[0][2] = 255;  // Red
    defaultPalette[0][3] = 0;    // Reserved

    NEResourceExtractor ne;
    if (!ne.open(datPath)) {
        std::cerr << "Failed to open NE: " << ne.getLastError() << "\n";
        return;
    }

    fs::create_directories(outDir);

    auto resources = ne.listResources();
    int totalExtracted = 0;

    // Build map of per-resource palettes (CUSTOM_32514, 1536 bytes, doubled-byte format)
    std::map<int, std::vector<uint8_t>> perResourcePalettes;
    for (const auto& res : resources) {
        if (res.typeId == 0xFF02 && res.size == 1536) {  // CUSTOM_32514 palette
            auto palData = ne.extractResource(res);
            if (palData.size() == 1536) {
                std::vector<uint8_t> pal(1024, 0);
                // Convert doubled-byte format: 00 R 00 G 00 B -> BGRA
                for (int i = 0; i < 256 && i * 6 + 5 < (int)palData.size(); ++i) {
                    pal[i * 4 + 2] = palData[i * 6 + 1];  // R
                    pal[i * 4 + 1] = palData[i * 6 + 3];  // G
                    pal[i * 4 + 0] = palData[i * 6 + 5];  // B
                    pal[i * 4 + 3] = 0;
                }
                // Set index 0 to magenta for transparency
                pal[0] = 255; pal[1] = 0; pal[2] = 255; pal[3] = 0;
                perResourcePalettes[res.id] = pal;
            }
        }
    }
    if (!perResourcePalettes.empty()) {
        std::cout << "Found " << perResourcePalettes.size() << " per-resource palettes\n";
    }

    // Known dimensions for specific resources (from successful manual extractions)
    std::map<int, std::pair<int,int>> knownDims = {
        {35283, {94, 109}},
        {35368, {64, 58}},
        {35384, {55, 47}},
        {35299, {31, 9}},
        {35557, {43, 43}},
        {34368, {64, 64}},  // Gizmos
        {34441, {11, 17}},  // Gizmos small sprites
    };

    for (const auto& res : resources) {
        if (res.typeId != 0xFF01 || res.size < 18) continue;  // CUSTOM_32513

        // Use per-resource palette if available, otherwise default
        uint8_t palette[256][4];
        if (perResourcePalettes.count(res.id)) {
            auto& pal = perResourcePalettes[res.id];
            memcpy(palette, pal.data(), 1024);
        } else {
            memcpy(palette, defaultPalette, 1024);
        }

        auto data = ne.extractResource(res);
        if (data.size() < 18) continue;

        // Parse header:
        // bytes 0-1: version (usually 1)
        // bytes 2-3: sprite/frame count
        // bytes 4-13: reserved
        // bytes 14+: offset table (4 bytes per sprite)
        uint16_t version = data[0] | (data[1] << 8);
        uint16_t spriteCount = data[2] | (data[3] << 8);

        if (version != 1 || spriteCount == 0 || spriteCount > 500) continue;

        size_t headerSize = 14 + spriteCount * 4;
        if (data.size() < headerSize) continue;

        // Read offset table (starts at byte 14)
        std::vector<uint32_t> offsets;
        for (int i = 0; i < spriteCount; ++i) {
            size_t idx = 14 + i * 4;
            uint32_t off = data[idx] | (data[idx+1] << 8) |
                          (data[idx+2] << 16) | (data[idx+3] << 24);
            offsets.push_back(off);
        }

        // Determine dimensions - same for all sprites in this resource
        int width, height;
        if (knownDims.count(res.id)) {
            width = knownDims[res.id].first;
            height = knownDims[res.id].second;
        } else {
            // Estimate from first sprite
            uint32_t firstOffset = offsets[0];
            uint32_t spriteSize = (offsets.size() > 1) ? offsets[1] - firstOffset : data.size() - firstOffset;
            auto dims = estimateDimensions(data, firstOffset, spriteSize);
            width = dims.first;
            height = dims.second;
        }

        // Extract each sprite frame
        for (size_t frameIdx = 0; frameIdx < offsets.size(); ++frameIdx) {
            uint32_t offset = offsets[frameIdx];
            if (offset >= data.size()) continue;

            int totalPixels = width * height;
            std::vector<uint8_t> pixels(totalPixels, 0);

            // Decompress RLE (matching PowerShell reference):
            // FF VV CC = repeat value VV for CC+1 times
            // 00 = row end (move to next row)
            // Other = literal pixel
            int x = 0, y = 0;
            size_t pos = offset;

            while (pos < data.size() && y < height) {
                uint8_t byte = data[pos++];

                if (byte == 0xFF && pos + 1 < data.size()) {
                    // RLE: repeat value for count+1 times
                    uint8_t value = data[pos++];
                    uint8_t count = data[pos++] + 1;
                    for (int k = 0; k < count && x < width; ++k) {
                        if (y < height && y * width + x < totalPixels) {
                            pixels[y * width + x] = value;
                        }
                        x++;
                    }
                } else if (byte == 0x00) {
                    // Row end - move to next row
                    y++;
                    x = 0;
                } else {
                    // Literal pixel
                    if (x < width && y < height && y * width + x < totalPixels) {
                        pixels[y * width + x] = byte;
                    }
                    x++;
                }
            }

            // Write BMP (no vertical flip - BMP already bottom-up)
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/idx%05d_spr%03zu_%dx%d.bmp",
                     outDir.c_str(), res.id, frameIdx, width, height);

            std::ofstream out(filename, std::ios::binary);
            if (!out) continue;

            int rowSize = (width + 3) & ~3;
            int imageSize = rowSize * height;
            int bmpSize = 54 + 1024 + imageSize;

            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = width;
            *(int32_t*)&bmpHeader[22] = height;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;
            *(uint32_t*)&bmpHeader[46] = 256;

            out.write(reinterpret_cast<char*>(bmpHeader), 54);
            out.write(reinterpret_cast<char*>(palette), 1024);

            // Write pixel rows (BMP is bottom-up, so flip Y)
            std::vector<uint8_t> row(rowSize, 0);
            for (int by = height - 1; by >= 0; --by) {
                for (int bx = 0; bx < width; ++bx) {
                    size_t idx = by * width + bx;
                    row[bx] = (idx < pixels.size()) ? pixels[idx] : 0;
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }

            totalExtracted++;
        }
    }

    std::cout << "Extracted " << totalExtracted << " sprite frames to " << outDir << "\n";
}

// Extract sprites in RUND format (Treasure Cove, Treasure Mountain)
// Format: width(2) + height(2) + "RUND" + RLE data
void extractRundSprites(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    // Set palette index 0 to magenta (transparency indicator)
    palette[0][0] = 255;  // Blue
    palette[0][1] = 0;    // Green
    palette[0][2] = 255;  // Red
    palette[0][3] = 0;    // Reserved

    NEResourceExtractor ne;
    if (!ne.open(datPath)) {
        std::cerr << "Failed to open NE: " << ne.getLastError() << "\n";
        return;
    }

    fs::create_directories(outDir);

    auto resources = ne.listResources();
    int totalExtracted = 0;

    for (const auto& res : resources) {
        if (res.typeId != 0xFF01 || res.size < 8) continue;  // CUSTOM_32513

        auto data = ne.extractResource(res);
        if (data.size() < 8) continue;

        // Check for RUND magic at bytes 4-7
        if (data[4] != 'R' || data[5] != 'U' || data[6] != 'N' || data[7] != 'D') continue;

        // Parse dimensions
        uint16_t width = data[0] | (data[1] << 8);
        uint16_t height = data[2] | (data[3] << 8);

        // Sanity check dimensions
        if (width == 0 || width > 1024 || height == 0 || height > 1024) continue;

        int totalPixels = width * height;
        std::vector<uint8_t> pixels(totalPixels, 0);

        // Decompress RLE starting at byte 8
        // Format discovered from analysis:
        //   Byte >= 0x80: RLE run, count = (byte & 0x7F), next byte is value to repeat
        //   Byte < 0x80: Literal run, count = byte, followed by 'count' literal bytes
        int pixelIdx = 0;
        size_t pos = 8;

        while (pos < data.size() && pixelIdx < totalPixels) {
            uint8_t byte = data[pos++];

            if (byte >= 0x80) {
                // RLE: repeat next byte (byte & 0x7F) times
                int count = byte & 0x7F;
                if (pos >= data.size()) break;
                uint8_t value = data[pos++];
                for (int k = 0; k < count && pixelIdx < totalPixels; ++k) {
                    pixels[pixelIdx++] = value;
                }
            } else {
                // Literal: read 'byte' literal bytes
                int count = byte;
                for (int k = 0; k < count && pos < data.size() && pixelIdx < totalPixels; ++k) {
                    pixels[pixelIdx++] = data[pos++];
                }
            }
        }

        // Write BMP
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/rund_%05d_%dx%d.bmp",
                 outDir.c_str(), res.id, width, height);

        std::ofstream out(filename, std::ios::binary);
        if (!out) continue;

        int rowSize = (width + 3) & ~3;
        int imageSize = rowSize * height;
        int bmpSize = 54 + 1024 + imageSize;

        uint8_t bmpHeader[54] = {'B', 'M'};
        *(uint32_t*)&bmpHeader[2] = bmpSize;
        *(uint32_t*)&bmpHeader[10] = 54 + 1024;
        *(uint32_t*)&bmpHeader[14] = 40;
        *(int32_t*)&bmpHeader[18] = width;
        *(int32_t*)&bmpHeader[22] = height;
        *(uint16_t*)&bmpHeader[26] = 1;
        *(uint16_t*)&bmpHeader[28] = 8;
        *(uint32_t*)&bmpHeader[34] = imageSize;

        out.write(reinterpret_cast<char*>(bmpHeader), 54);

        // Write palette (BGRA format)
        for (int i = 0; i < 256; ++i) {
            out.write(reinterpret_cast<char*>(palette[i]), 4);
        }

        // Write pixel rows (BMP is bottom-up, so flip Y)
        std::vector<uint8_t> row(rowSize, 0);
        for (int by = height - 1; by >= 0; --by) {
            for (int bx = 0; bx < width; ++bx) {
                size_t idx = by * width + bx;
                row[bx] = (idx < pixels.size()) ? pixels[idx] : 0;
            }
            out.write(reinterpret_cast<char*>(row.data()), rowSize);
        }

        totalExtracted++;
    }

    std::cout << "Extracted " << totalExtracted << " RUND sprites to " << outDir << "\n";
}

// Dump raw RUND resource bytes for analysis
void dumpRundBytes(const std::string& datPath, int count) {
    NEResourceExtractor ne;
    if (!ne.open(datPath)) {
        std::cerr << "Failed to open NE: " << ne.getLastError() << "\n";
        return;
    }

    auto resources = ne.listResources();
    int dumped = 0;

    for (const auto& res : resources) {
        if (dumped >= count) break;
        if (res.typeId != 0xFF01 || res.size < 8) continue;  // CUSTOM_32513

        auto data = ne.extractResource(res);
        if (data.size() < 8) continue;

        // Check for RUND magic at bytes 4-7
        if (data[4] != 'R' || data[5] != 'U' || data[6] != 'N' || data[7] != 'D') continue;

        // Parse dimensions
        uint16_t width = data[0] | (data[1] << 8);
        uint16_t height = data[2] | (data[3] << 8);

        std::cout << "\n=== Resource ID " << res.id << " (" << width << "x" << height
                  << "), size=" << data.size() << " bytes ===\n";

        // Dump first 128 bytes as hex
        size_t dumpLen = std::min(data.size(), (size_t)128);
        for (size_t i = 0; i < dumpLen; i++) {
            if (i % 16 == 0) {
                if (i > 0) std::cout << "\n";
                printf("%04zx: ", i);
            }
            printf("%02X ", data[i]);
        }
        std::cout << "\n";

        // Show expected total pixels vs compressed data size
        int totalPixels = width * height;
        int compressedSize = data.size() - 8;
        float ratio = (float)compressedSize / totalPixels;
        std::cout << "Total pixels: " << totalPixels << ", Compressed bytes: " << compressedSize
                  << ", Ratio: " << ratio << "\n";

        dumped++;
    }
}

// Extract Operation Neptune labyrinth tilemaps (640x480 RLE-compressed)
// Palette is in separate 1536-byte resources (doubled-byte format: 00 R 00 G 00 B)
void extractLabyrinthTilemaps(const std::string& datPath, const std::string& outDir) {
    NEResourceExtractor ne;
    if (!ne.open(datPath)) {
        std::cerr << "Failed to open NE: " << ne.getLastError() << "\n";
        return;
    }

    fs::create_directories(outDir);

    auto resources = ne.listResources();

    // First pass: collect palettes (1536-byte resources)
    std::map<int, std::vector<uint8_t>> palettes;
    for (const auto& res : resources) {
        if (res.size == 1536) {
            auto data = ne.extractResource(res);
            // Convert doubled-byte palette to standard RGB
            std::vector<uint8_t> pal(256 * 4, 0);
            for (int i = 0; i < 256 && i * 6 + 5 < (int)data.size(); ++i) {
                // Format: 00 R 00 G 00 B (every other byte)
                pal[i * 4 + 2] = data[i * 6 + 1];  // Red
                pal[i * 4 + 1] = data[i * 6 + 3];  // Green
                pal[i * 4 + 0] = data[i * 6 + 5];  // Blue
                pal[i * 4 + 3] = 0;                 // Reserved
            }
            palettes[res.id] = pal;
            std::cout << "Found palette for ID " << res.id << "\n";
        }
    }

    int totalExtracted = 0;

    // Second pass: extract tilemaps (large resources with tilemap header)
    for (const auto& res : resources) {
        if (res.size < 0x2A) continue;  // Need at least header

        auto data = ne.extractResource(res);
        if (data.size() < 0x2A) continue;

        // Check for tilemap header signature
        uint16_t version = data[0] | (data[1] << 8);
        uint16_t type = data[2] | (data[3] << 8);

        if (version != 1 || type != 1) continue;

        // Check for separator at 0x20
        if (data[0x20] != 0xFF || data[0x21] != 0xFF) continue;

        // Read dimensions
        uint16_t width = data[0x26] | (data[0x27] << 8);
        uint16_t height = data[0x28] | (data[0x29] << 8);

        if (width != 640 || height != 480) continue;  // Only 640x480 tilemaps

        std::cout << "Decoding tilemap " << res.id << " (" << width << "x" << height << ")...\n";

        // Find matching palette
        std::vector<uint8_t>* palette = nullptr;
        if (palettes.count(res.id)) {
            palette = &palettes[res.id];
        } else {
            // Use first available palette
            if (!palettes.empty()) {
                palette = &palettes.begin()->second;
            }
        }

        int totalPixels = width * height;
        std::vector<uint8_t> pixels(totalPixels, 0);

        // Decompress RLE starting at byte 0x2A
        // Format: FF VV CC = repeat value VV for CC+1 times
        //         Other = literal pixel
        int pixelIdx = 0;
        size_t pos = 0x2A;

        while (pos < data.size() && pixelIdx < totalPixels) {
            uint8_t byte = data[pos++];

            if (byte == 0xFF && pos + 1 < data.size()) {
                uint8_t value = data[pos++];
                uint8_t count = data[pos++];
                int repeat = count + 1;
                for (int k = 0; k < repeat && pixelIdx < totalPixels; ++k) {
                    pixels[pixelIdx++] = value;
                }
            } else {
                pixels[pixelIdx++] = byte;
            }
        }

        // Write BMP
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/tilemap_%05d_%dx%d.bmp",
                 outDir.c_str(), res.id, width, height);

        std::ofstream out(filename, std::ios::binary);
        if (!out) continue;

        int rowSize = (width + 3) & ~3;
        int imageSize = rowSize * height;
        int bmpSize = 54 + 1024 + imageSize;

        uint8_t bmpHeader[54] = {'B', 'M'};
        *(uint32_t*)&bmpHeader[2] = bmpSize;
        *(uint32_t*)&bmpHeader[10] = 54 + 1024;
        *(uint32_t*)&bmpHeader[14] = 40;
        *(int32_t*)&bmpHeader[18] = width;
        *(int32_t*)&bmpHeader[22] = height;
        *(uint16_t*)&bmpHeader[26] = 1;
        *(uint16_t*)&bmpHeader[28] = 8;
        *(uint32_t*)&bmpHeader[34] = imageSize;

        out.write(reinterpret_cast<char*>(bmpHeader), 54);

        // Write palette
        if (palette) {
            out.write(reinterpret_cast<char*>(palette->data()), 1024);
        } else {
            // Default grayscale palette
            for (int i = 0; i < 256; ++i) {
                uint8_t p[4] = {(uint8_t)i, (uint8_t)i, (uint8_t)i, 0};
                out.write(reinterpret_cast<char*>(p), 4);
            }
        }

        // Write pixel rows (BMP is bottom-up)
        std::vector<uint8_t> row(rowSize, 0);
        for (int by = height - 1; by >= 0; --by) {
            for (int bx = 0; bx < width; ++bx) {
                size_t idx = by * width + bx;
                row[bx] = (idx < pixels.size()) ? pixels[idx] : 0;
            }
            out.write(reinterpret_cast<char*>(row.data()), rowSize);
        }

        totalExtracted++;
    }

    std::cout << "Extracted " << totalExtracted << " tilemaps to " << outDir << "\n";
}

// Extract Operation Neptune labyrinth sprites (uses different type IDs)
void extractLabyrinthSprites(const std::string& datPath, const std::string& palettePath, const std::string& outDir) {
    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    // Set palette index 0 to magenta (transparency)
    palette[0][0] = 255;
    palette[0][1] = 0;
    palette[0][2] = 255;
    palette[0][3] = 0;

    NEResourceExtractor ne;
    if (!ne.open(datPath)) {
        std::cerr << "Failed to open NE: " << ne.getLastError() << "\n";
        return;
    }

    fs::create_directories(outDir);

    auto resources = ne.listResources();
    int totalExtracted = 0;

    for (const auto& res : resources) {
        // Skip small (palette) and large (tilemap) resources
        if (res.size < 100 || res.size == 1536) continue;

        auto data = ne.extractResource(res);
        if (data.size() < 18) continue;

        // Check for sprite header (version 1, sprite count)
        uint16_t version = data[0] | (data[1] << 8);
        uint16_t spriteCount = data[2] | (data[3] << 8);

        // Labyrinth sprites have version=1 and reasonable sprite count
        if (version != 1 || spriteCount == 0 || spriteCount > 100) continue;

        // Check if this looks like a tilemap (has separator at 0x20)
        if (data.size() > 0x22 && data[0x20] == 0xFF && data[0x21] == 0xFF) continue;

        size_t headerSize = 14 + spriteCount * 4;
        if (data.size() < headerSize) continue;

        // Read offset table
        std::vector<uint32_t> offsets;
        for (int i = 0; i < spriteCount; ++i) {
            size_t idx = 14 + i * 4;
            uint32_t off = data[idx] | (data[idx+1] << 8) |
                          (data[idx+2] << 16) | (data[idx+3] << 24);
            offsets.push_back(off);
        }

        // Estimate dimensions from first sprite
        uint32_t firstOffset = offsets[0];
        uint32_t spriteSize = (offsets.size() > 1) ? offsets[1] - firstOffset : data.size() - firstOffset;

        // Count row terminators to estimate height
        int rowCount = 0;
        for (size_t p = firstOffset; p < firstOffset + spriteSize && p < data.size(); ++p) {
            if (data[p] == 0x00) rowCount++;
        }
        if (rowCount == 0) rowCount = 32;

        // Estimate width
        int estimatedPixels = spriteSize * 2;
        int estimatedWidth = std::max(16, std::min(256, estimatedPixels / rowCount));

        static const int commonWidths[] = {16, 24, 32, 40, 48, 64, 80, 96, 128};
        int width = 32;
        int minDiff = 999;
        for (int w : commonWidths) {
            int diff = std::abs(w - estimatedWidth);
            if (diff < minDiff) {
                minDiff = diff;
                width = w;
            }
        }
        int height = rowCount;

        std::cout << "Extracting " << spriteCount << " sprites from resource " << res.id
                  << " (" << width << "x" << height << ")...\n";

        // Extract each sprite
        for (size_t frameIdx = 0; frameIdx < offsets.size(); ++frameIdx) {
            uint32_t offset = offsets[frameIdx];
            if (offset >= data.size()) continue;

            int totalPixels = width * height;
            std::vector<uint8_t> pixels(totalPixels, 0);

            int x = 0, y = 0;
            size_t pos = offset;

            while (pos < data.size() && y < height) {
                uint8_t byte = data[pos++];

                if (byte == 0xFF && pos + 1 < data.size()) {
                    uint8_t value = data[pos++];
                    uint8_t count = data[pos++] + 1;
                    for (int k = 0; k < count && x < width; ++k) {
                        if (y < height) pixels[y * width + x] = value;
                        x++;
                    }
                } else if (byte == 0x00) {
                    y++;
                    x = 0;
                } else {
                    if (x < width && y < height) pixels[y * width + x] = byte;
                    x++;
                }
            }

            // Write BMP
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/lab_%05d_spr%03zu_%dx%d.bmp",
                     outDir.c_str(), res.id, frameIdx, width, height);

            std::ofstream out(filename, std::ios::binary);
            if (!out) continue;

            int rowSize = (width + 3) & ~3;
            int imageSize = rowSize * height;
            int bmpSize = 54 + 1024 + imageSize;

            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = width;
            *(int32_t*)&bmpHeader[22] = height;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;

            out.write(reinterpret_cast<char*>(bmpHeader), 54);

            for (int i = 0; i < 256; ++i) {
                out.write(reinterpret_cast<char*>(palette[i]), 4);
            }

            std::vector<uint8_t> row(rowSize, 0);
            for (int by = height - 1; by >= 0; --by) {
                for (int bx = 0; bx < width; ++bx) {
                    row[bx] = pixels[by * width + bx];
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }

            totalExtracted++;
        }
    }

    std::cout << "Extracted " << totalExtracted << " labyrinth sprites to " << outDir << "\n";
}

void testRLEFormats(const std::string& datPath, const std::string& palettePath,
                    uint32_t offset, int width, int height, const std::string& outDir) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    fs::create_directories(outDir);

    file.seekg(offset);
    std::vector<uint8_t> data(32768);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int totalPixels = width * height;

    std::cout << "Testing RLE formats at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << "\n";
    std::cout << "First 40 bytes:\n";
    for (int i = 0; i < 40; ++i) {
        printf("%02X ", data[i]);
        if ((i + 1) % 20 == 0) printf("\n");
    }
    std::cout << "\n";

    auto writeBMP = [&](const std::string& name, const std::vector<uint8_t>& pixels) {
        std::string path = outDir + "/" + name;
        std::ofstream out(path, std::ios::binary);
        if (!out) return;

        int rowSize = (width + 3) & ~3;
        int imageSize = rowSize * height;
        int bmpSize = 54 + 1024 + imageSize;

        uint8_t bmpHeader[54] = {'B', 'M'};
        *(uint32_t*)&bmpHeader[2] = bmpSize;
        *(uint32_t*)&bmpHeader[10] = 54 + 1024;
        *(uint32_t*)&bmpHeader[14] = 40;
        *(int32_t*)&bmpHeader[18] = width;
        *(int32_t*)&bmpHeader[22] = height;
        *(uint16_t*)&bmpHeader[26] = 1;
        *(uint16_t*)&bmpHeader[28] = 8;
        *(uint32_t*)&bmpHeader[34] = imageSize;
        *(uint32_t*)&bmpHeader[46] = 256;

        out.write(reinterpret_cast<char*>(bmpHeader), 54);
        out.write(reinterpret_cast<const char*>(palette), 1024);

        std::vector<uint8_t> row(rowSize, 0);
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                size_t idx = y * width + x;
                row[x] = (idx < pixels.size()) ? pixels[idx] : 0;
            }
            out.write(reinterpret_cast<char*>(row.data()), rowSize);
        }
        std::cout << "Wrote: " << path << "\n";
    };

    // Test 1: Raw data (no RLE), skip 2 bytes header
    {
        std::vector<uint8_t> pixels(data.begin() + 2, data.begin() + 2 + totalPixels);
        writeBMP("raw_skip2.bmp", pixels);
    }

    // Test 2: Raw data (no RLE), no header skip
    {
        std::vector<uint8_t> pixels(data.begin(), data.begin() + totalPixels);
        writeBMP("raw_skip0.bmp", pixels);
    }

    // Test 3: RLE FF <byte> <count> (current format), skip 2
    {
        std::vector<uint8_t> pixels;
        for (size_t pos = 2; pos < data.size() && (int)pixels.size() < totalPixels; ) {
            if (data[pos] == 0xFF && pos + 2 < data.size()) {
                uint8_t byte = data[pos + 1];
                uint8_t count = data[pos + 2];
                if (count == 0) count = 1;
                for (int k = 0; k < count && (int)pixels.size() < totalPixels; ++k)
                    pixels.push_back(byte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }
        writeBMP("rle_ff_byte_count_skip2.bmp", pixels);
    }

    // Test 4: RLE FF <count> <byte> (alternate format), skip 2
    {
        std::vector<uint8_t> pixels;
        for (size_t pos = 2; pos < data.size() && (int)pixels.size() < totalPixels; ) {
            if (data[pos] == 0xFF && pos + 2 < data.size()) {
                uint8_t count = data[pos + 1];
                uint8_t byte = data[pos + 2];
                if (count == 0) count = 1;
                for (int k = 0; k < count && (int)pixels.size() < totalPixels; ++k)
                    pixels.push_back(byte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }
        writeBMP("rle_ff_count_byte_skip2.bmp", pixels);
    }

    // Test 5: RLE with 0x00 as escape code
    {
        std::vector<uint8_t> pixels;
        for (size_t pos = 2; pos < data.size() && (int)pixels.size() < totalPixels; ) {
            if (data[pos] == 0x00 && pos + 2 < data.size()) {
                uint8_t count = data[pos + 1];
                uint8_t byte = data[pos + 2];
                if (count == 0) count = 1;
                for (int k = 0; k < count && (int)pixels.size() < totalPixels; ++k)
                    pixels.push_back(byte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }
        writeBMP("rle_00_count_byte_skip2.bmp", pixels);
    }

    // Test 6: Row-based with possible row headers
    {
        std::vector<uint8_t> pixels;
        size_t pos = 2;
        for (int row = 0; row < height && pos < data.size(); ++row) {
            // Maybe each row has a length prefix?
            int rowLen = width;
            for (int col = 0; col < rowLen && pos < data.size() && (int)pixels.size() < totalPixels; ) {
                if (data[pos] == 0xFF && pos + 2 < data.size()) {
                    uint8_t byte = data[pos + 1];
                    uint8_t count = data[pos + 2];
                    if (count == 0) count = 1;
                    for (int k = 0; k < count && col < rowLen; ++k, ++col)
                        pixels.push_back(byte);
                    pos += 3;
                } else {
                    pixels.push_back(data[pos]);
                    pos++;
                    col++;
                }
            }
        }
        writeBMP("rle_rowbased_skip2.bmp", pixels);
    }

    std::cout << "\nCheck the output files to see which format produces correct sprites.\n";

    // Test 7: Column-major storage (transposed)
    {
        std::vector<uint8_t> pixels(totalPixels, 0);
        size_t pos = 2;
        for (int col = 0; col < width && pos < data.size(); ++col) {
            for (int row = 0; row < height && pos < data.size(); ) {
                if (data[pos] == 0xFF && pos + 2 < data.size()) {
                    uint8_t byte = data[pos + 1];
                    uint8_t count = data[pos + 2];
                    if (count == 0) count = 1;
                    for (int k = 0; k < count && row < height; ++k, ++row) {
                        pixels[row * width + col] = byte;
                    }
                    pos += 3;
                } else {
                    pixels[row * width + col] = data[pos];
                    pos++;
                    row++;
                }
            }
        }
        writeBMP("column_major_rle.bmp", pixels);
    }

    // Test 8: Try different header interpretations (maybe byte[0] is height, byte[1] is width)
    {
        int swapW = height;  // Swap dimensions
        int swapH = width;
        std::vector<uint8_t> pixels;
        for (size_t pos = 2; pos < data.size() && (int)pixels.size() < swapW * swapH; ) {
            if (data[pos] == 0xFF && pos + 2 < data.size()) {
                uint8_t byte = data[pos + 1];
                uint8_t count = data[pos + 2];
                if (count == 0) count = 1;
                for (int k = 0; k < count && (int)pixels.size() < swapW * swapH; ++k)
                    pixels.push_back(byte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }
        // Write with swapped dimensions
        std::string path = outDir + "/swapped_dims_rle.bmp";
        std::ofstream out(path, std::ios::binary);
        if (out) {
            int rowSize = (swapW + 3) & ~3;
            int imageSize = rowSize * swapH;
            int bmpSize = 54 + 1024 + imageSize;
            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = swapW;
            *(int32_t*)&bmpHeader[22] = swapH;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;
            *(uint32_t*)&bmpHeader[46] = 256;
            out.write(reinterpret_cast<char*>(bmpHeader), 54);
            out.write(reinterpret_cast<const char*>(palette), 1024);
            std::vector<uint8_t> row(rowSize, 0);
            for (int y = swapH - 1; y >= 0; --y) {
                for (int x = 0; x < swapW; ++x) {
                    size_t idx = y * swapW + x;
                    row[x] = (idx < pixels.size()) ? pixels[idx] : 0;
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }
            std::cout << "Wrote: " << path << "\n";
        }
    }

    // Test 9: Maybe the RLE count is the next byte value repeated?
    // FF XX YY might mean: output YY copies of value (XX)
    // No wait, that's what we tried. Let me try: XX literal pixels followed by YY run
    {
        std::vector<uint8_t> pixels;
        size_t pos = 2;
        while (pos < data.size() && (int)pixels.size() < totalPixels) {
            if (data[pos] == 0xFF && pos + 2 < data.size()) {
                // Skip marker? Or treat differently
                uint8_t nextByte = data[pos + 1];
                uint8_t count = data[pos + 2];
                // Maybe FF means "run of count+1 pixels of value nextByte"
                for (int k = 0; k <= count && (int)pixels.size() < totalPixels; ++k)
                    pixels.push_back(nextByte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }
        writeBMP("rle_count_plus_one.bmp", pixels);
    }

    // Test 10: What if FF is NOT a special marker but just a regular pixel?
    // And compression is different
    {
        std::vector<uint8_t> pixels(data.begin() + 2, data.begin() + 2 + totalPixels);
        // Flip horizontally
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width / 2; ++x) {
                std::swap(pixels[y * width + x], pixels[y * width + (width - 1 - x)]);
            }
        }
        writeBMP("raw_hflip.bmp", pixels);
    }

    // Test 11: count+1 RLE but written top-to-bottom (no BMP flip)
    {
        std::vector<uint8_t> pixels;
        size_t pos = 2;
        while (pos < data.size() && (int)pixels.size() < totalPixels) {
            if (data[pos] == 0xFF && pos + 2 < data.size()) {
                uint8_t nextByte = data[pos + 1];
                uint8_t count = data[pos + 2];
                for (int k = 0; k <= count && (int)pixels.size() < totalPixels; ++k)
                    pixels.push_back(nextByte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }
        // Write WITHOUT the standard BMP y-flip (top-to-bottom)
        std::string path = outDir + "/rle_count_plus_one_topdown.bmp";
        std::ofstream out(path, std::ios::binary);
        if (out) {
            int rowSize = (width + 3) & ~3;
            int imageSize = rowSize * height;
            int bmpSize = 54 + 1024 + imageSize;
            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = width;
            *(int32_t*)&bmpHeader[22] = height;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;
            *(uint32_t*)&bmpHeader[46] = 256;
            out.write(reinterpret_cast<char*>(bmpHeader), 54);
            out.write(reinterpret_cast<const char*>(palette), 1024);
            std::vector<uint8_t> row(rowSize, 0);
            // Write rows in order (y=0 first, which BMP displays at bottom)
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    size_t idx = y * width + x;
                    row[x] = (idx < pixels.size()) ? pixels[idx] : 0;
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }
            std::cout << "Wrote: " << path << "\n";
        }
    }

    // Test 12: count+1 RLE with vertically flipped pixel array
    {
        std::vector<uint8_t> pixels;
        size_t pos = 2;
        while (pos < data.size() && (int)pixels.size() < totalPixels) {
            if (data[pos] == 0xFF && pos + 2 < data.size()) {
                uint8_t nextByte = data[pos + 1];
                uint8_t count = data[pos + 2];
                for (int k = 0; k <= count && (int)pixels.size() < totalPixels; ++k)
                    pixels.push_back(nextByte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }
        // Flip the pixel array vertically
        std::vector<uint8_t> flipped(totalPixels, 0);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int srcIdx = y * width + x;
                int dstIdx = (height - 1 - y) * width + x;
                if (srcIdx < (int)pixels.size()) flipped[dstIdx] = pixels[srcIdx];
            }
        }
        writeBMP("rle_count_plus_one_vflip.bmp", flipped);
    }

    // Test 13a: count+1 RLE with vflip, test different widths
    for (int testWidth = 8; testWidth <= 20; ++testWidth) {
        int testHeight = totalPixels / testWidth;
        if (testHeight < 4) continue;

        std::vector<uint8_t> pixels;
        size_t pos = 2;
        int testTotal = testWidth * testHeight;
        while (pos < data.size() && (int)pixels.size() < testTotal) {
            if (data[pos] == 0xFF && pos + 2 < data.size()) {
                uint8_t nextByte = data[pos + 1];
                uint8_t count = data[pos + 2];
                for (int k = 0; k <= count && (int)pixels.size() < testTotal; ++k)
                    pixels.push_back(nextByte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }

        // Flip vertically
        std::vector<uint8_t> flipped(testTotal, 0);
        for (int y = 0; y < testHeight; ++y) {
            for (int x = 0; x < testWidth; ++x) {
                int srcIdx = y * testWidth + x;
                int dstIdx = (testHeight - 1 - y) * testWidth + x;
                if (srcIdx < (int)pixels.size()) flipped[dstIdx] = pixels[srcIdx];
            }
        }

        char filename[64];
        snprintf(filename, sizeof(filename), "width_%02d_vflip.bmp", testWidth);
        std::string path = outDir + "/" + filename;
        std::ofstream out(path, std::ios::binary);
        if (out) {
            int rowSize = (testWidth + 3) & ~3;
            int imageSize = rowSize * testHeight;
            int bmpSize = 54 + 1024 + imageSize;
            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = testWidth;
            *(int32_t*)&bmpHeader[22] = testHeight;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;
            *(uint32_t*)&bmpHeader[46] = 256;
            out.write(reinterpret_cast<char*>(bmpHeader), 54);
            out.write(reinterpret_cast<const char*>(palette), 1024);
            std::vector<uint8_t> row(rowSize, 0);
            for (int y = testHeight - 1; y >= 0; --y) {
                for (int x = 0; x < testWidth; ++x) {
                    size_t idx = y * testWidth + x;
                    row[x] = (idx < flipped.size()) ? flipped[idx] : 0;
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }
        }
    }
    std::cout << "Wrote width test files (width_08_vflip.bmp to width_20_vflip.bmp)\n";

    // Test 13: Try dimensions from raw header bytes differently
    // Maybe it's height first, then width?
    {
        int h = data[0];  // 0x0B = 11
        int w = data[1];  // 0x11 = 17
        int total = w * h;
        std::vector<uint8_t> pixels;
        size_t pos = 2;
        while (pos < data.size() && (int)pixels.size() < total) {
            if (data[pos] == 0xFF && pos + 2 < data.size()) {
                uint8_t nextByte = data[pos + 1];
                uint8_t count = data[pos + 2];
                for (int k = 0; k <= count && (int)pixels.size() < total; ++k)
                    pixels.push_back(nextByte);
                pos += 3;
            } else {
                pixels.push_back(data[pos]);
                pos++;
            }
        }
        std::string path = outDir + "/rle_height_width_swap.bmp";
        std::ofstream out(path, std::ios::binary);
        if (out) {
            int rowSize = (w + 3) & ~3;
            int imageSize = rowSize * h;
            int bmpSize = 54 + 1024 + imageSize;
            uint8_t bmpHeader[54] = {'B', 'M'};
            *(uint32_t*)&bmpHeader[2] = bmpSize;
            *(uint32_t*)&bmpHeader[10] = 54 + 1024;
            *(uint32_t*)&bmpHeader[14] = 40;
            *(int32_t*)&bmpHeader[18] = w;
            *(int32_t*)&bmpHeader[22] = h;
            *(uint16_t*)&bmpHeader[26] = 1;
            *(uint16_t*)&bmpHeader[28] = 8;
            *(uint32_t*)&bmpHeader[34] = imageSize;
            *(uint32_t*)&bmpHeader[46] = 256;
            out.write(reinterpret_cast<char*>(bmpHeader), 54);
            out.write(reinterpret_cast<const char*>(palette), 1024);
            std::vector<uint8_t> row(rowSize, 0);
            for (int y = h - 1; y >= 0; --y) {
                for (int x = 0; x < w; ++x) {
                    size_t idx = y * w + x;
                    row[x] = (idx < pixels.size()) ? pixels[idx] : 0;
                }
                out.write(reinterpret_cast<char*>(row.data()), rowSize);
            }
            std::cout << "Wrote: " << path << "\n";
        }
    }
}

void extractWithDims(const std::string& datPath, const std::string& palettePath,
                     uint32_t offset, int width, int height, bool hasHeader,
                     const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(32768);  // Larger buffer for big sprites
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int totalPixels = width * height;
    int dataStart = hasHeader ? 2 : 0;

    std::cout << "Extracting sprite at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << " = " << totalPixels << " pixels\n";
    std::cout << "Header skip: " << dataStart << " bytes\n";
    std::cout << "First 20 bytes: ";
    for (int i = 0; i < 20 && i < (int)data.size(); ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    // Decompress
    std::vector<uint8_t> pixels;
    pixels.reserve(totalPixels);

    for (size_t pos = dataStart; pos < data.size() && (int)pixels.size() < totalPixels; ) {
        if (data[pos] == 0xFF && pos + 2 < data.size()) {
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            if (count == 0) count = 1;
            for (int k = 0; k < count && (int)pixels.size() < totalPixels; ++k) {
                pixels.push_back(byte);
            }
            pos += 3;
        } else {
            pixels.push_back(data[pos]);
            pos++;
        }
    }

    std::cout << "Decompressed " << pixels.size() << " pixels (expected " << totalPixels << ")\n";

    if ((int)pixels.size() < totalPixels) {
        std::cerr << "Warning: Not enough pixels decompressed!\n";
    }

    // Write BMP
    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to create output file\n";
        return;
    }

    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);
    out.write(reinterpret_cast<char*>(palette), 1024);

    std::vector<uint8_t> row(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = y * width + x;
            row[x] = (idx < pixels.size()) ? pixels[idx] : 0;
        }
        out.write(reinterpret_cast<char*>(row.data()), rowSize);
    }

    std::cout << "Wrote: " << outPath << "\n";
}

// Bounded RLE with explicit dimensions (no header in data)
void extractExplicitDims(const std::string& datPath, const std::string& palettePath,
                         uint32_t offset, int width, int height, const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) return;

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(4096);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int totalPixels = width * height;
    std::cout << "Explicit dims extraction at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << " = " << totalPixels << " pixels\n";
    std::cout << "Raw data: ";
    for (int i = 0; i < 40; ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    std::vector<uint8_t> pixels(totalPixels, 0);
    int pixelIdx = 0;

    size_t pos = 0;  // No header to skip
    while (pixelIdx < totalPixels && pos < data.size()) {
        if (data[pos] == 0xFF && pos + 2 < data.size()) {
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            for (int k = 0; k <= count && pixelIdx < totalPixels; ++k) {
                pixels[pixelIdx++] = byte;
            }
            pos += 3;
        } else if (data[pos] == 0x00 && pos + 1 < data.size()) {
            uint8_t count = data[pos + 1];
            pixelIdx += (count + 1);
            if (pixelIdx > totalPixels) pixelIdx = totalPixels;
            pos += 2;
        } else {
            if (pixelIdx < totalPixels) {
                pixels[pixelIdx++] = data[pos];
            }
            pos++;
        }
    }

    std::cout << "Filled " << pixelIdx << " / " << totalPixels << " pixels\n";
    std::cout << "Data consumed: " << pos << " bytes\n";

    // Vertical flip and write BMP
    std::vector<uint8_t> flipped(totalPixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            flipped[(height - 1 - y) * width + x] = pixels[y * width + x];
        }
    }

    std::ofstream out(outPath, std::ios::binary);
    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);

    uint8_t modPalette[256][4];
    memcpy(modPalette, palette, 1024);
    modPalette[0][0] = 255;
    modPalette[0][1] = 0;
    modPalette[0][2] = 255;
    modPalette[0][3] = 0;
    out.write(reinterpret_cast<char*>(modPalette), 1024);

    std::vector<uint8_t> bmpRow(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            bmpRow[x] = flipped[y * width + x];
        }
        out.write(reinterpret_cast<char*>(bmpRow.data()), rowSize);
    }

    std::cout << "Wrote: " << outPath << "\n";
}

// Bounded RLE extraction with max data size
// FF <byte> <count> = repeat byte (count+1) times
// 00 <count> = skip (count+1) pixels
// Stops at dataSize bytes to avoid reading into next sprite
void extractBoundedRLE(const std::string& datPath, const std::string& palettePath,
                       uint32_t offset, int dataSize, const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(dataSize);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int width = data[0];
    int height = data[1];
    int totalPixels = width * height;

    std::cout << "Bounded RLE at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << " = " << totalPixels << " pixels\n";
    std::cout << "Data size limit: " << dataSize << " bytes\n";
    std::cout << "Raw data: ";
    for (int i = 0; i < dataSize && i < 40; ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    std::vector<uint8_t> pixels(totalPixels, 0);
    int pixelIdx = 0;

    size_t pos = 2;  // Skip dimension header
    while (pixelIdx < totalPixels && pos < (size_t)dataSize) {
        if (data[pos] == 0xFF && pos + 2 < (size_t)dataSize) {
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            std::cout << "  [" << pos << "] RLE 0x" << std::hex << (int)byte
                      << " x " << std::dec << (count + 1) << " -> pos " << pixelIdx << "\n";
            for (int k = 0; k <= count && pixelIdx < totalPixels; ++k) {
                pixels[pixelIdx++] = byte;
            }
            pos += 3;
        } else if (data[pos] == 0x00 && pos + 1 < (size_t)dataSize) {
            uint8_t count = data[pos + 1];
            std::cout << "  [" << pos << "] Skip " << (count + 1) << " -> pos " << pixelIdx << "\n";
            pixelIdx += (count + 1);
            if (pixelIdx > totalPixels) pixelIdx = totalPixels;  // Clamp
            pos += 2;
        } else {
            std::cout << "  [" << pos << "] Literal 0x" << std::hex << (int)data[pos]
                      << std::dec << " -> pos " << pixelIdx << "\n";
            if (pixelIdx < totalPixels) {
                pixels[pixelIdx++] = data[pos];
            }
            pos++;
        }
    }

    std::cout << "\nFilled " << pixelIdx << " / " << totalPixels << " pixels\n";
    std::cout << "Data consumed: " << pos - 2 << " bytes\n";

    // Show pixel values
    std::map<uint8_t, int> pixelCounts;
    for (uint8_t p : pixels) pixelCounts[p]++;
    std::cout << "Pixel values:\n";
    for (auto& kv : pixelCounts) {
        printf("  0x%02X: %d\n", kv.first, kv.second);
    }

    // Vertical flip and write BMP
    std::vector<uint8_t> flipped(totalPixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            flipped[(height - 1 - y) * width + x] = pixels[y * width + x];
        }
    }

    std::ofstream out(outPath, std::ios::binary);
    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);

    uint8_t modPalette[256][4];
    memcpy(modPalette, palette, 1024);
    modPalette[0][0] = 255;
    modPalette[0][1] = 0;
    modPalette[0][2] = 255;
    modPalette[0][3] = 0;
    out.write(reinterpret_cast<char*>(modPalette), 1024);

    std::vector<uint8_t> bmpRow(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            bmpRow[x] = flipped[y * width + x];
        }
        out.write(reinterpret_cast<char*>(bmpRow.data()), rowSize);
    }

    std::cout << "\nWrote: " << outPath << "\n";
}

// Skip-based RLE:
// FF <byte> <count> = repeat byte (count+1) times
// 00 <count> = skip (count+1) pixels (leave transparent)
// Other bytes = literal pixel values
void extractSkipRLE(const std::string& datPath, const std::string& palettePath,
                    uint32_t offset, const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(4096);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int width = data[0];
    int height = data[1];
    int totalPixels = width * height;

    std::cout << "Skip-based RLE extraction at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << " = " << totalPixels << " pixels\n";
    std::cout << "Raw data: ";
    for (int i = 0; i < 60 && i < (int)data.size(); ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    std::vector<uint8_t> pixels(totalPixels, 0);  // Initialize all transparent
    int pixelIdx = 0;

    size_t pos = 2;
    while (pixelIdx < totalPixels && pos < data.size()) {
        if (data[pos] == 0xFF && pos + 2 < data.size()) {
            // RLE: FF <byte> <count>
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            std::cout << "  Pos " << pixelIdx << ": RLE 0x" << std::hex << (int)byte
                      << " x " << std::dec << (count + 1) << "\n";
            for (int k = 0; k <= count && pixelIdx < totalPixels; ++k) {
                pixels[pixelIdx++] = byte;
            }
            pos += 3;
        } else if (data[pos] == 0x00 && pos + 1 < data.size()) {
            // Skip: 00 <count> - advance position by (count+1)
            uint8_t count = data[pos + 1];
            std::cout << "  Pos " << pixelIdx << ": Skip " << (count + 1) << "\n";
            pixelIdx += (count + 1);
            pos += 2;
        } else {
            // Literal pixel
            if (pixelIdx < totalPixels) {
                pixels[pixelIdx++] = data[pos];
            }
            pos++;
        }
    }

    std::cout << "\nFilled to position " << pixelIdx << " (expected " << totalPixels << ")\n";

    // Show pixel value distribution
    std::map<uint8_t, int> pixelCounts;
    for (uint8_t p : pixels) pixelCounts[p]++;
    std::cout << "Pixel values:\n";
    for (auto& kv : pixelCounts) {
        printf("  0x%02X: %d\n", kv.first, kv.second);
    }

    // Vertical flip
    std::vector<uint8_t> flipped(totalPixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t srcIdx = y * width + x;
            size_t dstIdx = (height - 1 - y) * width + x;
            flipped[dstIdx] = pixels[srcIdx];
        }
    }

    std::ofstream out(outPath, std::ios::binary);
    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);

    uint8_t modPalette[256][4];
    memcpy(modPalette, palette, 1024);
    modPalette[0][0] = 255;
    modPalette[0][1] = 0;
    modPalette[0][2] = 255;
    modPalette[0][3] = 0;
    out.write(reinterpret_cast<char*>(modPalette), 1024);

    std::vector<uint8_t> bmpRow(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            bmpRow[x] = flipped[y * width + x];
        }
        out.write(reinterpret_cast<char*>(bmpRow.data()), rowSize);
    }

    std::cout << "\nWrote: " << outPath << "\n";
}

// Clean RLE: like basic RLE but filter out low values (likely control bytes)
// FF <byte> <count> = repeat byte (count+1) times
// Values < 0x10 are treated as transparent (likely control bytes)
void extractCleanRLE(const std::string& datPath, const std::string& palettePath,
                     uint32_t offset, const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(4096);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int width = data[0];
    int height = data[1];
    int totalPixels = width * height;

    std::cout << "Clean RLE extraction at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << " = " << totalPixels << " pixels\n";

    std::vector<uint8_t> pixels;
    pixels.reserve(totalPixels);

    size_t pos = 2;
    while (pixels.size() < (size_t)totalPixels && pos < data.size()) {
        if (data[pos] == 0xFF && pos + 2 < data.size()) {
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            for (int k = 0; k <= count && pixels.size() < (size_t)totalPixels; ++k) {
                pixels.push_back(byte);
            }
            pos += 3;
        } else {
            pixels.push_back(data[pos]);
            pos++;
        }
    }

    // Clean pass: replace low values with transparent
    int cleaned = 0;
    for (size_t i = 0; i < pixels.size(); ++i) {
        if (pixels[i] < 0x10 && pixels[i] != 0x00) {  // Keep 0x00 as-is (already transparent)
            pixels[i] = 0;
            cleaned++;
        }
    }
    std::cout << "Cleaned " << cleaned << " low-value pixels\n";

    // Vertical flip
    std::vector<uint8_t> flipped(totalPixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t srcIdx = y * width + x;
            size_t dstIdx = (height - 1 - y) * width + x;
            flipped[dstIdx] = (srcIdx < pixels.size()) ? pixels[srcIdx] : 0;
        }
    }

    std::ofstream out(outPath, std::ios::binary);
    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);

    uint8_t modPalette[256][4];
    memcpy(modPalette, palette, 1024);
    modPalette[0][0] = 255;
    modPalette[0][1] = 0;
    modPalette[0][2] = 255;
    modPalette[0][3] = 0;
    out.write(reinterpret_cast<char*>(modPalette), 1024);

    std::vector<uint8_t> bmpRow(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            bmpRow[x] = flipped[y * width + x];
        }
        out.write(reinterpret_cast<char*>(bmpRow.data()), rowSize);
    }

    std::cout << "Wrote: " << outPath << "\n";
}

// Row-based RLE:
// Each row starts with data, ends with 00 <skip>
// FF <byte> <count> = repeat byte (count+1) times
// 00 <skip> = end of row data, fill remaining with transparent, <skip> = left padding for next row
// Other bytes = literal pixel values
void extractRowBasedRLE(const std::string& datPath, const std::string& palettePath,
                        uint32_t offset, const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(4096);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int width = data[0];
    int height = data[1];
    int totalPixels = width * height;

    std::cout << "Row-based RLE extraction at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << " = " << totalPixels << " pixels\n";
    std::cout << "First 60 bytes: ";
    for (int i = 0; i < 60 && i < (int)data.size(); ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    std::vector<uint8_t> pixels(totalPixels, 0);  // Initialize all to transparent
    int row = 0;
    int col = 0;

    size_t pos = 2;  // Skip dimension header
    while (row < height && pos < data.size()) {
        if (data[pos] == 0xFF && pos + 2 < data.size()) {
            // RLE: FF <byte> <count>
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            std::cout << "  Row " << row << " Col " << col << ": RLE 0x" << std::hex << (int)byte
                      << " x " << std::dec << (count + 1) << "\n";
            for (int k = 0; k <= count && col < width; ++k) {
                pixels[row * width + col] = byte;
                col++;
            }
            // Handle row overflow
            while (col >= width && row < height) {
                col -= width;
                row++;
            }
            pos += 3;
        } else if (data[pos] == 0x00 && pos + 1 < data.size()) {
            // End of row, next byte is left padding for next row
            uint8_t leftPad = data[pos + 1];
            std::cout << "  Row " << row << " Col " << col << ": End row, next row pad=" << (int)leftPad << "\n";
            // Fill rest of current row with transparent
            while (col < width) {
                pixels[row * width + col] = 0;
                col++;
            }
            row++;
            col = leftPad;  // Start next row with padding
            pos += 2;
        } else {
            // Literal pixel
            if (col < width && row < height) {
                pixels[row * width + col] = data[pos];
                col++;
                // Handle row overflow
                if (col >= width) {
                    col = 0;
                    row++;
                }
            }
            pos++;
        }
    }

    std::cout << "\nFilled " << row << " rows (expected " << height << ")\n";
    std::cout << "Data consumed: " << pos - 2 << " bytes\n";

    // Show pixel value distribution
    std::map<uint8_t, int> pixelCounts;
    for (uint8_t p : pixels) pixelCounts[p]++;
    std::cout << "\nPixel values:\n";
    for (auto& kv : pixelCounts) {
        printf("  0x%02X: %d\n", kv.first, kv.second);
    }

    // Vertical flip and write BMP
    std::vector<uint8_t> flipped(totalPixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t srcIdx = y * width + x;
            size_t dstIdx = (height - 1 - y) * width + x;
            flipped[dstIdx] = pixels[srcIdx];
        }
    }

    std::ofstream out(outPath, std::ios::binary);
    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);

    uint8_t modPalette[256][4];
    memcpy(modPalette, palette, 1024);
    modPalette[0][0] = 255;
    modPalette[0][1] = 0;
    modPalette[0][2] = 255;
    modPalette[0][3] = 0;
    out.write(reinterpret_cast<char*>(modPalette), 1024);

    std::vector<uint8_t> bmpRow(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            bmpRow[x] = flipped[y * width + x];
        }
        out.write(reinterpret_cast<char*>(bmpRow.data()), rowSize);
    }

    std::cout << "\nWrote: " << outPath << "\n";
}

// Test RLE with transparency runs:
// FF <byte> <count> = repeat byte (count+1) times
// 00 <count> = transparency run of (count+1) pixels (index 0)
// Other bytes = literal pixel values
void extractRLETransparency(const std::string& datPath, const std::string& palettePath,
                            uint32_t offset, const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(4096);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int width = data[0];
    int height = data[1];
    int totalPixels = width * height;

    std::cout << "RLE + Transparency extraction at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << " = " << totalPixels << " pixels\n";
    std::cout << "First 50 bytes: ";
    for (int i = 0; i < 50 && i < (int)data.size(); ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    std::vector<uint8_t> pixels;
    pixels.reserve(totalPixels);

    size_t pos = 2;  // Skip dimension header
    while (pixels.size() < (size_t)totalPixels && pos < data.size()) {
        if (data[pos] == 0xFF && pos + 2 < data.size()) {
            // RLE: FF <byte> <count>
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            std::cout << "  RLE @ " << pos << ": 0x" << std::hex << (int)byte
                      << " x " << std::dec << (count + 1) << "\n";
            for (int k = 0; k <= count && pixels.size() < (size_t)totalPixels; ++k) {
                pixels.push_back(byte);
            }
            pos += 3;
        } else if (data[pos] == 0x00 && pos + 1 < data.size()) {
            // Transparency run: 00 <count>
            uint8_t count = data[pos + 1];
            std::cout << "  Trans @ " << pos << ": " << (count + 1) << " transparent pixels\n";
            for (int k = 0; k <= count && pixels.size() < (size_t)totalPixels; ++k) {
                pixels.push_back(0);  // Transparent (index 0)
            }
            pos += 2;
        } else {
            // Literal pixel
            pixels.push_back(data[pos]);
            pos++;
        }
    }

    std::cout << "\nDecompressed " << pixels.size() << " pixels (expected " << totalPixels << ")\n";
    std::cout << "Data consumed: " << pos - 2 << " bytes\n";

    // Show pixel value distribution
    std::map<uint8_t, int> pixelCounts;
    for (uint8_t p : pixels) pixelCounts[p]++;
    std::cout << "\nPixel values:\n";
    for (auto& kv : pixelCounts) {
        printf("  0x%02X: %d\n", kv.first, kv.second);
    }

    // Vertical flip and write BMP
    std::vector<uint8_t> flipped(totalPixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t srcIdx = y * width + x;
            size_t dstIdx = (height - 1 - y) * width + x;
            flipped[dstIdx] = (srcIdx < pixels.size()) ? pixels[srcIdx] : 0;
        }
    }

    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to create output file\n";
        return;
    }

    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);

    uint8_t modPalette[256][4];
    memcpy(modPalette, palette, 1024);
    modPalette[0][0] = 255;  // B
    modPalette[0][1] = 0;    // G
    modPalette[0][2] = 255;  // R (magenta for transparency)
    modPalette[0][3] = 0;
    out.write(reinterpret_cast<char*>(modPalette), 1024);

    std::vector<uint8_t> row(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = y * width + x;
            row[x] = (idx < flipped.size()) ? flipped[idx] : 0;
        }
        out.write(reinterpret_cast<char*>(row.data()), rowSize);
    }

    std::cout << "\nWrote: " << outPath << "\n";
}

// Test RLE with literal count interpretation:
// FF <byte> <count> = repeat byte (count+1) times
// <count> <bytes...> = literal count followed by that many literal bytes (if first byte != 0xFF)
void extractRLELiteralCount(const std::string& datPath, const std::string& palettePath,
                            uint32_t offset, const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(4096);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    int width = data[0];
    int height = data[1];
    int totalPixels = width * height;

    std::cout << "RLE + Literal Count extraction at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions: " << width << "x" << height << " = " << totalPixels << " pixels\n";
    std::cout << "First 40 bytes: ";
    for (int i = 0; i < 40 && i < (int)data.size(); ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    std::vector<uint8_t> pixels;
    pixels.reserve(totalPixels);

    size_t pos = 2;  // Skip dimension header
    while (pixels.size() < (size_t)totalPixels && pos < data.size()) {
        if (data[pos] == 0xFF && pos + 2 < data.size()) {
            // RLE: FF <byte> <count>
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            std::cout << "  RLE at " << pos << ": repeat 0x" << std::hex << (int)byte
                      << " x " << std::dec << (count + 1) << "\n";
            for (int k = 0; k <= count && pixels.size() < (size_t)totalPixels; ++k) {
                pixels.push_back(byte);
            }
            pos += 3;
        } else {
            // Literal count: <count> <bytes...>
            uint8_t literalCount = data[pos];
            if (literalCount == 0) {
                std::cout << "  Zero literal count at " << pos << ", skipping\n";
                pos++;
                continue;
            }
            std::cout << "  Literal at " << pos << ": " << (int)literalCount << " bytes: ";
            pos++;
            for (int k = 0; k < literalCount && pixels.size() < (size_t)totalPixels && pos < data.size(); ++k) {
                std::cout << std::hex << (int)data[pos] << " ";
                pixels.push_back(data[pos]);
                pos++;
            }
            std::cout << std::dec << "\n";
        }
    }

    std::cout << "\nDecompressed " << pixels.size() << " pixels (expected " << totalPixels << ")\n";
    std::cout << "Data consumed: " << pos - 2 << " bytes\n";

    // Show pixel value distribution
    std::map<uint8_t, int> pixelCounts;
    for (uint8_t p : pixels) pixelCounts[p]++;
    std::cout << "\nPixel values:\n";
    for (auto& kv : pixelCounts) {
        printf("  0x%02X: %d\n", kv.first, kv.second);
    }

    // Vertical flip and write BMP
    std::vector<uint8_t> flipped(totalPixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t srcIdx = y * width + x;
            size_t dstIdx = (height - 1 - y) * width + x;
            flipped[dstIdx] = (srcIdx < pixels.size()) ? pixels[srcIdx] : 0;
        }
    }

    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to create output file\n";
        return;
    }

    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);

    uint8_t modPalette[256][4];
    memcpy(modPalette, palette, 1024);
    modPalette[0][0] = 255;
    modPalette[0][1] = 0;
    modPalette[0][2] = 255;
    modPalette[0][3] = 0;
    out.write(reinterpret_cast<char*>(modPalette), 1024);

    std::vector<uint8_t> row(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = y * width + x;
            row[x] = (idx < flipped.size()) ? flipped[idx] : 0;
        }
        out.write(reinterpret_cast<char*>(row.data()), rowSize);
    }

    std::cout << "\nWrote: " << outPath << "\n";
}

void extractSingleSprite(const std::string& datPath, const std::string& palettePath,
                         uint32_t offset, const std::string& outPath) {
    std::ifstream file(datPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open DAT file\n";
        return;
    }

    uint8_t palette[256][4] = {};
    loadPalette(palettePath, palette);

    file.seekg(offset);
    std::vector<uint8_t> data(4096);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    // Read dimensions from header
    int width = data[0];
    int height = data[1];
    int totalPixels = width * height;

    std::cout << "Extracting sprite at 0x" << std::hex << offset << std::dec << "\n";
    std::cout << "Dimensions from header: " << width << "x" << height << " = " << totalPixels << " pixels\n";
    std::cout << "First 30 bytes: ";
    for (int i = 0; i < 30 && i < (int)data.size(); ++i) printf("%02X ", data[i]);
    std::cout << "\n\n";

    // Decompress RLE with row terminators
    // FF <byte> <count> = repeat byte (count+1) times
    // 00 = row terminator (skip to next row start)
    // Other = literal pixel
    std::vector<uint8_t> pixels;
    pixels.reserve(totalPixels);

    int currentRow = 0;
    int currentCol = 0;

    size_t pos = 2;  // Skip dimension header
    while ((int)pixels.size() < totalPixels && pos < data.size()) {
        if (data[pos] == 0xFF && pos + 2 < data.size()) {
            uint8_t byte = data[pos + 1];
            uint8_t count = data[pos + 2];
            for (int k = 0; k <= count && (int)pixels.size() < totalPixels; ++k) {
                pixels.push_back(byte);
                currentCol++;
            }
            pos += 3;
        } else if (data[pos] == 0x00) {
            // Row terminator - fill rest of row with transparent and move to next
            while (currentCol < width && (int)pixels.size() < totalPixels) {
                pixels.push_back(0);  // Transparent
                currentCol++;
            }
            currentCol = 0;
            currentRow++;
            pos++;
        } else {
            pixels.push_back(data[pos]);
            currentCol++;
            pos++;
        }
    }

    // Pad remaining with zeros
    while ((int)pixels.size() < totalPixels) {
        pixels.push_back(0);
    }

    std::cout << "Decompressed " << pixels.size() << " pixels (expected " << totalPixels << ")\n";
    std::cout << "Compressed data used: " << pos - 2 << " bytes\n";

    // Show pixel value distribution
    std::map<uint8_t, int> pixelCounts;
    for (uint8_t p : pixels) pixelCounts[p]++;
    std::cout << "Unique pixel values and counts:\n";
    for (auto& kv : pixelCounts) {
        printf("  0x%02X: %d\n", kv.first, kv.second);
    }

    // Vertical flip
    std::vector<uint8_t> flipped(totalPixels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            flipped[(height - 1 - y) * width + x] = pixels[y * width + x];
        }
    }

    // Write BMP
    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to create output file\n";
        return;
    }

    int rowSize = (width + 3) & ~3;
    int imageSize = rowSize * height;
    int bmpSize = 54 + 1024 + imageSize;

    uint8_t bmpHeader[54] = {'B', 'M'};
    *(uint32_t*)&bmpHeader[2] = bmpSize;
    *(uint32_t*)&bmpHeader[10] = 54 + 1024;
    *(uint32_t*)&bmpHeader[14] = 40;
    *(int32_t*)&bmpHeader[18] = width;
    *(int32_t*)&bmpHeader[22] = height;
    *(uint16_t*)&bmpHeader[26] = 1;
    *(uint16_t*)&bmpHeader[28] = 8;
    *(uint32_t*)&bmpHeader[34] = imageSize;
    *(uint32_t*)&bmpHeader[46] = 256;

    out.write(reinterpret_cast<char*>(bmpHeader), 54);

    // Make palette index 0 magenta to highlight transparency
    uint8_t modPalette[256][4];
    memcpy(modPalette, palette, 1024);
    modPalette[0][0] = 255;  // B
    modPalette[0][1] = 0;    // G
    modPalette[0][2] = 255;  // R (magenta)
    modPalette[0][3] = 0;
    out.write(reinterpret_cast<char*>(modPalette), 1024);

    // Write pixel data (BMP is bottom-up)
    std::vector<uint8_t> row(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = y * width + x;
            row[x] = (idx < flipped.size()) ? flipped[idx] : 0;
        }
        out.write(reinterpret_cast<char*>(row.data()), rowSize);
    }

    std::cout << "Wrote: " << outPath << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    if (command == "list-ne" && argc >= 3) {
        listNE(argv[2]);
    } else if (command == "extract-ne" && argc >= 4) {
        extractNE(argv[2], argv[3]);
    } else if (command == "list-grp" && argc >= 3) {
        listGRP(argv[2]);
    } else if (command == "extract-grp" && argc >= 4) {
        extractGRP(argv[2], argv[3]);
    } else if (command == "info" && argc >= 3) {
        showInfo(argv[2]);
    } else if (command == "validate" && argc >= 3) {
        return validateGame(argv[2]) ? 0 : 1;
    } else if (command == "analyze-sprites" && argc >= 3) {
        analyzeSprites(argv[2]);
    } else if (command == "analyze-ne" && argc >= 3) {
        analyzeNEStructure(argv[2]);
    } else if (command == "analyze-entities" && argc >= 3) {
        analyzeEntities(argv[2]);
    } else if (command == "analyze-aseq" && argc >= 3) {
        analyzeASEQ(argv[2]);
    } else if (command == "analyze-sprite-res" && argc >= 3) {
        analyzeSpriteResource(argv[2]);
    } else if (command == "analyze-raw" && argc >= 3) {
        analyzeRawFileStructure(argv[2]);
    } else if (command == "analyze-rle" && argc >= 3) {
        analyzeRLEFormat(argv[2]);
    } else if (command == "analyze-index" && argc >= 3) {
        analyzeSpriteIndex(argv[2]);
    } else if (command == "deep-analyze" && argc >= 3) {
        deepAnalyzeSprites(argv[2]);
    } else if (command == "trace-offsets" && argc >= 3) {
        traceSpritesOffsets(argv[2]);
    } else if (command == "extract-sprite" && argc >= 8) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        int width = std::stoi(argv[5]);
        int height = std::stoi(argv[6]);
        extractSprite(argv[2], argv[3], offset, width, height, argv[7]);
    } else if (command == "extract-all" && argc >= 5) {
        extractAllSprites(argv[2], argv[3], argv[4]);
    } else if (command == "analyze-meta" && argc >= 3) {
        analyzeSpriteMetadata(argv[2]);
    } else if (command == "extract-v2" && argc >= 5) {
        extractSpritesV2(argv[2], argv[3], argv[4]);
    } else if (command == "extract-raw" && argc >= 5) {
        extractSpritesRaw(argv[2], argv[3], argv[4]);
    } else if (command == "test-dims" && argc >= 5) {
        testDimensions(argv[2], argv[3], argv[4]);
    } else if (command == "find-width" && argc >= 5) {
        findWidth(argv[2], argv[3], argv[4]);
    } else if (command == "extract-single" && argc >= 6) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        extractSingleSprite(argv[2], argv[3], offset, argv[5]);
    } else if (command == "extract-indexed" && argc >= 5) {
        extractIndexedSprites(argv[2], argv[3], argv[4]);
    } else if (command == "extract-rund" && argc >= 5) {
        extractRundSprites(argv[2], argv[3], argv[4]);
    } else if (command == "dump-rund" && argc >= 3) {
        int count = (argc >= 4) ? std::stoi(argv[3]) : 5;
        dumpRundBytes(argv[2], count);
    } else if (command == "extract-labyrinth" && argc >= 4) {
        extractLabyrinthTilemaps(argv[2], argv[3]);
    } else if (command == "extract-labyrinth-sprites" && argc >= 5) {
        extractLabyrinthSprites(argv[2], argv[3], argv[4]);
    } else if (command == "extract-dims" && argc >= 8) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        int width = std::stoi(argv[5]);
        int height = std::stoi(argv[6]);
        bool hasHeader = (argc >= 9) ? (std::stoi(argv[7]) != 0) : false;
        const char* outPath = (argc >= 9) ? argv[8] : argv[7];
        extractWithDims(argv[2], argv[3], offset, width, height, hasHeader, outPath);
    } else if (command == "test-rle" && argc >= 8) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        int width = std::stoi(argv[5]);
        int height = std::stoi(argv[6]);
        testRLEFormats(argv[2], argv[3], offset, width, height, argv[7]);
    } else if (command == "rle-literal" && argc >= 6) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        extractRLELiteralCount(argv[2], argv[3], offset, argv[5]);
    } else if (command == "rle-trans" && argc >= 6) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        extractRLETransparency(argv[2], argv[3], offset, argv[5]);
    } else if (command == "rle-row" && argc >= 6) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        extractRowBasedRLE(argv[2], argv[3], offset, argv[5]);
    } else if (command == "rle-clean" && argc >= 6) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        extractCleanRLE(argv[2], argv[3], offset, argv[5]);
    } else if (command == "rle-skip" && argc >= 6) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        extractSkipRLE(argv[2], argv[3], offset, argv[5]);
    } else if (command == "rle-bounded" && argc >= 7) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        int dataSize = std::stoi(argv[5]);
        extractBoundedRLE(argv[2], argv[3], offset, dataSize, argv[6]);
    } else if (command == "rle-explicit" && argc >= 8) {
        uint32_t offset = std::stoul(argv[4], nullptr, 0);
        int width = std::stoi(argv[5]);
        int height = std::stoi(argv[6]);
        extractExplicitDims(argv[2], argv[3], offset, width, height, argv[7]);
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
