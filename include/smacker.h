#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <fstream>

struct SDL_Texture;
struct SDL_Renderer;

namespace opengg {

// Smacker audio track info
struct SmackerAudioInfo {
    bool hasAudio;
    uint32_t sampleRate;
    uint8_t channels;
    uint8_t bitsPerSample;
    bool isCompressed;
};

// Smacker video decoder
// Decodes RAD Game Tools Smacker (.SMK) video files
class SmackerPlayer {
public:
    SmackerPlayer();
    ~SmackerPlayer();

    // Open a Smacker video file
    bool open(const std::string& path);

    // Close the video
    void close();

    // Check if video is open
    bool isOpen() const;

    // Get video dimensions
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    // Get frame count and current frame
    uint32_t getFrameCount() const { return frameCount_; }
    uint32_t getCurrentFrame() const { return currentFrame_; }

    // Get frame rate (frames per second)
    float getFrameRate() const;

    // Get audio info for a track (0-6)
    SmackerAudioInfo getAudioInfo(int track) const;

    // Decode next frame - returns false if no more frames
    bool nextFrame();

    // Get current frame pixels (RGB format)
    const std::vector<uint8_t>& getFrameRGB() const { return frameRGB_; }

    // Get current frame as SDL texture
    SDL_Texture* getFrameTexture(SDL_Renderer* renderer);

    // Get audio samples for current frame (16-bit signed)
    std::vector<int16_t> getAudioSamples(int track);

    // Reset to beginning
    void rewind();

    // Seek to specific frame
    bool seekToFrame(uint32_t frame);

    // Get last error
    std::string getLastError() const { return lastError_; }

private:
    struct TreeNode;

    bool readHeader();
    bool readFrameSizes();
    bool decodeFrame(uint32_t frameIndex);
    bool decodePalette(const uint8_t* data, size_t size);
    bool decodeVideo(const uint8_t* data, size_t size);
    bool decodeAudio(const uint8_t* data, size_t size, int track);

    // Huffman tree operations
    std::unique_ptr<TreeNode> buildTree(const uint8_t*& data, size_t& remaining);
    int decodeTreeValue(TreeNode* tree, const uint8_t*& data, int& bitPos);

    // Bit reading helpers
    uint32_t readBits(const uint8_t*& data, int& bitPos, int numBits);

    std::string filePath_;
    std::ifstream file_;

    // Video properties
    int width_ = 0;
    int height_ = 0;
    uint32_t frameCount_ = 0;
    uint32_t currentFrame_ = 0;
    int32_t frameRateNum_ = 0;
    int32_t frameRateDen_ = 1;

    // Frame data
    std::vector<uint32_t> frameSizes_;
    std::vector<uint8_t> frameTypes_;
    uint32_t treeSize_ = 0;

    // Decoded frame (indexed color)
    std::vector<uint8_t> frameBuffer_;

    // Current palette (256 RGB entries)
    std::vector<uint8_t> palette_;

    // RGB output
    std::vector<uint8_t> frameRGB_;

    // Audio tracks
    SmackerAudioInfo audioInfo_[7];
    std::vector<int16_t> audioBuffers_[7];

    // Internal state
    uint32_t dataOffset_ = 0;
    bool isOpen_ = false;
    std::string lastError_;

    // SDL texture (cached)
    SDL_Texture* texture_ = nullptr;
};

} // namespace opengg
