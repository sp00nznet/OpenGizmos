#include "formats/sprite_format.h"
#include <fstream>
#include <cstring>

namespace opengg {

bool SpriteDecoder::loadPalette(const std::string& gamePath) {
    // Try to load from AUTO256.BMP
    std::string palettePath = gamePath + "/INSTALL/AUTO256.BMP";
    std::ifstream file(palettePath, std::ios::binary);

    if (!file) {
        // Try alternate path
        palettePath = gamePath + "/../INSTALL/AUTO256.BMP";
        file.open(palettePath, std::ios::binary);
    }

    if (!file) {
        return false;
    }

    // BMP palette starts at offset 54 (14-byte file header + 40-byte info header)
    file.seekg(54);
    file.read(reinterpret_cast<char*>(palette_), 1024);

    paletteLoaded_ = file.good();
    return paletteLoaded_;
}

std::vector<uint8_t> SpriteDecoder::getRawGraphics(const std::string& datFile,
                                                    uint32_t offset,
                                                    uint32_t width,
                                                    uint32_t height) {
    std::vector<uint8_t> data;

    std::ifstream file(datFile, std::ios::binary);
    if (!file) {
        return data;
    }

    size_t size = width * height;
    data.resize(size);

    file.seekg(offset);
    file.read(reinterpret_cast<char*>(data.data()), size);

    return data;
}

std::vector<uint32_t> SpriteDecoder::convertToRGBA(const std::vector<uint8_t>& indexed) {
    std::vector<uint32_t> rgba;
    rgba.reserve(indexed.size());

    for (uint8_t idx : indexed) {
        // Palette is in BGRA format, output as RGBA
        uint8_t b = palette_[idx][0];
        uint8_t g = palette_[idx][1];
        uint8_t r = palette_[idx][2];
        // Alpha is always 255 (fully opaque)
        // Pack as 0xAARRGGBB
        uint32_t pixel = 0xFF000000 | (r << 16) | (g << 8) | b;
        rgba.push_back(pixel);
    }

    return rgba;
}

void SpriteDecoder::getPaletteColor(uint8_t index, uint8_t& r, uint8_t& g, uint8_t& b) const {
    if (!paletteLoaded_) {
        r = g = b = index; // Grayscale fallback
        return;
    }

    b = palette_[index][0];
    g = palette_[index][1];
    r = palette_[index][2];
}

std::vector<uint8_t> SpriteDecoder::decompressRLE(const uint8_t* data, size_t dataSize,
                                                   size_t expectedPixels) {
    std::vector<uint8_t> pixels;
    pixels.reserve(expectedPixels);

    size_t i = 0;
    while (pixels.size() < expectedPixels && i < dataSize) {
        if (data[i] == 0xFF && i + 2 < dataSize) {
            // RLE: FF <byte> <count> - repeat byte count times
            uint8_t byte = data[i + 1];
            uint8_t count = data[i + 2];
            if (count == 0) count = 1;  // Safety

            for (int j = 0; j < count && pixels.size() < expectedPixels; ++j) {
                pixels.push_back(byte);
            }
            i += 3;
        } else {
            // Literal byte
            pixels.push_back(data[i]);
            i++;
        }
    }

    // Pad with zeros if we didn't get enough pixels
    while (pixels.size() < expectedPixels) {
        pixels.push_back(0);
    }

    return pixels;
}

} // namespace opengg
