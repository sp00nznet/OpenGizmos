#include "asset_cache.h"
#include "ne_resource.h"
#include "grp_archive.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <regex>

#ifdef SDL_h_
#include <SDL.h>
#include <SDL_mixer.h>
#endif

namespace fs = std::filesystem;

namespace opengg {

// CRC32 lookup table
static uint32_t crc32Table[256];
static bool crc32TableInit = false;

static void initCRC32Table() {
    if (crc32TableInit) return;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        crc32Table[i] = crc;
    }
    crc32TableInit = true;
}

AssetCache::AssetCache() {
    initCRC32Table();
}

AssetCache::~AssetCache() {
    clearCache();
}

bool AssetCache::initialize(const std::string& gamePath, const std::string& cachePath) {
    gamePath_ = gamePath;
    cachePath_ = cachePath;

    // Create cache directory if it doesn't exist
    try {
        fs::create_directories(cachePath);
    } catch (const std::exception& e) {
        lastError_ = "Failed to create cache directory: " + std::string(e.what());
        return false;
    }

    // Load cache index
    loadCacheIndex();

    return true;
}

void AssetCache::setRenderer(SDL_Renderer* renderer) {
    renderer_ = renderer;
}

bool AssetCache::validateCache() {
    // Check if original game files have changed
    // For now, just check if cache index exists
    std::string indexPath = cachePath_ + "/cache_index.dat";
    return fs::exists(indexPath);
}

void AssetCache::clearCache() {
#ifdef SDL_h_
    // Free textures
    for (auto& pair : textures_) {
        if (pair.second.texture) {
            SDL_DestroyTexture(pair.second.texture);
        }
    }
    textures_.clear();

    // Free sounds
    for (auto& pair : sounds_) {
        if (pair.second) {
            Mix_FreeChunk(pair.second);
        }
    }
    sounds_.clear();

    // Free music
    for (auto& pair : music_) {
        if (pair.second) {
            Mix_FreeMusic(pair.second);
        }
    }
    music_.clear();
#endif

    sprites_.clear();
    neFiles_.clear();
    grpFiles_.clear();
    cacheIndex_.clear();
    stats_ = {};
}

