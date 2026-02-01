#include "grp_archive.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>

// SDL2 is optional for this module
#ifdef SDL_h_
#include <SDL.h>
#endif

namespace opengg {

GrpArchive::GrpArchive() {
    // Initialize default VGA palette (grayscale fallback)
    palette_.resize(256);
    for (int i = 0; i < 256; ++i) {
        uint8_t c = static_cast<uint8_t>(i);
        palette_[i] = (0xFF << 24) | (c << 16) | (c << 8) | c;  // RGBA
    }
}

GrpArchive::~GrpArchive() {
    close();
}

bool GrpArchive::open(const std::string& path) {
    close();
    filePath_ = path;

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        lastError_ = "Failed to open file: " + path;
        return false;
    }

    // Read and verify header
    GrpHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file) {
        lastError_ = "Failed to read GRP header";
        return false;
    }

    // Check magic
    if (std::memcmp(header.magic, GRP_MAGIC, 4) != 0) {
        lastError_ = "Invalid GRP magic (not RGrp)";
        return false;
    }

    // Parse file table
    // The file table typically starts after the header
    uint32_t tableOffset = sizeof(GrpHeader);
    file.seekg(tableOffset);

    // Read file count (first entry in table)
    uint32_t fileCount;
    file.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));
    if (!file || fileCount > 10000) {
        // Try alternative format - count might be at different location
        file.seekg(header.offset1);
        file.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));
        if (!file || fileCount > 10000) {
            lastError_ = "Invalid file count in GRP archive";
            return false;
        }
    }

    // Read file entries
    entries_.reserve(fileCount);
    for (uint32_t i = 0; i < fileCount; ++i) {
        GrpFileEntry entry;
        file.read(reinterpret_cast<char*>(&entry), sizeof(entry));
        if (!file) {
            break;
        }

        GrpEntry grpEntry;
        grpEntry.name = std::string(entry.name, strnlen(entry.name, 13));
        grpEntry.offset = entry.offset;
        grpEntry.size = entry.size;
        grpEntry.compressedSize = entry.compressedSize;
        grpEntry.flags = entry.flags;
        grpEntry.isCompressed = (entry.flags & GRP_COMPRESSION_RLE) != 0 ||
                                (entry.flags & GRP_COMPRESSION_LZ) != 0;

        // Normalize name to lowercase for lookup
        std::string lowerName = grpEntry.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        entryMap_[lowerName] = entries_.size();

        entries_.push_back(std::move(grpEntry));
    }

    isOpen_ = true;
    return true;
}

void GrpArchive::close() {
    entries_.clear();
    entryMap_.clear();
    filePath_.clear();
    isOpen_ = false;
}

bool GrpArchive::isOpen() const {
    return isOpen_;
}

std::vector<std::string> GrpArchive::listFiles() const {
    std::vector<std::string> names;
    names.reserve(entries_.size());
    for (const auto& entry : entries_) {
        names.push_back(entry.name);
    }
    return names;
}

const GrpEntry* GrpArchive::getEntry(const std::string& name) const {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto it = entryMap_.find(lowerName);
    if (it != entryMap_.end()) {
        return &entries_[it->second];
    }
    return nullptr;
}

std::vector<uint8_t> GrpArchive::extract(const std::string& name) {
    const GrpEntry* entry = getEntry(name);
    if (!entry) {
        lastError_ = "File not found: " + name;
        return {};
    }

    std::ifstream file(filePath_, std::ios::binary);
    if (!file) {
        lastError_ = "Failed to reopen archive";
        return {};
    }

    file.seekg(entry->offset);
    if (!file) {
        lastError_ = "Failed to seek to file offset";
        return {};
    }

    // Read compressed or raw data
    uint32_t readSize = entry->isCompressed ? entry->compressedSize : entry->size;
    if (readSize == 0) {
        readSize = entry->size;
    }

    std::vector<uint8_t> data(readSize);
    file.read(reinterpret_cast<char*>(data.data()), readSize);
    if (!file) {
        lastError_ = "Failed to read file data";
        return {};
    }

    // Decompress if needed
    if (entry->isCompressed) {
        if (entry->flags & GRP_COMPRESSION_RLE) {
            return decompressRLE(data, entry->size);
        } else if (entry->flags & GRP_COMPRESSION_LZ) {
            return decompressLZ(data, entry->size);
        }
    }

    return data;
}

std::vector<uint8_t> GrpArchive::decompressRLE(const std::vector<uint8_t>& compressed, uint32_t uncompressedSize) {
    std::vector<uint8_t> output;
    output.reserve(uncompressedSize);

    size_t i = 0;
    while (i < compressed.size() && output.size() < uncompressedSize) {
        uint8_t control = compressed[i++];

        if (control & 0x80) {
            // Run of same byte
            uint8_t count = (control & 0x7F) + 1;
            if (i >= compressed.size()) break;
            uint8_t value = compressed[i++];
            for (uint8_t j = 0; j < count && output.size() < uncompressedSize; ++j) {
                output.push_back(value);
            }
        } else {
            // Literal bytes
            uint8_t count = control + 1;
            for (uint8_t j = 0; j < count && i < compressed.size() && output.size() < uncompressedSize; ++j) {
                output.push_back(compressed[i++]);
            }
        }
    }

    return output;
}

