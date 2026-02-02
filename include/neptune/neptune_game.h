#pragma once

#include "game_loop.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace opengg {

class Renderer;
class AudioSystem;
class InputSystem;
class AssetCache;

// Neptune game section types
enum class NeptuneSection {
    MainMenu,
    Submarine,      // Main submarine navigation
    Labyrinth,      // Maze exploration
    SortingPuzzle,  // Categorization puzzle
    ReaderPuzzle,   // Reading comprehension
    MathPuzzle,     // Math problem solving
    Victory,
    GameOver
};

// Submarine state
struct SubmarineState {
    float x = 320.0f;
    float y = 240.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float rotation = 0.0f;      // Facing direction in degrees

    float oxygen = 100.0f;      // 0-100
    float fuel = 100.0f;        // 0-100
    int depth = 0;              // Current depth level

    int canistersCollected = 0;
    int totalCanisters = 10;

    bool lightsOn = true;
    bool engineOn = true;

    // Movement parameters
    static constexpr float MAX_SPEED = 150.0f;
    static constexpr float ACCELERATION = 100.0f;
    static constexpr float DRAG = 0.95f;
    static constexpr float OXYGEN_DRAIN_RATE = 0.5f;  // Per second
    static constexpr float FUEL_DRAIN_RATE = 0.2f;    // Per second when moving
};

// Canister collectible
struct Canister {
    float x, y;
    int roomId;
    bool collected = false;
    int puzzleRequired = -1;  // Puzzle ID to solve to access, -1 = none
    SDL_Texture* texture = nullptr;
};

// Oxygen/Fuel station
struct ResourceStation {
    float x, y;
    int roomId;
    enum Type { Oxygen, Fuel, Both } type;
    bool available = true;
    float refillRate = 25.0f;  // Per second
};

// Hazard in the environment
struct Hazard {
    float x, y;
    float width, height;
    int roomId;
    enum Type { Rock, Eel, Current, Pressure } type;
    float damage = 10.0f;
    bool active = true;
};

// Neptune room/area
struct NeptuneRoom {
    int id;
    std::string name;
    int backgroundId;  // Resource ID for background

    std::vector<Canister> canisters;
    std::vector<ResourceStation> stations;
    std::vector<Hazard> hazards;

    // Room connections (exit points)
    struct Exit {
        float x, y;
        float width, height;
        int targetRoom;
        float targetX, targetY;
    };
    std::vector<Exit> exits;

    // Collision bounds
    std::vector<SDL_Rect> walls;
};

// Sorting puzzle item
struct SortingItem {
    int id;
    std::string name;
    int category;       // Target category (0, 1, 2...)
    int spriteId;
    float x, y;
    bool sorted = false;
};

// Reading puzzle question
struct ReaderQuestion {
    std::string passage;
    std::string question;
    std::vector<std::string> choices;
    int correctAnswer;
    int difficulty;
};

// Math puzzle problem
struct MathProblem {
    std::string problem;
    int answer;
    std::vector<int> choices;
    int correctChoice;
    enum Type { Addition, Subtraction, Multiplication, Division, WordProblem } type;
    int difficulty;
};

// ============================================
// Neptune Main Game State
// ============================================
class NeptuneGameState : public GameState {
public:
    NeptuneGameState(Game* game);
    ~NeptuneGameState() override;

    void enter() override;
    void exit() override;
    void update(float dt) override;
    void render() override;
    void handleInput() override;

    // Section management
    void changeSection(NeptuneSection section);
    NeptuneSection getCurrentSection() const { return currentSection_; }

    // Game state access
    SubmarineState& getSubmarine() { return submarine_; }
    const SubmarineState& getSubmarine() const { return submarine_; }

    // Room management
    void loadRoom(int roomId);
    NeptuneRoom* getCurrentRoom() { return currentRoom_; }

    // Puzzle triggers
    void startSortingPuzzle(int puzzleId);
    void startReaderPuzzle(int puzzleId);
    void startMathPuzzle(int puzzleId);
    void onPuzzleComplete(bool success);

private:
    void loadAssets();
    void loadRooms();

    void updateSubmarine(float dt);
    void updateHazards(float dt);
    void checkCollisions();
    void checkCanisterCollection();
    void checkStationRefill(float dt);
    void checkRoomTransition();

    void renderBackground();
    void renderSubmarine();
    void renderHUD();
    void renderCanisters();
    void renderHazards();

    Game* game_;
    NeptuneSection currentSection_ = NeptuneSection::MainMenu;

    SubmarineState submarine_;
    std::unordered_map<int, NeptuneRoom> rooms_;
    NeptuneRoom* currentRoom_ = nullptr;

    // Textures
    SDL_Texture* submarineTexture_ = nullptr;
    SDL_Texture* canisterTexture_ = nullptr;
    SDL_Texture* oxygenStationTexture_ = nullptr;
    SDL_Texture* fuelStationTexture_ = nullptr;
    SDL_Texture* hudTexture_ = nullptr;