SDL_Texture* AssetCache::getTexture(const std::string& assetId) {
#ifdef SDL_h_
    if (!renderer_) {
        lastError_ = "No renderer set";
        return nullptr;
    }

    // Check if already loaded
    auto it = textures_.find(assetId);
    if (it != textures_.end()) {
        it->second.refCount++;
        stats_.cacheHits++;
        return it->second.texture;
    }

    stats_.cacheMisses++;

    // Parse asset ID
    std::string source, type;
    int id;
    if (!parseAssetId(assetId, source, type, id)) {
        lastError_ = "Invalid asset ID: " + assetId;
        return nullptr;
    }

    // Try to load from disk cache first
    auto cached = loadFromCache(assetId);
    if (!cached.empty()) {
        // Create texture from cached BMP/PNG data
        SDL_RWops* rw = SDL_RWFromMem(cached.data(), static_cast<int>(cached.size()));
        if (rw) {
            SDL_Surface* surface = SDL_LoadBMP_RW(rw, 1);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
                if (texture) {
                    CachedTexture ct;
                    ct.texture = texture;
                    ct.width = surface->w;
                    ct.height = surface->h;
                    ct.refCount = 1;
                    textures_[assetId] = ct;
                    SDL_FreeSurface(surface);
                    stats_.texturesLoaded++;
                    return texture;
                }
                SDL_FreeSurface(surface);
            }
        }
    }

    // Load from original game files
    std::vector<uint8_t> data;
    if (type == "bitmap") {
        if (!loadFromNE(source, type, id, data)) {
            return nullptr;
        }
    } else if (type == "sprite") {
        if (!loadFromGrp(source, std::to_string(id), data)) {
            return nullptr;
        }
    }

    if (data.empty()) {
        lastError_ = "Failed to load asset: " + assetId;
        return nullptr;
    }

    // Parse as bitmap
    SDL_RWops* rw = SDL_RWFromMem(data.data(), static_cast<int>(data.size()));
    if (!rw) {
        lastError_ = "Failed to create RWops";
        return nullptr;
    }

    SDL_Surface* surface = SDL_LoadBMP_RW(rw, 1);
    if (!surface) {
        lastError_ = "Failed to load BMP: " + std::string(SDL_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (!texture) {
        lastError_ = "Failed to create texture: " + std::string(SDL_GetError());
        SDL_FreeSurface(surface);
        return nullptr;
    }

    // Cache in memory
    CachedTexture ct;
    ct.texture = texture;
    ct.width = surface->w;
    ct.height = surface->h;
    ct.refCount = 1;
    textures_[assetId] = ct;

    // Save to disk cache
    saveToCache(assetId, data);

    SDL_FreeSurface(surface);
    stats_.texturesLoaded++;

    return texture;
#else
    lastError_ = "SDL2 not available";
    return nullptr;
#endif
}

std::shared_ptr<Sprite> AssetCache::getSprite(const std::string& assetId) {
    // Check if already loaded
    auto it = sprites_.find(assetId);
    if (it != sprites_.end()) {
        stats_.cacheHits++;
        return it->second;
    }

    stats_.cacheMisses++;

    // Parse asset ID to get GRP file and sprite name
    std::string source, type;
    int id;
    if (!parseAssetId(assetId, source, type, id)) {
        // Try as direct GRP reference
        size_t colonPos = assetId.find(':');
        if (colonPos != std::string::npos) {
            source = assetId.substr(0, colonPos);
            std::string name = assetId.substr(colonPos + 1);

            // Get or open GRP archive
            auto grpIt = grpFiles_.find(source);
            if (grpIt == grpFiles_.end()) {
                auto grp = std::make_unique<GrpArchive>();
                std::string grpPath = gamePath_ + "/ASSETS/" + source + ".GRP";
                if (!grp->open(grpPath)) {
                    lastError_ = grp->getLastError();
                    return nullptr;
                }
                grpFiles_[source] = std::move(grp);
                grpIt = grpFiles_.find(source);
            }

            auto sprite = grpIt->second->extractSprite(name);
            if (sprite) {
                auto shared = std::make_shared<Sprite>(std::move(*sprite));
                sprites_[assetId] = shared;
                return shared;
            }
        }
        lastError_ = "Invalid sprite asset ID: " + assetId;
        return nullptr;
    }

    return nullptr;
}

Mix_Chunk* AssetCache::getSound(const std::string& assetId) {
#ifdef SDL_h_
    auto it = sounds_.find(assetId);
    if (it != sounds_.end()) {
        stats_.cacheHits++;
        return it->second;
    }

    stats_.cacheMisses++;

    // Load from cache or extract
    auto cached = loadFromCache(assetId);
    if (cached.empty()) {
        // Extract from original files
        std::string source, type;
        int id;
        if (!parseAssetId(assetId, source, type, id)) {
            lastError_ = "Invalid sound asset ID: " + assetId;
            return nullptr;
        }

        if (!loadFromNE(source, type, id, cached)) {
            return nullptr;
        }

        saveToCache(assetId, cached);
    }

    SDL_RWops* rw = SDL_RWFromMem(cached.data(), static_cast<int>(cached.size()));
    if (!rw) {
        return nullptr;
    }

    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1);
    if (chunk) {
        sounds_[assetId] = chunk;
        stats_.soundsLoaded++;
    }

    return chunk;
#else
    lastError_ = "SDL2 not available";
    return nullptr;
#endif
}

