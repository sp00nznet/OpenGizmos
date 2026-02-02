#pragma once

#include "bot_manager.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace Bot {

// Part information
struct PartInfo {
    int partId;
    int partType;       // Engine, wheels, body, etc.
    int quality;        // 1-3, higher is better
    int roomId;
    float x, y;
    bool collected = false;
    int requiredPuzzleId = -1;
};

// Room connection info
struct RoomConnection {
    int fromRoom;
    int toRoom;
    float doorX, doorY;
    enum Type { Door, Ladder, Elevator } type;
};

// Puzzle solution patterns
struct PuzzleSolution {
    int puzzleId;
    int puzzleType;
    std::vector<int> steps;  // Sequence of moves/actions
    bool solved = false;
};

// Gizmos & Gadgets Bot
class GizmosBot : public GameBot {
public:
    GizmosBot();
    ~GizmosBot() override;

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

    GameType getGameType() const override { return GameType::GizmosAndGadgets; }
    std::string getStatusText() const override;
    float getCompletionProgress() const override;

private:
    // Game state
    struct GameState {
        int currentBuilding = 0;  // 0-2 (Cars, Planes, Alternative)
        int currentFloor = 0;     // 0-4
        int currentRoom = 0;

        // Collected parts per vehicle type
        std::vector<PartInfo> collectedParts;

        // Morty state
        float mortyX = 0, mortyY = 0;
        int mortyRoom = -1;
        bool mortyNearby = false;

        // Vehicle building progress
        int partsNeeded = 5;
        int partsHave = 0;
        bool vehicleComplete = false;

        // Race state
        bool inRace = false;
        float racePosition = 0.0f;
    };

    // Navigation
    std::vector<int> findPathToRoom(int targetRoom);
    std::vector<RoomConnection> getRoomConnections(int roomId);
    bool canReachRoom(int targetRoom);
    float estimatePathCost(int fromRoom, int toRoom);

    // Part selection
    PartInfo* findBestAvailablePart();
    bool needPartType(int partType);
    int getPartPriority(const PartInfo& part);

    // Puzzle solving
    BotDecision handleBalancePuzzle();
    BotDecision handleElectricityPuzzle();
    BotDecision handleGearPuzzle();
    BotDecision handleMagnetPuzzle();
    BotDecision handleSimpleMachinePuzzle();
    BotDecision handleJigsawPuzzle();
    BotDecision handleEnergyPuzzle();
    BotDecision handleForcePuzzle();

    // Morty avoidance
    bool isMortyThreat();
    BotDecision evadeMorty();
    std::vector<int> getMortySafeRooms();

    // Racing
    BotDecision handleRacing();
    void optimizeRaceStrategy();

    // Decision making
    BotDecision decideExploration();
    BotDecision decidePartCollection();
    BotDecision decideVehicleBuilding();

    // State
    GameState state_;
    std::vector<PartInfo> knownParts_;
    std::vector<PuzzleSolution> knownSolutions_;
    std::unordered_set<int> exploredRooms_;
    std::unordered_map<int, std::vector<RoomConnection>> roomGraph_;

    // Navigation
    std::vector<int> currentPath_;
    int pathIndex_ = 0;

    // Current objective
    enum class BotObjective {
        Idle,
        Explore,
        SeekPart,
        SolvePuzzle,
        EvadeMorty,
        BuildVehicle,
        Race,
        ReturnToStart
    };
    BotObjective currentObjective_ = BotObjective::Idle;

    // Timers
    float stuckTimer_ = 0.0f;
    float mortyCheckTimer_ = 0.0f;

    // Building/vehicle tracking
    static constexpr int NUM_BUILDINGS = 3;
    static constexpr int FLOORS_PER_BUILDING = 5;
    bool buildingsCompleted_[NUM_BUILDINGS] = {false, false, false};
};

} // namespace Bot
