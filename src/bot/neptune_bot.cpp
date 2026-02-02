#include "bot/neptune_bot.h"
#include "input.h"
#include "game_loop.h"
#include "room.h"
#include "player.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Bot {

NeptuneBot::NeptuneBot() {
    // Initialize known answers for common puzzle types
    // These would be loaded from a data file in a real implementation
}

NeptuneBot::~NeptuneBot() = default;

void NeptuneBot::initialize(Game* game) {
    game_ = game;

    // Initialize submarine state
    submarine_ = SubmarineState();

    // Clear pathfinding data
    currentPath_.clear();
    currentPathIndex_ = 0;
    canisters_.clear();

    // Build initial navigation map
    buildNavigationMap();

    SDL_Log("Neptune Bot initialized");
}

void NeptuneBot::shutdown() {
    currentPath_.clear();
    canisters_.clear();
    knownAnswers_.clear();
    navGrid_.clear();

    SDL_Log("Neptune Bot shutdown");
}

void NeptuneBot::update(float deltaTime) {
    if (mode_ == BotMode::Disabled) return;

    decisionCooldown_ -= deltaTime;

    // Check if we're stuck
    if (currentPath_.size() > 0 && currentPathIndex_ < currentPath_.size()) {
        Waypoint current = currentPath_[currentPathIndex_];
        float dist = std::sqrt(
            std::pow(current.x - lastPosition_.x, 2) +
            std::pow(current.y - lastPosition_.y, 2)
        );

        if (dist < 2.0f) {
            stuckTimer_ += deltaTime;
            if (stuckTimer_ > 3.0f) {
                // We're stuck, recalculate path
                SDL_Log("Neptune Bot: Stuck, recalculating path");
                currentPath_.clear();
                currentPathIndex_ = 0;
                stuckTimer_ = 0.0f;
            }
        } else {
            stuckTimer_ = 0.0f;
            lastPosition_ = current;
        }
    }

    // Analyze current game state
    analyzeGameState();
}

BotDecision NeptuneBot::getNextDecision() {
    if (mode_ == BotMode::Disabled || decisionCooldown_ > 0) {
        return BotDecision::None;
    }

    decisionCooldown_ = MIN_DECISION_INTERVAL;

    // Priority-based decision making for Operation Neptune

    // 1. Critical resource check - return to surface if dying
    if (submarine_.oxygen < CRITICAL_RESOURCE_THRESHOLD ||
        submarine_.fuel < CRITICAL_RESOURCE_THRESHOLD) {
        if (shouldReturnToBase()) {
            currentObjective_ = BotObjective::ReturnToBase;
            return decideNavigation();
        }
    }

    // 2. Low oxygen - seek oxygen station
    if (submarine_.oxygen < LOW_OXYGEN_THRESHOLD) {
        currentObjective_ = BotObjective::SeekOxygen;
        Waypoint oxygenStation = findOxygenStation();
        if (oxygenStation.roomId >= 0) {
            currentPath_ = findPathTo(oxygenStation.x, oxygenStation.y);
            currentPathIndex_ = 0;
            return decideNavigation();
        }
    }

    // 3. Low fuel - seek fuel station
    if (submarine_.fuel < LOW_FUEL_THRESHOLD) {
        currentObjective_ = BotObjective::SeekFuel;
        Waypoint fuelStation = findFuelStation();
        if (fuelStation.roomId >= 0) {
            currentPath_ = findPathTo(fuelStation.x, fuelStation.y);
            currentPathIndex_ = 0;
            return decideNavigation();
        }
    }

    // 4. Seek next canister
    currentObjective_ = BotObjective::SeekCanister;
    Waypoint nextCanister = findNearestUncollectedCanister();
    if (nextCanister.roomId >= 0) {
        if (currentPath_.empty()) {
            currentPath_ = findPathTo(nextCanister.x, nextCanister.y);
            currentPathIndex_ = 0;
        }
        return decideNavigation();
    }

    // 5. All canisters collected - return to base
    if (submarine_.canistersCollected >= submarine_.totalCanisters) {
        currentObjective_ = BotObjective::ReturnToBase;
        return decideNavigation();
    }

    // 6. Default exploration
    currentObjective_ = BotObjective::Idle;
    return decideNavigation();
}