Mix_Music* AssetCache::getMusic(const std::string& assetId) {
#ifdef SDL_h_
    auto it = music_.find(assetId);
    if (it != music_.end()) {
        stats_.cacheHits++;
        return it->second;
    }

    stats_.cacheMisses++;

    // For MIDI, load directly from game path
    std::string midiPath = gamePath_ + "/SSGWINCD/MIDI/" + assetId + ".MID";
    if (!fs::exists(midiPath)) {
        midiPath = gamePath_ + "/MIDI/" + assetId + ".MID";
    }

    Mix_Music* mus = Mix_LoadMUS(midiPath.c_str());
    if (mus) {
        music_[assetId] = mus;
    } else {
        lastError_ = "Failed to load music: " + std::string(Mix_GetError());
    }

    return mus;
#else
    lastError_ = "SDL2 not available";
    return nullptr;
#endif
}

std::vector<uint8_t> AssetCache::getData(const std::string& assetId) {
    // Try cache first
    auto cached = loadFromCache(assetId);
    if (!cached.empty()) {
        stats_.cacheHits++;
        return cached;
    }

    stats_.cacheMisses++;

    std::string source, type;
    int id;
    if (!parseAssetId(assetId, source, type, id)) {
        lastError_ = "Invalid asset ID: " + assetId;
        return {};
    }

    std::vector<uint8_t> data;
    if (!loadFromNE(source, type, id, data)) {
        return {};
    }

    saveToCache(assetId, data);
    return data;
}

void AssetCache::releaseTexture(const std::string& assetId) {
    auto it = textures_.find(assetId);
    if (it != textures_.end()) {
        it->second.refCount--;
        // Don't actually free - keep in cache
    }
}

void AssetCache::preload(const std::string& pattern) {
    // Convert glob pattern to regex
    std::string regexStr = pattern;
    // Escape regex special chars except * and ?
    for (size_t i = 0; i < regexStr.size(); ++i) {
        if (regexStr[i] == '*') {
            regexStr.replace(i, 1, ".*");
            i++;
        } else if (regexStr[i] == '?') {
            regexStr[i] = '.';
        }
    }

    std::regex re(regexStr);

    // Scan cache index for matches
    for (const auto& pair : cacheIndex_) {
        if (std::regex_match(pair.first, re)) {
            // Trigger load based on type
            switch (pair.second.type) {
                case AssetType::Texture:
                    getTexture(pair.first);
                    break;
                case AssetType::Sound:
                    getSound(pair.first);
                    break;
                default:
                    getData(pair.first);
                    break;
            }
        }
    }
}

AssetCache::Stats AssetCache::getStats() const {
    stats_.texturesCached = textures_.size();
    return stats_;
}

std::string AssetCache::makeAssetId(const std::string& source, const std::string& type, int id) {
    return source + ":" + type + ":" + std::to_string(id);
}

bool AssetCache::parseAssetId(const std::string& assetId, std::string& source, std::string& type, int& id) {
    std::istringstream ss(assetId);
    std::string idStr;

    if (!std::getline(ss, source, ':')) return false;
    if (!std::getline(ss, type, ':')) return false;
    if (!std::getline(ss, idStr, ':')) return false;

    try {
        id = std::stoi(idStr);
    } catch (...) {
        return false;
    }

    return true;
}

