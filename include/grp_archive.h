#pragma once

#include "formats/grp_format.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>

struct SDL_Surface;

namespace opengg {

// File entry info
struct GrpEntry {
    std::string name;
    uint32_t offset;
    uint32_t size;
    uint32_t compressedSize;
    uint8_t flags;
    bool isCompressed;
};

// Decoded sprite
struct Sprite {
    int width;
    int height;
    int hotspotX;
    int hotspotY;
    std::vector<uint8_t> pixels;      // Indexed color (8-bit)
    std::vector<uint32_t> palette;    // RGBA palette (256 entries)
    bool hasPalette;
};

// GRP Archive Reader
// Handles RGrp format archives containing sprites and game assets
class GrpArchive {
public:
    GrpArchive();
    ~GrpArchive();

    // Open a GRP archive
    bool open(const std::string& path);

    // Close the archive
    void close();

    // Check if archive is open
    bool isOpen() const;

    // List all files in the archive
    std::vector<std::string> listFiles() const;

    // Get entry info by name
    const GrpEntry* getEntry(const std::string& name) const;

    // Extract raw file data
    std::vector<uint8_t> extract(const std::string& name);

    // Extract and decode as sprite
    std::unique_ptr<Sprite> extractSprite(const std::string& name);

    // Extract as SDL_Surface (requires SDL2)
    SDL_Surface* extractAsSurface(const std::string& name);

    // Set the palette to use for indexed sprites
    void setPalette(const std::vector<uint32_t>& palette);

    // Get last error
    std::string getLastError() const;

private:
    bool parseHeader();
    bool parseFileTable();
    std::vector<uint8_t> decompressRLE(const std::vector<uint8_t>& compressed, uint32_t uncompressedSize);
    std::vector<uint8_t> decompressLZ(const std::vector<uint8_t>& compressed, uint32_t uncompressedSize);
    std::unique_ptr<Sprite> decodeSprite(const std::vector<uint8_t>& data);

    std::string filePath_;
    std::vector<GrpEntry> entries_;
    std::unordered_map<std::string, size_t> entryMap_;
    std::vector<uint32_t> palette_;
    std::string lastError_;
    bool isOpen_ = false;
};

} // namespace opengg