void NeptuneBot::executeDecision(BotDecision decision, InputSystem* input) {
    if (!input) return;

    // Simulate key presses based on decision
    // In a real implementation, this would inject input events
    switch (decision) {
        case BotDecision::MoveLeft:
            // input->simulateKeyPress(SDL_SCANCODE_LEFT);
            SDL_Log("Bot: Move Left");
            break;

        case BotDecision::MoveRight:
            // input->simulateKeyPress(SDL_SCANCODE_RIGHT);
            SDL_Log("Bot: Move Right");
            break;

        case BotDecision::MoveUp:
            // input->simulateKeyPress(SDL_SCANCODE_UP);
            SDL_Log("Bot: Move Up");
            break;

        case BotDecision::MoveDown:
            // input->simulateKeyPress(SDL_SCANCODE_DOWN);
            SDL_Log("Bot: Move Down");
            break;

        case BotDecision::Interact:
            // input->simulateKeyPress(SDL_SCANCODE_SPACE);
            SDL_Log("Bot: Interact");
            break;

        case BotDecision::SolvePuzzle:
            // Handle puzzle solving
            SDL_Log("Bot: Solving Puzzle");
            break;

        default:
            break;
    }
}

void NeptuneBot::analyzeGameState() {
    // In a real implementation, this would read from the game state
    // For now, we simulate state updates

    // Update submarine position from player
    // submarine_.x = player->getX();
    // submarine_.y = player->getY();

    // Update resources (would come from game state)
    // submarine_.oxygen -= 0.1f * deltaTime;  // Oxygen depletes over time
    // submarine_.fuel -= 0.05f * deltaTime;   // Fuel depletes when moving
}

void NeptuneBot::onRoomChanged(Room* newRoom) {
    SDL_Log("Neptune Bot: Room changed");

    // Rebuild navigation for new room
    buildNavigationMap();

    // Clear current path - need to recalculate
    currentPath_.clear();
    currentPathIndex_ = 0;

    // Scan for canisters and resources in new room
    // This would read from room entity data
}

void NeptuneBot::onPuzzleStarted(int puzzleType) {
    SDL_Log("Neptune Bot: Puzzle started - type %d", puzzleType);
    currentObjective_ = BotObjective::SolvePuzzle;
}

void NeptuneBot::onPuzzleEnded(bool success) {
    SDL_Log("Neptune Bot: Puzzle ended - %s", success ? "success" : "failure");

    // Resume previous objective
    currentObjective_ = BotObjective::SeekCanister;
    currentPath_.clear();
}

std::string NeptuneBot::getStatusText() const {
    std::ostringstream ss;

    switch (currentObjective_) {
        case BotObjective::Idle:
            ss << "Idle - Scanning area";
            break;
        case BotObjective::SeekCanister:
            ss << "Seeking canister (" << submarine_.canistersCollected
               << "/" << submarine_.totalCanisters << ")";
            break;
        case BotObjective::SeekOxygen:
            ss << "LOW OXYGEN - Seeking station";
            break;
        case BotObjective::SeekFuel:
            ss << "LOW FUEL - Seeking station";
            break;
        case BotObjective::ReturnToBase:
            ss << "Returning to base";
            break;
        case BotObjective::SolvePuzzle:
            ss << "Solving puzzle";
            break;
        case BotObjective::AvoidHazard:
            ss << "Avoiding hazard";
            break;
    }

    ss << " [O2: " << (int)submarine_.oxygen << "% Fuel: " << (int)submarine_.fuel << "%]";

    return ss.str();
}

float NeptuneBot::getCompletionProgress() const {
    if (submarine_.totalCanisters == 0) return 0.0f;
    return (float)submarine_.canistersCollected / (float)submarine_.totalCanisters;
}

// Navigation helpers

void NeptuneBot::buildNavigationMap() {
    // Build a grid-based navigation map from room data
    // For now, create a simple open grid

    gridWidth_ = 20;  // 640 / 32
    gridHeight_ = 15; // 480 / 32

    navGrid_.resize(gridHeight_);
    for (int y = 0; y < gridHeight_; y++) {
        navGrid_[y].resize(gridWidth_);
        for (int x = 0; x < gridWidth_; x++) {
            navGrid_[y][x] = 1; // 1 = passable, 0 = blocked
        }
    }

    // In real implementation, mark obstacles from room collision data
}