bool AssetCache::loadFromNE(const std::string& source, const std::string& type, int id,
                            std::vector<uint8_t>& data) {
    // Get or open NE file
    auto it = neFiles_.find(source);
    if (it == neFiles_.end()) {
        auto ne = std::make_unique<NEResourceExtractor>();
        std::string nePath = gamePath_ + "/SSGWINCD/" + source + ".DAT";
        if (!fs::exists(nePath)) {
            nePath = gamePath_ + "/" + source + ".DAT";
        }
        if (!ne->open(nePath)) {
            lastError_ = ne->getLastError();
            return false;
        }
        neFiles_[source] = std::move(ne);
        it = neFiles_.find(source);
    }

    // Determine resource type
    uint16_t resType = NE_RT_RCDATA;
    if (type == "bitmap") {
        resType = NE_RT_BITMAP;
    } else if (type == "font") {
        resType = NE_RT_FONT;
    }

    data = it->second->extractResource(resType, static_cast<uint16_t>(id));
    if (data.empty()) {
        lastError_ = it->second->getLastError();
        return false;
    }

    // If bitmap, add BMP header
    if (type == "bitmap" && data.size() > 40) {
        std::vector<uint8_t> bmp;
        bmp.reserve(14 + data.size());

        // BMP file header
        uint32_t fileSize = 14 + static_cast<uint32_t>(data.size());
        uint32_t headerSize = *reinterpret_cast<uint32_t*>(data.data());
        uint16_t bitCount = *reinterpret_cast<uint16_t*>(data.data() + 14);
        uint32_t paletteSize = (bitCount <= 8) ? (1u << bitCount) * 4 : 0;
        uint32_t dataOffset = 14 + headerSize + paletteSize;

        bmp.push_back('B');
        bmp.push_back('M');
        bmp.insert(bmp.end(), reinterpret_cast<uint8_t*>(&fileSize),
                   reinterpret_cast<uint8_t*>(&fileSize) + 4);
        bmp.push_back(0); bmp.push_back(0);  // Reserved
        bmp.push_back(0); bmp.push_back(0);  // Reserved
        bmp.insert(bmp.end(), reinterpret_cast<uint8_t*>(&dataOffset),
                   reinterpret_cast<uint8_t*>(&dataOffset) + 4);
        bmp.insert(bmp.end(), data.begin(), data.end());

        data = std::move(bmp);
    }

    return true;
}

bool AssetCache::loadFromGrp(const std::string& source, const std::string& name,
                             std::vector<uint8_t>& data) {
    auto it = grpFiles_.find(source);
    if (it == grpFiles_.end()) {
        auto grp = std::make_unique<GrpArchive>();
        std::string grpPath = gamePath_ + "/ASSETS/" + source + ".GRP";
        if (!grp->open(grpPath)) {
            lastError_ = grp->getLastError();
            return false;
        }
        grpFiles_[source] = std::move(grp);
        it = grpFiles_.find(source);
    }

    data = it->second->extract(name);
    return !data.empty();
}

bool AssetCache::loadCacheIndex() {
    std::string indexPath = cachePath_ + "/cache_index.dat";
    std::ifstream file(indexPath, std::ios::binary);
    if (!file) {
        return false;
    }

    // Simple binary format: count, then entries
    uint32_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (uint32_t i = 0; i < count && file; ++i) {
        uint32_t idLen;
        file.read(reinterpret_cast<char*>(&idLen), sizeof(idLen));

        std::string id(idLen, '\0');
        file.read(id.data(), idLen);

        AssetMeta meta;
        meta.id = id;
        file.read(reinterpret_cast<char*>(&meta.type), sizeof(meta.type));
        file.read(reinterpret_cast<char*>(&meta.crc32), sizeof(meta.crc32));
        file.read(reinterpret_cast<char*>(&meta.timestamp), sizeof(meta.timestamp));

        cacheIndex_[id] = meta;
    }

    return true;
}

bool AssetCache::saveCacheIndex() {
    std::string indexPath = cachePath_ + "/cache_index.dat";
    std::ofstream file(indexPath, std::ios::binary);
    if (!file) {
        return false;
    }

    uint32_t count = static_cast<uint32_t>(cacheIndex_.size());
    file.write(reinterpret_cast<char*>(&count), sizeof(count));

    for (const auto& pair : cacheIndex_) {
        uint32_t idLen = static_cast<uint32_t>(pair.first.size());
        file.write(reinterpret_cast<const char*>(&idLen), sizeof(idLen));
        file.write(pair.first.c_str(), idLen);
        file.write(reinterpret_cast<const char*>(&pair.second.type), sizeof(pair.second.type));
        file.write(reinterpret_cast<const char*>(&pair.second.crc32), sizeof(pair.second.crc32));
        file.write(reinterpret_cast<const char*>(&pair.second.timestamp), sizeof(pair.second.timestamp));
    }

    return true;
}

