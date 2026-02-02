#include "bot/gizmos_bot.h"
#include "input.h"
#include "game_loop.h"
#include "room.h"
#include "player.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <queue>

namespace Bot {

GizmosBot::GizmosBot() = default;
GizmosBot::~GizmosBot() = default;

void GizmosBot::initialize(Game* game) {
    game_ = game;

    // Reset state
    state_ = GameState();
    knownParts_.clear();
    knownSolutions_.clear();
    exploredRooms_.clear();
    roomGraph_.clear();
    currentPath_.clear();
    pathIndex_ = 0;

    std::fill(std::begin(buildingsCompleted_), std::end(buildingsCompleted_), false);

    SDL_Log("Gizmos Bot initialized");
}

void GizmosBot::shutdown() {
    knownParts_.clear();
    knownSolutions_.clear();
    exploredRooms_.clear();
    roomGraph_.clear();
    currentPath_.clear();

    SDL_Log("Gizmos Bot shutdown");
}

void GizmosBot::update(float deltaTime) {
    if (mode_ == BotMode::Disabled) return;

    decisionCooldown_ -= deltaTime;
    mortyCheckTimer_ -= deltaTime;

    // Periodically check for Morty
    if (mortyCheckTimer_ <= 0) {
        mortyCheckTimer_ = 0.5f; // Check every 500ms

        // Would check Morty position from game state
        state_.mortyNearby = isMortyThreat();
    }

    // Analyze current game state
    analyzeGameState();
}

BotDecision GizmosBot::getNextDecision() {
    if (mode_ == BotMode::Disabled || decisionCooldown_ > 0) {
        return BotDecision::None;
    }

    decisionCooldown_ = MIN_DECISION_INTERVAL;

    // Priority-based decision making for Gizmos & Gadgets

    // 1. If Morty is nearby, evade!
    if (state_.mortyNearby) {
        currentObjective_ = BotObjective::EvadeMorty;
        return evadeMorty();
    }

    // 2. If in a race, handle racing
    if (state_.inRace) {
        currentObjective_ = BotObjective::Race;
        return handleRacing();
    }

    // 3. If vehicle is complete, go build it
    if (state_.partsHave >= state_.partsNeeded && !state_.vehicleComplete) {
        currentObjective_ = BotObjective::BuildVehicle;
        return decideVehicleBuilding();
    }

    // 4. Seek parts we need
    if (state_.partsHave < state_.partsNeeded) {
        PartInfo* nextPart = findBestAvailablePart();
        if (nextPart) {
            currentObjective_ = BotObjective::SeekPart;
            return decidePartCollection();
        }
    }

    // 5. Explore to find more parts
    currentObjective_ = BotObjective::Explore;
    return decideExploration();
}

void GizmosBot::executeDecision(BotDecision decision, InputSystem* input) {
    if (!input) return;

    switch (decision) {
        case BotDecision::MoveLeft:
            SDL_Log("Gizmos Bot: Move Left");
            break;
        case BotDecision::MoveRight:
            SDL_Log("Gizmos Bot: Move Right");
            break;
        case BotDecision::MoveUp:
            SDL_Log("Gizmos Bot: Move Up (Ladder)");
            break;
        case BotDecision::MoveDown:
            SDL_Log("Gizmos Bot: Move Down (Ladder)");
            break;
        case BotDecision::Jump:
            SDL_Log("Gizmos Bot: Jump");
            break;
        case BotDecision::Climb:
            SDL_Log("Gizmos Bot: Climb");
            break;
        case BotDecision::Interact:
            SDL_Log("Gizmos Bot: Interact");
            break;
        case BotDecision::EnterDoor:
            SDL_Log("Gizmos Bot: Enter Door");
            break;
        case BotDecision::SolvePuzzle:
            SDL_Log("Gizmos Bot: Solve Puzzle");
            break;
        default:
            break;
    }
}

void GizmosBot::analyzeGameState() {
    // Would read from actual game state
    // Update building/floor/room info
    // Update part collection status
    // Update Morty position
}

void GizmosBot::onRoomChanged(Room* newRoom) {
    SDL_Log("Gizmos Bot: Room changed");

    // Mark room as explored
    if (newRoom) {
        // exploredRooms_.insert(newRoom->getId());
    }

    // Clear path - need to recalculate
    currentPath_.clear();
    pathIndex_ = 0;

    // Scan for parts, doors, ladders in new room
}

void GizmosBot::onPuzzleStarted(int puzzleType) {
    SDL_Log("Gizmos Bot: Puzzle started - type %d", puzzleType);
    currentObjective_ = BotObjective::SolvePuzzle;
}

void GizmosBot::onPuzzleEnded(bool success) {
    SDL_Log("Gizmos Bot: Puzzle ended - %s", success ? "success" : "failure");

    if (success) {
        // Record solution for future reference
    }

    currentObjective_ = BotObjective::SeekPart;
}

std::string GizmosBot::getStatusText() const {
    std::ostringstream ss;

    const char* buildingNames[] = {"Cars", "Planes", "Alt"};

    ss << buildingNames[state_.currentBuilding] << " F" << (state_.currentFloor + 1);
    ss << " - ";

    switch (currentObjective_) {
        case BotObjective::Idle:
            ss << "Idle";
            break;
        case BotObjective::Explore:
            ss << "Exploring";
            break;
        case BotObjective::SeekPart:
            ss << "Seeking part (" << state_.partsHave << "/" << state_.partsNeeded << ")";
            break;
        case BotObjective::SolvePuzzle:
            ss << "Solving puzzle";
            break;
        case BotObjective::EvadeMorty:
            ss << "EVADING MORTY!";
            break;
        case BotObjective::BuildVehicle:
            ss << "Building vehicle";
            break;
        case BotObjective::Race:
            ss << "Racing!";
            break;
        case BotObjective::ReturnToStart:
            ss << "Returning to start";
            break;
    }

    return ss.str();
}

float GizmosBot::getCompletionProgress() const {
    // Calculate overall game progress
    int buildingsComplete = 0;
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        if (buildingsCompleted_[i]) buildingsComplete++;
    }

