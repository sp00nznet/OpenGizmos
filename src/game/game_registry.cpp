#include "game_registry.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef SDL_h_
#include <SDL.h>
#else
#include <cstdio>
#define SDL_Log(...) do { printf(__VA_ARGS__); printf("\n"); } while(0)
#endif

namespace fs = std::filesystem;

namespace opengg {

GameRegistry::GameRegistry() {
    // Always populate with all known TLC games so the UI always shows the full list
    populateKnownGames();
}

GameRegistry::~GameRegistry() = default;

void GameRegistry::populateKnownGames() {
    struct KnownGame {
        const char* id;
        const char* name;
    };
    static const KnownGame knownGames[] = {
        {"ssg", "Super Solvers: Gizmos & Gadgets"},
        {"on",  "Operation Neptune"},
        {"tms", "Treasure MathStorm!"},
        {"tcv", "Treasure Cove!"},
        {"ssr", "Super Solvers: Spellbound!"},
        {"sso", "Super Solvers: OutNumbered!"},
        {"tmt", "Treasure Mountain!"},
        {"ssb", "Super Solvers: Spellbound Wizards"},
    };

    for (const auto& kg : knownGames) {
        if (games_.find(kg.id) == games_.end()) {
            GameInfo info;
            info.id = kg.id;
            info.name = kg.name;
            info.company = "TLC";
            info.available = false;
            games_[info.id] = info;
            gameOrder_.push_back(info.id);
        }
    }
}

bool GameRegistry::discoverGames(const std::string& extractedBasePath) {
    extractedBasePath_ = extractedBasePath;

    // Reset availability and counts (keep the known games list)
    for (auto& pair : games_) {
        pair.second.available = false;
    }

    // Try to parse the manifest file to get real asset counts
    std::string manifestPath = extractedBasePath + "/all_games_manifest.json";
    if (fs::exists(manifestPath)) {
        if (!parseManifest(manifestPath)) {
            SDL_Log("GameRegistry: Failed to parse manifest at %s", manifestPath.c_str());
        }
    } else {
        SDL_Log("GameRegistry: No manifest found at %s", manifestPath.c_str());
    }

    // Validate which game directories actually exist
    for (auto& pair : games_) {
        pair.second.available = validateGameDirectory(pair.first);
        if (pair.second.available) {
            SDL_Log("GameRegistry: Found game '%s' (%s)",
                    pair.second.name.c_str(), pair.second.id.c_str());
        }
    }

    int available = getAvailableCount();
    SDL_Log("GameRegistry: Discovered %d games (%d available)",
            static_cast<int>(games_.size()), available);

    return available > 0;
}

bool GameRegistry::parseManifest(const std::string& manifestPath) {
    std::ifstream file(manifestPath);
    if (!file) return false;

    // Read entire file
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    // Simple JSON parser for our known format - no external library needed.
    // The manifest is a flat object of objects with known string/int fields.
    // We parse by finding each game_id key block.

    // Find all game blocks by searching for "game_id"
    size_t pos = 0;
    while (pos < content.size()) {
        // Find next top-level key (a game ID like "on", "ssg", etc.)
        size_t keyStart = content.find("\"game_id\"", pos);
        if (keyStart == std::string::npos) break;

        GameInfo info;

        // Helper lambda to extract a string value for a key
        auto extractString = [&](const std::string& key) -> std::string {
            // Search near keyStart for the key
            size_t blockStart = content.rfind('{', keyStart);
            size_t blockEnd = content.find('}', keyStart);
            if (blockStart == std::string::npos || blockEnd == std::string::npos) return "";

            std::string block = content.substr(blockStart, blockEnd - blockStart + 1);
            size_t kpos = block.find("\"" + key + "\"");
            if (kpos == std::string::npos) return "";

            size_t colon = block.find(':', kpos + key.size() + 2);
            if (colon == std::string::npos) return "";

            size_t qstart = block.find('"', colon + 1);
            if (qstart == std::string::npos) return "";
            size_t qend = block.find('"', qstart + 1);
            if (qend == std::string::npos) return "";

            return block.substr(qstart + 1, qend - qstart - 1);
        };

        // Helper lambda to extract an int value for a key
        auto extractInt = [&](const std::string& key) -> int {
            size_t blockStart = content.rfind('{', keyStart);
            size_t blockEnd = content.find('}', keyStart);
            if (blockStart == std::string::npos || blockEnd == std::string::npos) return 0;

            std::string block = content.substr(blockStart, blockEnd - blockStart + 1);
            size_t kpos = block.find("\"" + key + "\"");
            if (kpos == std::string::npos) return 0;

            size_t colon = block.find(':', kpos + key.size() + 2);
            if (colon == std::string::npos) return 0;

            // Skip whitespace after colon
            size_t numStart = colon + 1;
            while (numStart < block.size() && (block[numStart] == ' ' || block[numStart] == '\t')) {
                numStart++;
            }

            try {
                return std::stoi(block.substr(numStart));
            } catch (...) {
                return 0;
            }
        };

        info.id = extractString("game_id");
        info.name = extractString("game_name");
        info.company = extractString("company");
        info.sourcePath = extractString("source_path");
        info.spriteCount = extractInt("sprites");
        info.wavCount = extractInt("wav_files");
        info.midiCount = extractInt("midi_files");
        info.puzzleCount = extractInt("puzzle_resources");
        info.videoCount = extractInt("video_files");

        if (!info.id.empty()) {
            info.extractedPath = extractedBasePath_ + "/" + info.id;
            // Merge into existing entry (preserves known game list order)
            auto it = games_.find(info.id);
            if (it != games_.end()) {
                // Update existing entry with manifest data
                it->second.name = info.name;
                it->second.company = info.company;
                it->second.sourcePath = info.sourcePath;
                it->second.spriteCount = info.spriteCount;
                it->second.wavCount = info.wavCount;
                it->second.midiCount = info.midiCount;
                it->second.puzzleCount = info.puzzleCount;
                it->second.videoCount = info.videoCount;
                it->second.extractedPath = info.extractedPath;
            } else {
                games_[info.id] = info;
                gameOrder_.push_back(info.id);
            }
        }

        pos = keyStart + 1;
    }

    return !games_.empty();
}

bool GameRegistry::validateGameDirectory(const std::string& gameId) {
    auto it = games_.find(gameId);
    if (it == games_.end()) return false;

    std::string gamePath = extractedBasePath_ + "/" + gameId;

    // Check if the directory exists
    if (!fs::exists(gamePath) || !fs::is_directory(gamePath)) {
        return false;
    }

    // Check for at least a sprites subdirectory
    if (fs::exists(gamePath + "/sprites") ||
        fs::exists(gamePath + "/audio") ||
        fs::exists(gamePath + "/manifest.json")) {
        it->second.extractedPath = gamePath;
        return true;
    }

    // Directory exists but may be empty or incomplete
    it->second.extractedPath = gamePath;
    return true;
}

std::vector<GameInfo> GameRegistry::getAvailableGames() const {
    std::vector<GameInfo> result;
    for (const auto& id : gameOrder_) {
        auto it = games_.find(id);
        if (it != games_.end() && it->second.available) {
            result.push_back(it->second);
        }
    }
    return result;
}

std::vector<GameInfo> GameRegistry::getAllGames() const {
    std::vector<GameInfo> result;
    for (const auto& id : gameOrder_) {
        auto it = games_.find(id);
        if (it != games_.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

const GameInfo* GameRegistry::getGameInfo(const std::string& gameId) const {
    auto it = games_.find(gameId);
    if (it != games_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GameRegistry::isGameAvailable(const std::string& gameId) const {
    auto it = games_.find(gameId);
    return it != games_.end() && it->second.available;
}

std::string GameRegistry::getSpritePath(const std::string& gameId) const {
    auto it = games_.find(gameId);
    if (it == games_.end()) return "";
    return it->second.extractedPath + "/sprites";
}

std::string GameRegistry::getWavPath(const std::string& gameId) const {
    auto it = games_.find(gameId);
    if (it == games_.end()) return "";
    return it->second.extractedPath + "/audio/wav";
}

std::string GameRegistry::getMidiPath(const std::string& gameId) const {
    auto it = games_.find(gameId);
    if (it == games_.end()) return "";
    return it->second.extractedPath + "/audio/midi";
}

int GameRegistry::getAvailableCount() const {
    int count = 0;
    for (const auto& pair : games_) {
        if (pair.second.available) count++;
    }
    return count;
}

} // namespace opengg