std::string AssetCache::getCacheFilePath(const std::string& assetId) const {
    // Convert asset ID to safe filename
    std::string filename = assetId;
    std::replace(filename.begin(), filename.end(), ':', '_');
    std::replace(filename.begin(), filename.end(), '/', '_');
    std::replace(filename.begin(), filename.end(), '\\', '_');
    return cachePath_ + "/" + filename + ".cache";
}

bool AssetCache::saveToCache(const std::string& assetId, const std::vector<uint8_t>& data) {
    std::string path = getCacheFilePath(assetId);
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());

    // Update index
    AssetMeta meta;
    meta.id = assetId;
    meta.crc32 = calculateCRC32(data);
    meta.timestamp = static_cast<uint64_t>(std::time(nullptr));
    cacheIndex_[assetId] = meta;

    stats_.texturesCached++;
    return true;
}

std::vector<uint8_t> AssetCache::loadFromCache(const std::string& assetId) {
    std::string path = getCacheFilePath(assetId);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return {};
    }

    auto size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()), size);

    return data;
}

uint32_t AssetCache::calculateCRC32(const std::vector<uint8_t>& data) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc = crc32Table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

std::vector<std::pair<std::string, std::string>> AssetCache::listNEResources(const std::string& filename) {
    std::vector<std::pair<std::string, std::string>> result;

    // Build full path
    std::string fullPath = gamePath_ + "/SSGWINCD/" + filename;
    if (!fs::exists(fullPath)) {
        fullPath = gamePath_ + "/" + filename;
        if (!fs::exists(fullPath)) {
            return result;
        }
    }

    // Check if we have this file cached
    auto it = neFiles_.find(filename);
    if (it == neFiles_.end()) {
        auto extractor = std::make_unique<NEResourceExtractor>();
        if (!extractor->open(fullPath)) {
            return result;
        }
        neFiles_[filename] = std::move(extractor);
        it = neFiles_.find(filename);
    }

    // Get all resources
    auto resources = it->second->listResources();
    for (const auto& res : resources) {
        char info[128];
        snprintf(info, sizeof(info), "Type: %s  ID: %u  Size: %u bytes",
                 res.typeName.c_str(), res.id, res.size);
        result.emplace_back(res.name.empty() ?
            ("Resource " + std::to_string(res.id)) : res.name, info);
    }

    return result;
}

std::vector<Resource> AssetCache::getNEResourceList(const std::string& filename) {
    // Build full path - check multiple game directories
    std::vector<std::string> searchPaths = {
        gamePath_ + "/SSGWINCD/" + filename,      // Gizmos & Gadgets
        gamePath_ + "/ONWINCD/" + filename,       // Operation Neptune (alternate)
        gamePath_ + "/ONWINCD/INSTALL/" + filename, // Neptune install dir
        gamePath_ + "/sso_extract/" + filename,   // OutNumbered
        gamePath_ + "/ssr_extract/" + filename,   // Spellbound
        gamePath_ + "/tms_extract/" + filename,   // Treasure MathStorm
        gamePath_ + "/iso/SSGWINCD/" + filename,  // ISO mount paths
        gamePath_ + "/iso/INSTALL/" + filename,
        gamePath_ + "/" + filename                // Root fallback
    };

    std::string fullPath;
    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            fullPath = path;
            break;
        }
    }

    if (fullPath.empty()) {
        return {};
    }

    // Check if we have this file cached
    auto it = neFiles_.find(filename);
    if (it == neFiles_.end()) {
        auto extractor = std::make_unique<NEResourceExtractor>();
        if (!extractor->open(fullPath)) {
            return {};
        }
        neFiles_[filename] = std::move(extractor);
        it = neFiles_.find(filename);
    }

    return it->second->listResources();
}