    float buildingProgress = (float)buildingsComplete / (float)NUM_BUILDINGS;
    float partProgress = (float)state_.partsHave / (float)state_.partsNeeded;

    return (buildingProgress * 0.8f) + (partProgress * 0.2f);
}

// Navigation

std::vector<int> GizmosBot::findPathToRoom(int targetRoom) {
    std::vector<int> path;

    // BFS to find path through room graph
    if (roomGraph_.empty()) return path;

    std::queue<int> queue;
    std::unordered_map<int, int> parent;

    queue.push(state_.currentRoom);
    parent[state_.currentRoom] = -1;

    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();

        if (current == targetRoom) {
            // Reconstruct path
            int node = targetRoom;
            while (node != -1) {
                path.insert(path.begin(), node);
                node = parent[node];
            }
            return path;
        }

        auto it = roomGraph_.find(current);
        if (it != roomGraph_.end()) {
            for (const auto& conn : it->second) {
                if (parent.find(conn.toRoom) == parent.end()) {
                    parent[conn.toRoom] = current;
                    queue.push(conn.toRoom);
                }
            }
        }
    }

    return path; // Empty if no path found
}

std::vector<RoomConnection> GizmosBot::getRoomConnections(int roomId) {
    auto it = roomGraph_.find(roomId);
    if (it != roomGraph_.end()) {
        return it->second;
    }
    return {};
}

bool GizmosBot::canReachRoom(int targetRoom) {
    return !findPathToRoom(targetRoom).empty();
}

// Part management

PartInfo* GizmosBot::findBestAvailablePart() {
    PartInfo* best = nullptr;
    int bestPriority = -1;

    for (auto& part : knownParts_) {
        if (part.collected) continue;

        int priority = getPartPriority(part);
        if (priority > bestPriority && canReachRoom(part.roomId)) {
            bestPriority = priority;
            best = &part;
        }
    }

    return best;
}

bool GizmosBot::needPartType(int partType) {
    // Check if we need this type of part for current vehicle
    // Would check against vehicle requirements
    return true;
}

int GizmosBot::getPartPriority(const PartInfo& part) {
    int priority = 0;

    // Higher quality parts are better
    priority += part.quality * 10;

    // Parts we need are higher priority
    if (needPartType(part.partType)) {
        priority += 50;
    }

    // Closer parts are slightly preferred
    // priority -= estimatePathCost(state_.currentRoom, part.roomId);

    return priority;
}

