#include "smacker.h"
#include <cstring>
#include <algorithm>

#ifdef SDL_h_
#include <SDL.h>
#endif

namespace opengg {

// Smacker file header structure
#pragma pack(push, 1)
struct SmkHeader {
    char signature[4];      // "SMK2" or "SMK4"
    uint32_t width;
    uint32_t height;
    uint32_t frameCount;
    int32_t  frameRate;     // Negative = microseconds per frame
    uint32_t flags;
    uint32_t audioSize[7];
    uint32_t treesSize;
    uint32_t mMapSize;
    uint32_t mClrSize;
    uint32_t fullSize;
    uint32_t typeSize;
    uint32_t audioRate[7];
    uint32_t dummy;
};
#pragma pack(pop)

// Huffman tree node
struct SmackerPlayer::TreeNode {
    bool isLeaf;
    uint16_t value;
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;

    TreeNode() : isLeaf(false), value(0) {}
};

SmackerPlayer::SmackerPlayer() {
    palette_.resize(256 * 3, 0);
}

SmackerPlayer::~SmackerPlayer() {
    close();
}

bool SmackerPlayer::open(const std::string& path) {
    close();
    filePath_ = path;

    file_.open(path, std::ios::binary);
    if (!file_) {
        lastError_ = "Failed to open file: " + path;
        return false;
    }

    if (!readHeader()) {
        return false;
    }

    if (!readFrameSizes()) {
        return false;
    }

    // Allocate frame buffer
    frameBuffer_.resize(static_cast<size_t>(width_) * height_, 0);
    frameRGB_.resize(static_cast<size_t>(width_) * height_ * 3, 0);

    // Initialize default grayscale palette
    for (int i = 0; i < 256; ++i) {
        palette_[i * 3] = static_cast<uint8_t>(i);
        palette_[i * 3 + 1] = static_cast<uint8_t>(i);
        palette_[i * 3 + 2] = static_cast<uint8_t>(i);
    }

    isOpen_ = true;
    currentFrame_ = 0;
    return true;
}

void SmackerPlayer::close() {
    file_.close();
    frameSizes_.clear();
    frameTypes_.clear();
    frameBuffer_.clear();
    frameRGB_.clear();
    palette_.clear();
    palette_.resize(256 * 3, 0);

#ifdef SDL_h_
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
#endif

    isOpen_ = false;
    currentFrame_ = 0;
}

bool SmackerPlayer::isOpen() const {
    return isOpen_;
}

bool SmackerPlayer::readHeader() {
    SmkHeader header;
    file_.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file_) {
        lastError_ = "Failed to read SMK header";
        return false;
    }

    // Check signature
    if (std::memcmp(header.signature, "SMK2", 4) != 0 &&
        std::memcmp(header.signature, "SMK4", 4) != 0) {
        lastError_ = "Invalid SMK signature";
        return false;
    }

    width_ = header.width;
    height_ = header.height;
    frameCount_ = header.frameCount;
    treeSize_ = header.treesSize;

    // Handle frame rate (can be negative for microseconds)
    if (header.frameRate > 0) {
        frameRateNum_ = 1000;
        frameRateDen_ = header.frameRate;
    } else if (header.frameRate < 0) {
        frameRateNum_ = 1000000;
        frameRateDen_ = -header.frameRate;
    } else {
        frameRateNum_ = 10;
        frameRateDen_ = 1;
    }

    // Parse audio track info
    for (int i = 0; i < 7; ++i) {
        uint32_t rate = header.audioRate[i];
        audioInfo_[i].hasAudio = (header.audioSize[i] > 0);
        audioInfo_[i].sampleRate = rate & 0xFFFFFF;
        audioInfo_[i].channels = (rate & 0x10000000) ? 2 : 1;
        audioInfo_[i].bitsPerSample = (rate & 0x20000000) ? 16 : 8;
        audioInfo_[i].isCompressed = (rate & 0x80000000) != 0;
    }

    return true;
}