std::vector<uint8_t> GrpArchive::decompressLZ(const std::vector<uint8_t>& compressed, uint32_t uncompressedSize) {
    std::vector<uint8_t> output;
    output.reserve(uncompressedSize);

    size_t i = 0;
    while (i < compressed.size() && output.size() < uncompressedSize) {
        uint8_t flags = compressed[i++];

        for (int bit = 0; bit < 8 && i < compressed.size() && output.size() < uncompressedSize; ++bit) {
            if (flags & (1 << bit)) {
                // Literal byte
                output.push_back(compressed[i++]);
            } else {
                // Back reference
                if (i + 1 >= compressed.size()) break;
                uint16_t ref = compressed[i] | (compressed[i + 1] << 8);
                i += 2;

                uint16_t offset = (ref >> 4) + 1;
                uint16_t length = (ref & 0x0F) + 3;

                for (uint16_t j = 0; j < length && output.size() < uncompressedSize; ++j) {
                    size_t srcIdx = output.size() - offset;
                    if (srcIdx < output.size()) {
                        output.push_back(output[srcIdx]);
                    } else {
                        output.push_back(0);
                    }
                }
            }
        }
    }

    return output;
}

std::unique_ptr<Sprite> GrpArchive::extractSprite(const std::string& name) {
    auto data = extract(name);
    if (data.empty()) {
        return nullptr;
    }
    return decodeSprite(data);
}

std::unique_ptr<Sprite> GrpArchive::decodeSprite(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(SpriteHeader)) {
        lastError_ = "Sprite data too small";
        return nullptr;
    }

    auto sprite = std::make_unique<Sprite>();

    // Parse header
    const SpriteHeader* header = reinterpret_cast<const SpriteHeader*>(data.data());
    sprite->width = header->width;
    sprite->height = header->height;
    sprite->hotspotX = header->hotspotX;
    sprite->hotspotY = header->hotspotY;

    if (sprite->width <= 0 || sprite->height <= 0 ||
        sprite->width > 4096 || sprite->height > 4096) {
        lastError_ = "Invalid sprite dimensions";
        return nullptr;
    }

    // Calculate expected pixel data size
    size_t pixelDataSize = static_cast<size_t>(sprite->width) * sprite->height;
    size_t headerEnd = sizeof(SpriteHeader);

    // Check for embedded palette
    if (header->paletteOffset > 0 && header->paletteOffset < data.size()) {
        size_t palOffset = header->paletteOffset;
        size_t palSize = std::min<size_t>(256 * 3, data.size() - palOffset);

        sprite->palette.resize(256);
        for (size_t i = 0; i < palSize / 3; ++i) {
            uint8_t r = data[palOffset + i * 3];
            uint8_t g = data[palOffset + i * 3 + 1];
            uint8_t b = data[palOffset + i * 3 + 2];
            // VGA palette values are 6-bit, scale to 8-bit
            sprite->palette[i] = (0xFF << 24) | ((r << 2) << 16) | ((g << 2) << 8) | (b << 2);
        }
        sprite->hasPalette = true;
    } else {
        sprite->palette = palette_;
        sprite->hasPalette = false;
    }

    // Extract pixel data
    if (headerEnd + pixelDataSize <= data.size()) {
        sprite->pixels.assign(data.begin() + headerEnd,
                              data.begin() + headerEnd + pixelDataSize);
    } else {
        // Might be RLE compressed sprite data
        sprite->pixels.resize(pixelDataSize);
        size_t srcIdx = headerEnd;
        size_t dstIdx = 0;

        while (srcIdx < data.size() && dstIdx < pixelDataSize) {
            uint8_t cmd = data[srcIdx++];
            if (cmd == 0) {
                // End of line or special
                if (srcIdx < data.size()) {
                    uint8_t count = data[srcIdx++];
                    if (count == 0) {
                        // End of bitmap
                        break;
                    }
                    // Skip pixels (transparent)
                    dstIdx += count;
                }
            } else if (cmd < 128) {
                // Literal run
                for (uint8_t i = 0; i < cmd && srcIdx < data.size() && dstIdx < pixelDataSize; ++i) {
                    sprite->pixels[dstIdx++] = data[srcIdx++];
                }
            } else {
                // Repeat run
                uint8_t count = cmd - 128;
                if (srcIdx < data.size()) {
                    uint8_t value = data[srcIdx++];
                    for (uint8_t i = 0; i < count && dstIdx < pixelDataSize; ++i) {
                        sprite->pixels[dstIdx++] = value;
                    }
                }
            }
        }
    }

    return sprite;
}

SDL_Surface* GrpArchive::extractAsSurface(const std::string& name) {
#ifdef SDL_h_
    auto sprite = extractSprite(name);
    if (!sprite) {
        return nullptr;
    }

    SDL_Surface* surface = SDL_CreateRGBSurface(0, sprite->width, sprite->height,
                                                 8, 0, 0, 0, 0);
    if (!surface) {
        lastError_ = "Failed to create SDL surface";
        return nullptr;
    }

    // Set palette
    SDL_Color colors[256];
    for (int i = 0; i < 256; ++i) {
        uint32_t c = sprite->palette[i];
        colors[i].r = (c >> 16) & 0xFF;
        colors[i].g = (c >> 8) & 0xFF;
        colors[i].b = c & 0xFF;
        colors[i].a = (c >> 24) & 0xFF;
    }
    SDL_SetPaletteColors(surface->format->palette, colors, 0, 256);

    // Copy pixels
    SDL_LockSurface(surface);
    uint8_t* dst = static_cast<uint8_t*>(surface->pixels);
    for (int y = 0; y < sprite->height; ++y) {
        std::memcpy(dst + y * surface->pitch,
                    sprite->pixels.data() + y * sprite->width,
                    sprite->width);
    }
    SDL_UnlockSurface(surface);

    return surface;
#else
    lastError_ = "SDL2 not available";
    return nullptr;
#endif
}

void GrpArchive::setPalette(const std::vector<uint32_t>& palette) {
    palette_ = palette;
    if (palette_.size() < 256) {
        palette_.resize(256, 0xFF000000);
    }
}

std::string GrpArchive::getLastError() const {
    return lastError_;
}

} // namespace opengg