std::vector<std::string> AssetCache::listGRPFiles(const std::string& filename) {
    std::vector<std::string> result;

    // Build full path - GRP files are in ASSETS folder
    std::string fullPath = gamePath_ + "/ASSETS/" + filename;
    if (!fs::exists(fullPath)) {
        fullPath = gamePath_ + "/" + filename;
        if (!fs::exists(fullPath)) {
            return result;
        }
    }

    // Check if we have this file cached
    auto it = grpFiles_.find(filename);
    if (it == grpFiles_.end()) {
        auto archive = std::make_unique<GrpArchive>();
        if (!archive->open(fullPath)) {
            return result;
        }
        grpFiles_[filename] = std::move(archive);
        it = grpFiles_.find(filename);
    }

    return it->second->listFiles();
}

std::vector<uint8_t> AssetCache::getRawResource(const std::string& filename, uint16_t type, uint16_t id) {
    // Build full path - check multiple game directories
    std::vector<std::string> searchPaths = {
        gamePath_ + "/SSGWINCD/" + filename,      // Gizmos & Gadgets
        gamePath_ + "/ONWINCD/" + filename,       // Operation Neptune (alternate)
        gamePath_ + "/ONWINCD/INSTALL/" + filename, // Neptune install dir
        gamePath_ + "/sso_extract/" + filename,   // OutNumbered
        gamePath_ + "/ssr_extract/" + filename,   // Spellbound
        gamePath_ + "/tms_extract/" + filename,   // Treasure MathStorm
        gamePath_ + "/iso/SSGWINCD/" + filename,  // ISO mount paths
        gamePath_ + "/iso/INSTALL/" + filename,
        gamePath_ + "/" + filename                // Root fallback
    };

    std::string fullPath;
    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            fullPath = path;
            break;
        }
    }

    if (fullPath.empty()) {
        return {};
    }

    // Check if we have this file cached
    auto it = neFiles_.find(filename);
    if (it == neFiles_.end()) {
        auto extractor = std::make_unique<NEResourceExtractor>();
        if (!extractor->open(fullPath)) {
            return {};
        }
        neFiles_[filename] = std::move(extractor);
        it = neFiles_.find(filename);
    }

    return it->second->extractResource(type, id);
}

