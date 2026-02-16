#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <functional>

struct SDL_Texture;
struct SDL_Renderer;
struct Mix_Chunk;
struct _Mix_Music;
typedef _Mix_Music Mix_Music;

namespace opengg {

// Forward declarations
class NEResourceExtractor;
class GrpArchive;
struct Sprite;
struct Resource;

// Asset types
enum class AssetType {
    Texture,
    Sprite,
    Sound,
    Music,
    Data
};

// Asset metadata stored in cache index
struct AssetMeta {
    std::string id;
    AssetType type;
    std::string sourcePath;
    uint32_t sourceOffset;
    uint32_t crc32;
    uint64_t timestamp;
    uint32_t width;
    uint32_t height;
};

// Cached texture with reference counting
struct CachedTexture {
    SDL_Texture* texture;
    int width;
    int height;
    int refCount;

    CachedTexture() : texture(nullptr), width(0), height(0), refCount(0) {}
};

// Asset Cache System
// Extracts assets from original game files and caches converted versions
class AssetCache {
public:
    AssetCache();
    ~AssetCache();

    // Initialize with game path and cache directory
    bool initialize(const std::string& gamePath, const std::string& cachePath);

    // Set SDL renderer for texture creation
    void setRenderer(SDL_Renderer* renderer);

    // Check if cache is valid (matches original files)
    bool validateCache();

    // Clear all cached data
    void clearCache();

    // Get texture by asset ID (e.g., "gizmo256:bitmap:100")
    SDL_Texture* getTexture(const std::string& assetId);

    // Get sprite data by ID
    std::shared_ptr<Sprite> getSprite(const std::string& assetId);

    // Get sound effect
    Mix_Chunk* getSound(const std::string& assetId);

    // Get music
    Mix_Music* getMusic(const std::string& assetId);

    // Get raw data
    std::vector<uint8_t> getData(const std::string& assetId);

    // Release a texture (decrement ref count)
    void releaseTexture(const std::string& assetId);

    // Preload assets matching a pattern
    void preload(const std::string& pattern);

    // Get cache statistics
    struct Stats {
        size_t texturesLoaded;
        size_t texturesCached;
        size_t soundsLoaded;
        size_t cacheHits;
        size_t cacheMisses;
        size_t memoryUsed;
    };
    Stats getStats() const;

    // Get last error
    std::string getLastError() const { return lastError_; }

    // Get game path
    std::string getGamePath() const { return gamePath_; }

    // List resources in an NE file (for asset viewer)
    // Returns pairs of (display_name, info_string)
    std::vector<std::pair<std::string, std::string>> listNEResources(const std::string& filename);

    // Get raw Resource objects from NE file (for asset viewer preview)
    std::vector<Resource> getNEResourceList(const std::string& filename);

    // List files in a GRP archive (for asset viewer)
    std::vector<std::string> listGRPFiles(const std::string& filename);

    // Get raw resource data for preview
    std::vector<uint8_t> getRawResource(const std::string& filename, uint16_t type, uint16_t id);

    // Create texture from bitmap resource data (for preview)
    SDL_Texture* createTextureFromBitmap(const std::vector<uint8_t>& bitmapData, int* outWidth = nullptr, int* outHeight = nullptr);

    // === Extracted asset loading (pre-extracted BMP/WAV/MIDI files) ===

    // Set the base path where extracted game directories live
    void setExtractedBasePath(const std::string& path) { extractedBasePath_ = path; }
    std::string getExtractedBasePath() const { return extractedBasePath_; }

    // Load a texture from extracted/<gameId>/sprites/<spriteName>.bmp
    SDL_Texture* loadExtractedTexture(const std::string& gameId, const std::string& spriteName,
                                       int* outWidth = nullptr, int* outHeight = nullptr);

    // Load a sound from extracted/<gameId>/audio/wav/<soundName>.wav
    Mix_Chunk* loadExtractedSound(const std::string& gameId, const std::string& soundName);

    // Load music from extracted/<gameId>/audio/midi/<midiName>.mid
    Mix_Music* loadExtractedMusic(const std::string& gameId, const std::string& midiName);

    // List files in an extracted game's asset directory
    // category: "sprites", "wav", "midi", "puzzles", "rooms", "video"
    std::vector<std::string> listExtractedAssets(const std::string& gameId, const std::string& category);

    // Asset ID helpers
    static std::string makeAssetId(const std::string& source, const std::string& type, int id);
    static bool parseAssetId(const std::string& assetId, std::string& source, std::string& type, int& id);

private:
    // Internal asset loading
    bool loadFromNE(const std::string& source, const std::string& type, int id,
                    std::vector<uint8_t>& data);
    bool loadFromGrp(const std::string& source, const std::string& name,
                     std::vector<uint8_t>& data);

    // Cache file operations
    bool loadCacheIndex();
    bool saveCacheIndex();
    std::string getCacheFilePath(const std::string& assetId) const;
    bool saveToCache(const std::string& assetId, const std::vector<uint8_t>& data);
    std::vector<uint8_t> loadFromCache(const std::string& assetId);

    // CRC32 calculation
    uint32_t calculateCRC32(const std::vector<uint8_t>& data);

    std::string gamePath_;
    std::string cachePath_;
    SDL_Renderer* renderer_ = nullptr;

    // Loaded assets
    std::unordered_map<std::string, CachedTexture> textures_;
    std::unordered_map<std::string, std::shared_ptr<Sprite>> sprites_;
    std::unordered_map<std::string, Mix_Chunk*> sounds_;
    std::unordered_map<std::string, Mix_Music*> music_;

    // Source file handles (lazy loaded)
    std::unordered_map<std::string, std::unique_ptr<NEResourceExtractor>> neFiles_;
    std::unordered_map<std::string, std::unique_ptr<GrpArchive>> grpFiles_;

    // Cache index
    std::unordered_map<std::string, AssetMeta> cacheIndex_;

    // Statistics
    mutable Stats stats_ = {};

    std::string lastError_;

    // Extracted assets base path (e.g., "C:/ggng/extracted")
    std::string extractedBasePath_;
};

} // namespace opengg