// Morty handling

bool GizmosBot::isMortyThreat() {
    if (state_.mortyRoom != state_.currentRoom) {
        return false;
    }

    // Check if Morty is close
    // float dist = ...
    // return dist < 100.0f;

    return false;
}

BotDecision GizmosBot::evadeMorty() {
    // Find safest direction away from Morty
    // Move towards a door or ladder if possible

    float dx = state_.mortyX - 320; // Player assumed at center
    float dy = state_.mortyY - 240;

    // Run opposite direction
    if (std::abs(dx) > std::abs(dy)) {
        return dx > 0 ? BotDecision::MoveLeft : BotDecision::MoveRight;
    } else {
        return dy > 0 ? BotDecision::MoveUp : BotDecision::MoveDown;
    }
}

std::vector<int> GizmosBot::getMortySafeRooms() {
    std::vector<int> safeRooms;

    // Find rooms where Morty isn't likely to go
    // Based on Morty's patrol patterns

    return safeRooms;
}

// Racing

BotDecision GizmosBot::handleRacing() {
    // Simple racing AI
    // Move forward and avoid obstacles

    return BotDecision::MoveRight; // Basic: just keep moving
}

void GizmosBot::optimizeRaceStrategy() {
    // Analyze track and plan optimal route
}

// Decision making

BotDecision GizmosBot::decideExploration() {
    // Find unexplored rooms and navigate to them
    auto connections = getRoomConnections(state_.currentRoom);

    for (const auto& conn : connections) {
        if (exploredRooms_.find(conn.toRoom) == exploredRooms_.end()) {
            // Found unexplored room, go there
            return BotDecision::EnterDoor;
        }
    }

    // All connected rooms explored, try to find a path to unexplored area
    // Or just wander

    return BotDecision::MoveRight; // Default exploration
}

BotDecision GizmosBot::decidePartCollection() {
    PartInfo* target = findBestAvailablePart();
    if (!target) {
        return BotDecision::Wait;
    }

    // Navigate to part
    if (target->roomId != state_.currentRoom) {
        currentPath_ = findPathToRoom(target->roomId);
        if (!currentPath_.empty()) {
            return BotDecision::EnterDoor;
        }
    }

    // In the same room, move towards part
    // Would calculate direction to part position

    return BotDecision::Interact; // Try to collect
}

BotDecision GizmosBot::decideVehicleBuilding() {
    // Navigate to building area (Floor 1)
    if (state_.currentFloor != 0) {
        return BotDecision::MoveDown; // Go to ground floor
    }

    // At building area, interact to build
    return BotDecision::Interact;
}

// Puzzle solving implementations

BotDecision GizmosBot::handleBalancePuzzle() {
    // Balance puzzle: add/remove weights to balance scales
    // Would analyze current state and calculate needed weights
    return BotDecision::SolvePuzzle;
}

BotDecision GizmosBot::handleElectricityPuzzle() {
    // Connect wires to complete circuit
    // Would trace circuit and find connections
    return BotDecision::SolvePuzzle;
}

BotDecision GizmosBot::handleGearPuzzle() {
    // Arrange gears to transfer motion
    // Would calculate gear ratios and positions
    return BotDecision::SolvePuzzle;
}

BotDecision GizmosBot::handleMagnetPuzzle() {
    // Use magnets to move objects
    // Would calculate magnetic field interactions
    return BotDecision::SolvePuzzle;
}

BotDecision GizmosBot::handleSimpleMachinePuzzle() {
    // Levers, pulleys, inclined planes
    // Would calculate mechanical advantage
    return BotDecision::SolvePuzzle;
}

BotDecision GizmosBot::handleJigsawPuzzle() {
    // Assemble pieces to form image
    // Would use pattern matching
    return BotDecision::SolvePuzzle;
}

BotDecision GizmosBot::handleEnergyPuzzle() {
    // Transform energy types
    // Would trace energy conversions
    return BotDecision::SolvePuzzle;
}

BotDecision GizmosBot::handleForcePuzzle() {
    // Apply forces to move objects
    // Would calculate force vectors
    return BotDecision::SolvePuzzle;
}

} // namespace Bot