SDL_Texture* AssetCache::createTextureFromBitmap(const std::vector<uint8_t>& bitmapData, int* outWidth, int* outHeight) {
#ifdef SDL_h_
    if (!renderer_ || bitmapData.size() < 40) {
        return nullptr;
    }

    // Parse BITMAPINFOHEADER
    const uint8_t* data = bitmapData.data();
    uint32_t headerSize = *reinterpret_cast<const uint32_t*>(data);
    int32_t width = *reinterpret_cast<const int32_t*>(data + 4);
    int32_t height = *reinterpret_cast<const int32_t*>(data + 8);
    uint16_t planes = *reinterpret_cast<const uint16_t*>(data + 12);
    uint16_t bitCount = *reinterpret_cast<const uint16_t*>(data + 14);
    uint32_t compression = *reinterpret_cast<const uint32_t*>(data + 16);

    // Handle negative height (top-down bitmap)
    bool topDown = height < 0;
    if (topDown) height = -height;

    if (outWidth) *outWidth = width;
    if (outHeight) *outHeight = height;

    // Only support uncompressed bitmaps for now
    if (compression != 0) {
        lastError_ = "Compressed bitmaps not supported";
        return nullptr;
    }

    // Calculate palette info
    uint32_t paletteSize = 0;
    uint32_t colorCount = 0;
    if (bitCount <= 8) {
        colorCount = *reinterpret_cast<const uint32_t*>(data + 32);
        if (colorCount == 0) {
            colorCount = 1u << bitCount;
        }
        paletteSize = colorCount * 4;
    }

    const uint8_t* palette = data + headerSize;
    const uint8_t* pixels = palette + paletteSize;

    // Calculate row stride (rows are padded to 4-byte boundaries)
    int rowStride = ((width * bitCount + 31) / 32) * 4;

    // Create SDL surface
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!surface) {
        lastError_ = "Failed to create surface";
        return nullptr;
    }

    // Convert pixel data
    uint32_t* destPixels = static_cast<uint32_t*>(surface->pixels);

    for (int y = 0; y < height; ++y) {
        int srcY = topDown ? y : (height - 1 - y);
        const uint8_t* srcRow = pixels + srcY * rowStride;

        for (int x = 0; x < width; ++x) {
            uint8_t r, g, b;

            if (bitCount == 8) {
                uint8_t index = srcRow[x];
                if (index < colorCount) {
                    b = palette[index * 4 + 0];
                    g = palette[index * 4 + 1];
                    r = palette[index * 4 + 2];
                } else {
                    r = g = b = 0;
                }
            } else if (bitCount == 4) {
                uint8_t byte = srcRow[x / 2];
                uint8_t index = (x % 2 == 0) ? (byte >> 4) : (byte & 0x0F);
                if (index < colorCount) {
                    b = palette[index * 4 + 0];
                    g = palette[index * 4 + 1];
                    r = palette[index * 4 + 2];
                } else {
                    r = g = b = 0;
                }
            } else if (bitCount == 1) {
                uint8_t byte = srcRow[x / 8];
                uint8_t bit = (byte >> (7 - (x % 8))) & 1;
                if (bit < colorCount) {
                    b = palette[bit * 4 + 0];
                    g = palette[bit * 4 + 1];
                    r = palette[bit * 4 + 2];
                } else {
                    r = g = b = 0;
                }
            } else if (bitCount == 24) {
                b = srcRow[x * 3 + 0];
                g = srcRow[x * 3 + 1];
                r = srcRow[x * 3 + 2];
            } else if (bitCount == 32) {
                b = srcRow[x * 4 + 0];
                g = srcRow[x * 4 + 1];
                r = srcRow[x * 4 + 2];
            } else {
                r = g = b = 128; // Unsupported format
            }

            destPixels[y * width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }

    // Create texture from surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);

    return texture;
#else
    return nullptr;
#endif
}

// === Extracted asset loading ===

SDL_Texture* AssetCache::loadExtractedTexture(const std::string& gameId, const std::string& spriteName,
                                                int* outWidth, int* outHeight) {
#ifdef SDL_h_
    if (!renderer_) {
        lastError_ = "No renderer set";
        return nullptr;
    }

    // Build cache key
    std::string cacheKey = "extracted:" + gameId + ":sprite:" + spriteName;

    // Check memory cache
    auto it = textures_.find(cacheKey);
    if (it != textures_.end()) {
        it->second.refCount++;
        stats_.cacheHits++;
        if (outWidth) *outWidth = it->second.width;
        if (outHeight) *outHeight = it->second.height;
        return it->second.texture;
    }

    stats_.cacheMisses++;

    // Build file path
    std::string basePath = extractedBasePath_.empty() ? gamePath_ : extractedBasePath_;
    std::string filePath = basePath + "/" + gameId + "/sprites/" + spriteName;

    // Try with .bmp extension if not already present
    if (!fs::exists(filePath)) {
        filePath += ".bmp";
    }
    if (!fs::exists(filePath)) {
        // Try without extension variations
        std::string dir = basePath + "/" + gameId + "/sprites/";
        if (fs::exists(dir)) {
            for (const auto& entry : fs::directory_iterator(dir)) {
                std::string fname = entry.path().stem().string();
                if (fname == spriteName) {
                    filePath = entry.path().string();
                    break;
                }
            }
        }
    }

    if (!fs::exists(filePath)) {
        lastError_ = "Extracted sprite not found: " + filePath;
        return nullptr;
    }

    // Load BMP directly via SDL
    SDL_Surface* surface = SDL_LoadBMP(filePath.c_str());
    if (!surface) {
        lastError_ = "Failed to load BMP: " + std::string(SDL_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (!texture) {
        lastError_ = "Failed to create texture: " + std::string(SDL_GetError());
        SDL_FreeSurface(surface);
        return nullptr;
    }

    // Cache
    CachedTexture ct;
    ct.texture = texture;
    ct.width = surface->w;
    ct.height = surface->h;
    ct.refCount = 1;
    textures_[cacheKey] = ct;

    if (outWidth) *outWidth = surface->w;
    if (outHeight) *outHeight = surface->h;

    SDL_FreeSurface(surface);
    stats_.texturesLoaded++;
    return texture;
#else
    return nullptr;
#endif
}

Mix_Chunk* AssetCache::loadExtractedSound(const std::string& gameId, const std::string& soundName) {
#ifdef SDL_h_
    std::string cacheKey = "extracted:" + gameId + ":wav:" + soundName;

    auto it = sounds_.find(cacheKey);
    if (it != sounds_.end()) {
        stats_.cacheHits++;
        return it->second;
    }

    stats_.cacheMisses++;

    std::string basePath = extractedBasePath_.empty() ? gamePath_ : extractedBasePath_;
    std::string filePath = basePath + "/" + gameId + "/audio/wav/" + soundName;

    if (!fs::exists(filePath)) {
        filePath += ".wav";
    }
    if (!fs::exists(filePath)) {
        lastError_ = "Extracted sound not found: " + filePath;
        return nullptr;
    }

    Mix_Chunk* chunk = Mix_LoadWAV(filePath.c_str());
    if (chunk) {
        sounds_[cacheKey] = chunk;
        stats_.soundsLoaded++;
    } else {
        lastError_ = "Failed to load WAV: " + std::string(Mix_GetError());
    }
    return chunk;
#else
    return nullptr;
#endif
}

Mix_Music* AssetCache::loadExtractedMusic(const std::string& gameId, const std::string& midiName) {
#ifdef SDL_h_
    std::string cacheKey = "extracted:" + gameId + ":midi:" + midiName;

    auto it = music_.find(cacheKey);
    if (it != music_.end()) {
        stats_.cacheHits++;
        return it->second;
    }

    stats_.cacheMisses++;

    std::string basePath = extractedBasePath_.empty() ? gamePath_ : extractedBasePath_;
    std::string filePath = basePath + "/" + gameId + "/audio/midi/" + midiName;

    if (!fs::exists(filePath)) {
        filePath += ".mid";
    }
    if (!fs::exists(filePath)) {
        lastError_ = "Extracted MIDI not found: " + filePath;
        return nullptr;
    }

    Mix_Music* mus = Mix_LoadMUS(filePath.c_str());
    if (mus) {
        music_[cacheKey] = mus;
    } else {
        lastError_ = "Failed to load MIDI: " + std::string(Mix_GetError());
    }
    return mus;
#else
    return nullptr;
#endif
}

std::vector<std::string> AssetCache::listExtractedAssets(const std::string& gameId, const std::string& category) {
    std::vector<std::string> result;

    std::string basePath = extractedBasePath_.empty() ? gamePath_ : extractedBasePath_;
    std::string dirPath;

    if (category == "sprites") {
        dirPath = basePath + "/" + gameId + "/sprites";
    } else if (category == "wav") {
        dirPath = basePath + "/" + gameId + "/audio/wav";
    } else if (category == "midi") {
        dirPath = basePath + "/" + gameId + "/audio/midi";
    } else if (category == "puzzles") {
        dirPath = basePath + "/" + gameId + "/puzzles";
    } else if (category == "rooms") {
        dirPath = basePath + "/" + gameId + "/rooms";
    } else if (category == "video") {
        dirPath = basePath + "/" + gameId + "/video";
    } else {
        dirPath = basePath + "/" + gameId + "/" + category;
    }

    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
        return result;
    }

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            result.push_back(entry.path().filename().string());
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

} // namespace opengg
