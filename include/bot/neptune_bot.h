#pragma once

#include "bot_manager.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <cmath>

namespace Bot {

// Navigation waypoint for pathfinding
struct Waypoint {
    float x, y;
    int roomId = -1;
    bool visited = false;
    float cost = 0.0f;       // Cost to reach this waypoint
    float heuristic = 0.0f;  // Estimated cost to goal
    int parentIndex = -1;
};

// Puzzle answer for reading/math/sorting challenges
struct PuzzleAnswer {
    int questionId;
    int correctAnswer;      // Index of correct choice
    std::string explanation;
};

// Canister location info
struct CanisterInfo {
    float x, y;
    int roomId;
    bool collected = false;
    int requiredPuzzleId = -1;  // Puzzle to solve to access
};

// Operation Neptune Bot - plays the submarine game
class NeptuneBot : public GameBot {
public:
    NeptuneBot();
    ~NeptuneBot() override;

    // GameBot interface
    void initialize(opengg::Game* game) override;
    void shutdown() override;
    void update(float deltaTime) override;

    BotDecision getNextDecision() override;
    void executeDecision(BotDecision decision, opengg::InputSystem* input) override;

    void analyzeGameState() override;
    void onRoomChanged(opengg::Room* newRoom) override;
    void onPuzzleStarted(int puzzleType) override;
    void onPuzzleEnded(bool success) override;

    GameType getGameType() const override { return GameType::OperationNeptune; }
    std::string getStatusText() const override;
    float getCompletionProgress() const override;

private:
    // Submarine state
    struct SubmarineState {
        float x = 0, y = 0;
        float oxygen = 100.0f;
        float fuel = 100.0f;
        int depth = 0;
        int canistersCollected = 0;
        int totalCanisters = 10;  // Typical level count
    };

    // Navigation
    void buildNavigationMap();
    std::vector<Waypoint> findPathTo(float targetX, float targetY);
    Waypoint findNearestUncollectedCanister();
    Waypoint findOxygenStation();
    Waypoint findFuelStation();
    bool isPathClear(float x1, float y1, float x2, float y2);

    // Puzzle solving
    BotDecision handleSortingPuzzle();
    BotDecision handleReadingPuzzle();
    BotDecision handleMathPuzzle();
    int calculateMathAnswer(const std::string& problem);
    int findSortingCategory(int itemId);

    // Decision making
    BotDecision decideNavigation();
    BotDecision decideResourceManagement();
    BotDecision handleHazard();
    bool shouldReturnToBase();

    // State
    SubmarineState submarine_;
    std::vector<CanisterInfo> canisters_;
    std::vector<Waypoint> currentPath_;
    int currentPathIndex_ = 0;

    // Navigation map
    std::vector<std::vector<int>> navGrid_;
    int gridWidth_ = 0;
    int gridHeight_ = 0;
    static constexpr int GRID_CELL_SIZE = 32;

    // Puzzle knowledge base
    std::unordered_map<int, PuzzleAnswer> knownAnswers_;

    // Thresholds
    static constexpr float LOW_OXYGEN_THRESHOLD = 30.0f;
    static constexpr float LOW_FUEL_THRESHOLD = 25.0f;
    static constexpr float CRITICAL_RESOURCE_THRESHOLD = 15.0f;

    // Current state
    enum class BotObjective {
        Idle,
        SeekCanister,
        SeekOxygen,
        SeekFuel,
        ReturnToBase,
        SolvePuzzle,
        AvoidHazard
    };
    BotObjective currentObjective_ = BotObjective::Idle;
    float stuckTimer_ = 0.0f;
    Waypoint lastPosition_;
};

} // namespace Bot