    // Puzzle state
    int currentPuzzleId_ = -1;
};

// ============================================
// Labyrinth Game State
// ============================================
class LabyrinthGameState : public GameState {
public:
    LabyrinthGameState(Game* game, int levelId);
    ~LabyrinthGameState() override;

    void enter() override;
    void exit() override;
    void update(float dt) override;
    void render() override;
    void handleInput() override;

private:
    void loadLabyrinth(int levelId);
    void updatePlayer(float dt);
    void checkCollisions();
    void checkGoal();

    Game* game_;
    int levelId_;

    // Player position in labyrinth
    float playerX_ = 0, playerY_ = 0;
    float playerVelX_ = 0, playerVelY_ = 0;

    // Labyrinth data
    SDL_Texture* backgroundTexture_ = nullptr;
    std::vector<SDL_Rect> walls_;
    SDL_Rect goal_;

    // Sprites for labyrinth tiles
    std::vector<SDL_Texture*> tileSprites_;
};

// ============================================
// Sorting Puzzle State
// ============================================
class SortingPuzzleState : public GameState {
public:
    SortingPuzzleState(Game* game, int puzzleId);
    ~SortingPuzzleState() override;

    void enter() override;
    void exit() override;
    void update(float dt) override;
    void render() override;
    void handleInput() override;

    bool isComplete() const { return complete_; }
    bool isSuccess() const { return success_; }

private:
    void loadPuzzle(int puzzleId);
    void selectItem(int index);
    void dropItem(int categoryIndex);
    bool checkSolution();

    Game* game_;
    int puzzleId_;

    std::vector<SortingItem> items_;
    std::vector<std::string> categories_;
    std::vector<SDL_Rect> categoryBins_;

    int selectedItem_ = -1;
    float dragOffsetX_ = 0, dragOffsetY_ = 0;

    bool complete_ = false;
    bool success_ = false;

    // Sprites
    std::vector<SDL_Texture*> itemSprites_;
    SDL_Texture* binTexture_ = nullptr;
    SDL_Texture* backgroundTexture_ = nullptr;
};

// ============================================
// Reader Puzzle State
// ============================================
class ReaderPuzzleState : public GameState {
public:
    ReaderPuzzleState(Game* game, int puzzleId);
    ~ReaderPuzzleState() override;

    void enter() override;
    void exit() override;
    void update(float dt) override;
    void render() override;
    void handleInput() override;

    bool isComplete() const { return complete_; }
    bool isSuccess() const { return success_; }

private:
    void loadPuzzle(int puzzleId);
    void selectAnswer(int index);
    void submitAnswer();

    Game* game_;
    int puzzleId_;

    ReaderQuestion question_;
    int selectedAnswer_ = -1;

    bool complete_ = false;
    bool success_ = false;

    // UI
    SDL_Texture* backgroundTexture_ = nullptr;
    std::vector<SDL_Rect> answerButtons_;
};

// ============================================
// Math Puzzle State
// ============================================
class MathPuzzleState : public GameState {
public:
    MathPuzzleState(Game* game, int puzzleId);
    ~MathPuzzleState() override;

    void enter() override;
    void exit() override;
    void update(float dt) override;
    void render() override;
    void handleInput() override;

    bool isComplete() const { return complete_; }
    bool isSuccess() const { return success_; }

private:
    void loadPuzzle(int puzzleId);
    void generateProblem(int difficulty);
    void selectAnswer(int index);
    void submitAnswer();

    Game* game_;
    int puzzleId_;

    MathProblem problem_;
    int selectedAnswer_ = -1;
    std::string inputBuffer_;  // For typed answers

    bool complete_ = false;
    bool success_ = false;

    // UI
    SDL_Texture* backgroundTexture_ = nullptr;
    std::vector<SDL_Rect> answerButtons_;
};

// ============================================
// Neptune Resource Loader
// ============================================
class NeptuneResourceLoader {
public:
    static bool loadSorterSprites(AssetCache* cache,
                                  std::vector<SDL_Texture*>& sprites);
    static bool loadLabyrinthBackground(AssetCache* cache, int levelId,
                                        SDL_Texture** background);
    static bool loadLabyrinthSprites(AssetCache* cache, int levelId,
                                     std::vector<SDL_Texture*>& sprites);
    static bool loadReaderBackground(AssetCache* cache, int puzzleId,
                                     SDL_Texture** background);

    // Load palette from RSC resource
    static bool loadPalette(AssetCache* cache, const std::string& rscFile,
                           int resourceId, std::vector<uint8_t>& palette);

    // Decode RLE sprite data
    static SDL_Texture* decodeRLESprite(SDL_Renderer* renderer,
                                        const std::vector<uint8_t>& data,
                                        int width, int height,
                                        const std::vector<uint8_t>& palette);
};

} // namespace opengg
