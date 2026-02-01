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
    uint16_t resType = RT_RCDATA;
    if (type == "bitmap") {
        resType = RT_BITMAP;
    } else if (type == "font") {
        resType = RT_FONT;
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

} // namespace opengg