bool SmackerPlayer::readFrameSizes() {
    frameSizes_.resize(frameCount_);
    frameTypes_.resize(frameCount_);

    // Read frame sizes
    file_.read(reinterpret_cast<char*>(frameSizes_.data()), frameCount_ * sizeof(uint32_t));
    if (!file_) {
        lastError_ = "Failed to read frame sizes";
        return false;
    }

    // Read frame types
    file_.read(reinterpret_cast<char*>(frameTypes_.data()), frameCount_);
    if (!file_) {
        lastError_ = "Failed to read frame types";
        return false;
    }

    // Skip Huffman trees (we'll read them per-frame for simplicity)
    file_.seekg(treeSize_, std::ios::cur);
    if (!file_) {
        lastError_ = "Failed to skip trees";
        return false;
    }

    // Remember data start offset
    dataOffset_ = static_cast<uint32_t>(file_.tellg());

    return true;
}

float SmackerPlayer::getFrameRate() const {
    if (frameRateDen_ == 0) return 10.0f;
    return static_cast<float>(frameRateNum_) / frameRateDen_;
}

SmackerAudioInfo SmackerPlayer::getAudioInfo(int track) const {
    if (track < 0 || track >= 7) {
        return SmackerAudioInfo{};
    }
    return audioInfo_[track];
}

bool SmackerPlayer::nextFrame() {
    if (!isOpen_ || currentFrame_ >= frameCount_) {
        return false;
    }

    if (!decodeFrame(currentFrame_)) {
        return false;
    }

    currentFrame_++;
    return true;
}

bool SmackerPlayer::decodeFrame(uint32_t frameIndex) {
    // Calculate frame offset
    uint32_t offset = dataOffset_;
    for (uint32_t i = 0; i < frameIndex; ++i) {
        offset += frameSizes_[i] & 0xFFFFFFFC;  // Mask off flags
    }

    file_.seekg(offset);
    if (!file_) {
        lastError_ = "Failed to seek to frame";
        return false;
    }

    uint32_t frameSize = frameSizes_[frameIndex] & 0xFFFFFFFC;
    uint8_t frameType = frameTypes_[frameIndex];

    std::vector<uint8_t> frameData(frameSize);
    file_.read(reinterpret_cast<char*>(frameData.data()), frameSize);
    if (!file_) {
        lastError_ = "Failed to read frame data";
        return false;
    }

    const uint8_t* ptr = frameData.data();
    size_t remaining = frameSize;

    // Decode palette if present
    if (frameType & 0x01) {
        uint8_t palSize = *ptr++;
        remaining--;

        size_t palBytes = (palSize + 1) * 4;
        if (remaining >= palBytes) {
            decodePalette(ptr, palBytes);
            ptr += palBytes;
            remaining -= palBytes;
        }
    }

    // Decode audio tracks
    for (int track = 0; track < 7; ++track) {
        if (frameType & (0x02 << track)) {
            if (remaining < 4) break;
            uint32_t audioSize = *reinterpret_cast<const uint32_t*>(ptr);
            ptr += 4;
            remaining -= 4;

            if (remaining >= audioSize - 4) {
                decodeAudio(ptr, audioSize - 4, track);
                ptr += audioSize - 4;
                remaining -= audioSize - 4;
            }
        }
    }

    // Decode video
    if (remaining > 0) {
        decodeVideo(ptr, remaining);
    }

    // Convert indexed to RGB
    for (size_t i = 0; i < frameBuffer_.size(); ++i) {
        uint8_t idx = frameBuffer_[i];
        frameRGB_[i * 3] = palette_[idx * 3];
        frameRGB_[i * 3 + 1] = palette_[idx * 3 + 1];
        frameRGB_[i * 3 + 2] = palette_[idx * 3 + 2];
    }

    return true;
}

