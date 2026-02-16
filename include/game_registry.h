#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace opengg {

// Metadata for a single game
struct GameInfo {
    std::string id;           // Short ID: "on", "ssg", "sso", etc.
    std::string name;         // Full name: "Operation Neptune"
    std::string company;      // Publisher: "TLC"
    std::string sourcePath;   // Original source dir name
    int spriteCount = 0;
    int wavCount = 0;
    int midiCount = 0;
    int puzzleCount = 0;
    int videoCount = 0;
    std::string extractedPath; // Full path to extracted/<id>/
    bool available = false;    // True if directory exists on disk
};

// Registry of all known/discovered games
class GameRegistry {
public:
    GameRegistry();
    ~GameRegistry();

    // Discover games by scanning extractedBasePath for game directories
    // and parsing all_games_manifest.json
    bool discoverGames(const std::string& extractedBasePath);

    // Get list of all available (on-disk) games
    std::vector<GameInfo> getAvailableGames() const;

    // Get list of all known games (including unavailable)
    std::vector<GameInfo> getAllGames() const;

    // Get info for a specific game by ID
    const GameInfo* getGameInfo(const std::string& gameId) const;

    // Check if a game's extracted assets are available on disk
    bool isGameAvailable(const std::string& gameId) const;

    // Get the extracted base path
    std::string getExtractedBasePath() const { return extractedBasePath_; }

    // Get path to a game's sprites directory
    std::string getSpritePath(const std::string& gameId) const;

    // Get path to a game's WAV audio directory
    std::string getWavPath(const std::string& gameId) const;

    // Get path to a game's MIDI audio directory
    std::string getMidiPath(const std::string& gameId) const;

    // Get the number of available games
    int getAvailableCount() const;

private:
    bool parseManifest(const std::string& manifestPath);
    bool validateGameDirectory(const std::string& gameId);

    std::string extractedBasePath_;
    std::unordered_map<std::string, GameInfo> games_;

    // Ordered list of game IDs for consistent display order
    std::vector<std::string> gameOrder_;
};

} // namespace opengg