std::vector<Waypoint> NeptuneBot::findPathTo(float targetX, float targetY) {
    std::vector<Waypoint> path;

    // Simple direct path for now
    // A real implementation would use A* pathfinding

    Waypoint start;
    start.x = submarine_.x;
    start.y = submarine_.y;

    Waypoint end;
    end.x = targetX;
    end.y = targetY;

    // Create intermediate waypoints
    float dx = targetX - submarine_.x;
    float dy = targetY - submarine_.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    int numWaypoints = std::max(1, (int)(dist / 50.0f));

    for (int i = 1; i <= numWaypoints; i++) {
        Waypoint wp;
        float t = (float)i / (float)numWaypoints;
        wp.x = submarine_.x + dx * t;
        wp.y = submarine_.y + dy * t;
        path.push_back(wp);
    }

    return path;
}

Waypoint NeptuneBot::findNearestUncollectedCanister() {
    Waypoint nearest;
    nearest.roomId = -1;
    float nearestDist = 999999.0f;

    for (const auto& canister : canisters_) {
        if (canister.collected) continue;

        float dx = canister.x - submarine_.x;
        float dy = canister.y - submarine_.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < nearestDist) {
            nearestDist = dist;
            nearest.x = canister.x;
            nearest.y = canister.y;
            nearest.roomId = canister.roomId;
        }
    }

    return nearest;
}

Waypoint NeptuneBot::findOxygenStation() {
    // In real implementation, find oxygen refill locations
    Waypoint station;
    station.roomId = -1;
    return station;
}

Waypoint NeptuneBot::findFuelStation() {
    // In real implementation, find fuel refill locations
    Waypoint station;
    station.roomId = -1;
    return station;
}

bool NeptuneBot::isPathClear(float x1, float y1, float x2, float y2) {
    // Check if path between two points is clear of obstacles
    // Simple line-of-sight check

    int gx1 = (int)(x1 / GRID_CELL_SIZE);
    int gy1 = (int)(y1 / GRID_CELL_SIZE);
    int gx2 = (int)(x2 / GRID_CELL_SIZE);
    int gy2 = (int)(y2 / GRID_CELL_SIZE);

    // Bresenham's line algorithm
    int dx = std::abs(gx2 - gx1);
    int dy = std::abs(gy2 - gy1);
    int sx = gx1 < gx2 ? 1 : -1;
    int sy = gy1 < gy2 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (gx1 >= 0 && gx1 < gridWidth_ && gy1 >= 0 && gy1 < gridHeight_) {
            if (navGrid_[gy1][gx1] == 0) {
                return false; // Blocked
            }
        }

        if (gx1 == gx2 && gy1 == gy2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            gx1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            gy1 += sy;
        }
    }

    return true;
}

BotDecision NeptuneBot::decideNavigation() {
    if (currentPath_.empty() || currentPathIndex_ >= currentPath_.size()) {
        return BotDecision::Wait;
    }

    Waypoint target = currentPath_[currentPathIndex_];

    float dx = target.x - submarine_.x;
    float dy = target.y - submarine_.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Reached waypoint?
    if (dist < 10.0f) {
        currentPathIndex_++;
        if (currentPathIndex_ >= currentPath_.size()) {
            return BotDecision::Wait;
        }
        target = currentPath_[currentPathIndex_];
        dx = target.x - submarine_.x;
        dy = target.y - submarine_.y;
    }

    // Decide direction to move
    if (std::abs(dx) > std::abs(dy)) {
        return dx > 0 ? BotDecision::MoveRight : BotDecision::MoveLeft;
    } else {
        return dy > 0 ? BotDecision::MoveDown : BotDecision::MoveUp;
    }
}

bool NeptuneBot::shouldReturnToBase() {
    // Calculate if we have enough resources to return
    // For now, simple threshold check
    return submarine_.oxygen < CRITICAL_RESOURCE_THRESHOLD ||
           submarine_.fuel < CRITICAL_RESOURCE_THRESHOLD;
}

// Puzzle solving

BotDecision NeptuneBot::handleSortingPuzzle() {
    // Handle sorting puzzle logic
    // Would analyze items and determine correct categories
    return BotDecision::SolvePuzzle;
}

BotDecision NeptuneBot::handleReadingPuzzle() {
    // Handle reading comprehension puzzle
    // Would parse text and select correct answer
    return BotDecision::SolvePuzzle;
}

BotDecision NeptuneBot::handleMathPuzzle() {
    // Handle math puzzle
    return BotDecision::SolvePuzzle;
}

int NeptuneBot::calculateMathAnswer(const std::string& problem) {
    // Parse and solve math problem
    // Simple implementation for basic operations
    return 0;
}

int NeptuneBot::findSortingCategory(int itemId) {
    // Determine which category an item belongs to
    // Would use game knowledge base
    return 0;
}

} // namespace Bot