bool SmackerPlayer::decodePalette(const uint8_t* data, size_t size) {
    size_t i = 0;
    size_t palIdx = 0;

    while (i < size && palIdx < 256) {
        uint8_t cmd = data[i++];

        if (cmd & 0x80) {
            // Skip entries
            uint8_t skip = (cmd & 0x7F) + 1;
            palIdx += skip;
        } else if (cmd & 0x40) {
            // Copy from previous entry
            uint8_t count = (cmd & 0x3F) + 1;
            if (i >= size) break;
            uint8_t src = data[i++];

            for (uint8_t j = 0; j < count && palIdx < 256; ++j) {
                if (src < 256) {
                    palette_[palIdx * 3] = palette_[src * 3];
                    palette_[palIdx * 3 + 1] = palette_[src * 3 + 1];
                    palette_[palIdx * 3 + 2] = palette_[src * 3 + 2];
                }
                palIdx++;
                src++;
            }
        } else {
            // Set RGB values
            uint8_t count = (cmd & 0x3F) + 1;

            for (uint8_t j = 0; j < count && palIdx < 256 && i + 2 < size; ++j) {
                // SMK uses 6-bit color values
                palette_[palIdx * 3] = (data[i] & 0x3F) << 2;
                palette_[palIdx * 3 + 1] = (data[i + 1] & 0x3F) << 2;
                palette_[palIdx * 3 + 2] = (data[i + 2] & 0x3F) << 2;
                i += 3;
                palIdx++;
            }
        }
    }

    return true;
}

bool SmackerPlayer::decodeVideo(const uint8_t* data, size_t size) {
    // Simplified video decoding - just copy raw data for now
    // Full Smacker video uses block-based compression with Huffman trees

    // For now, treat it as raw indexed data if size matches
    size_t expectedSize = static_cast<size_t>(width_) * height_;
    if (size >= expectedSize) {
        std::memcpy(frameBuffer_.data(), data, expectedSize);
        return true;
    }

    // Otherwise, this is compressed data
    // Full implementation would need Smacker's block decoding algorithm
    // which involves motion blocks, solid blocks, and residual data

    // For prototype, just skip (frame remains as previous)
    return true;
}

bool SmackerPlayer::decodeAudio(const uint8_t* data, size_t size, int track) {
    if (track < 0 || track >= 7) return false;

    // Clear previous audio
    audioBuffers_[track].clear();

    if (!audioInfo_[track].hasAudio || size == 0) {
        return true;
    }

    if (!audioInfo_[track].isCompressed) {
        // Uncompressed audio - just copy
        size_t samples = size;
        if (audioInfo_[track].bitsPerSample == 16) {
            samples /= 2;
            audioBuffers_[track].resize(samples);
            std::memcpy(audioBuffers_[track].data(), data, samples * 2);
        } else {
            // 8-bit to 16-bit conversion
            audioBuffers_[track].resize(samples);
            for (size_t i = 0; i < samples; ++i) {
                audioBuffers_[track][i] = (static_cast<int16_t>(data[i]) - 128) * 256;
            }
        }
    } else {
        // DPCM compressed audio
        // Smacker uses a Huffman-coded DPCM scheme
        // For prototype, skip compressed audio
    }

    return true;
}

std::vector<int16_t> SmackerPlayer::getAudioSamples(int track) {
    if (track < 0 || track >= 7) {
        return {};
    }
    return audioBuffers_[track];
}

void SmackerPlayer::rewind() {
    currentFrame_ = 0;
    // Reset frame buffer
    std::fill(frameBuffer_.begin(), frameBuffer_.end(), 0);
}

bool SmackerPlayer::seekToFrame(uint32_t frame) {
    if (frame >= frameCount_) {
        return false;
    }

    // For accurate seeking, we need to decode from beginning
    // (due to delta compression)
    if (frame < currentFrame_) {
        rewind();
    }

    while (currentFrame_ < frame) {
        if (!nextFrame()) {
            return false;
        }
    }

    return true;
}

SDL_Texture* SmackerPlayer::getFrameTexture(SDL_Renderer* renderer) {
#ifdef SDL_h_
    if (!renderer) return nullptr;

    // Create or recreate texture if needed
    if (!texture_) {
        texture_ = SDL_CreateTexture(renderer,
                                     SDL_PIXELFORMAT_RGB24,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     width_, height_);
        if (!texture_) {
            lastError_ = "Failed to create SDL texture";
            return nullptr;
        }
    }

    // Update texture with current frame
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) == 0) {
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        for (int y = 0; y < height_; ++y) {
            std::memcpy(dst + y * pitch,
                        frameRGB_.data() + y * width_ * 3,
                        width_ * 3);
        }
        SDL_UnlockTexture(texture_);
    }

    return texture_;
#else
    lastError_ = "SDL2 not available";
    return nullptr;
#endif
}

} // namespace opengg
