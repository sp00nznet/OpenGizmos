#include "neptune/neptune_game.h"
#include "renderer.h"
#include "audio.h"
#include "input.h"
#include "asset_cache.h"
#include "font.h"
#include <SDL.h>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>

namespace opengg {

// ============================================
// NeptuneGameState Implementation
// ============================================

NeptuneGameState::NeptuneGameState(Game* game)
    : game_(game) {
}

NeptuneGameState::~NeptuneGameState() {
    // Textures are managed by AssetCache
}

void NeptuneGameState::enter() {
    SDL_Log("Neptune: Entering game state");

    loadAssets();
    loadRooms();

    // Start in first room
    if (!rooms_.empty()) {
        loadRoom(rooms_.begin()->first);
    }

    // Reset submarine state
    submarine_ = SubmarineState();
    submarine_.x = 320.0f;
    submarine_.y = 400.0f;

    currentSection_ = NeptuneSection::Submarine;
}

void NeptuneGameState::exit() {
    SDL_Log("Neptune: Exiting game state");
}

void NeptuneGameState::update(float dt) {
    switch (currentSection_) {
        case NeptuneSection::Submarine:
            updateSubmarine(dt);
            updateHazards(dt);
            checkCollisions();
            checkCanisterCollection();
            checkStationRefill(dt);
            checkRoomTransition();

            // Check win/lose conditions
            if (submarine_.oxygen <= 0 || submarine_.fuel <= 0) {
                changeSection(NeptuneSection::GameOver);
            }
            if (submarine_.canistersCollected >= submarine_.totalCanisters) {
                changeSection(NeptuneSection::Victory);
            }
            break;

        case NeptuneSection::Labyrinth:
            // Handled by LabyrinthGameState
            break;

        case NeptuneSection::SortingPuzzle:
        case NeptuneSection::ReaderPuzzle:
        case NeptuneSection::MathPuzzle:
            // Handled by respective puzzle states
            break;

        default:
            break;
    }
}

void NeptuneGameState::render() {
    Renderer* renderer = game_->getRenderer();

    switch (currentSection_) {
        case NeptuneSection::Submarine:
            renderBackground();
            renderCanisters();
            renderHazards();
            renderSubmarine();
            renderHUD();
            break;

        case NeptuneSection::MainMenu:
            // Draw menu
            renderer->clear({0, 0, 64, 255});
            break;

        case NeptuneSection::Victory:
            renderer->clear({0, 64, 0, 255});
            // Draw victory screen
            break;

        case NeptuneSection::GameOver:
            renderer->clear({64, 0, 0, 255});
            // Draw game over screen
            break;

        default:
            break;
    }
}

void NeptuneGameState::handleInput() {
    InputSystem* input = game_->getInput();

    switch (currentSection_) {
        case NeptuneSection::Submarine: {
            // Movement controls (use isKeyDown for continuous movement)
            float moveX = 0, moveY = 0;

            if (input->isKeyDown(SDL_SCANCODE_LEFT) ||
                input->isKeyDown(SDL_SCANCODE_A)) {
                moveX = -1.0f;
            }
            if (input->isKeyDown(SDL_SCANCODE_RIGHT) ||
                input->isKeyDown(SDL_SCANCODE_D)) {
                moveX = 1.0f;
            }
            if (input->isKeyDown(SDL_SCANCODE_UP) ||
                input->isKeyDown(SDL_SCANCODE_W)) {
                moveY = -1.0f;
            }
            if (input->isKeyDown(SDL_SCANCODE_DOWN) ||
                input->isKeyDown(SDL_SCANCODE_S)) {
                moveY = 1.0f;
            }

            // Apply input to submarine
            submarine_.velocityX += moveX * SubmarineState::ACCELERATION * game_->getDeltaTime();
            submarine_.velocityY += moveY * SubmarineState::ACCELERATION * game_->getDeltaTime();

            // Interaction key
            if (input->isKeyPressed(SDL_SCANCODE_SPACE) ||
                input->isKeyPressed(SDL_SCANCODE_RETURN)) {
                // Check for nearby puzzles/interactions
            }

            // Toggle lights
            if (input->isKeyPressed(SDL_SCANCODE_L)) {
                submarine_.lightsOn = !submarine_.lightsOn;
            }

            // ESC to pause/menu
            if (input->isKeyPressed(SDL_SCANCODE_ESCAPE)) {
                changeSection(NeptuneSection::MainMenu);
            }
            break;
        }

        case NeptuneSection::MainMenu:
            if (input->isKeyPressed(SDL_SCANCODE_ESCAPE) ||
                input->isKeyPressed(SDL_SCANCODE_RETURN)) {
                changeSection(NeptuneSection::Submarine);
            }
            break;

        case NeptuneSection::Victory:
        case NeptuneSection::GameOver:
            if (input->isKeyPressed(SDL_SCANCODE_RETURN) ||
                input->isKeyPressed(SDL_SCANCODE_ESCAPE)) {
                // Return to main menu or restart
                game_->popState();
            }
            break;

        default:
            break;
    }
}

void NeptuneGameState::changeSection(NeptuneSection section) {
    SDL_Log("Neptune: Changing section to %d", static_cast<int>(section));
    currentSection_ = section;
}

void NeptuneGameState::loadAssets() {
    AssetCache* cache = game_->getAssetCache();

    // For now, we'll use placeholder textures
    // In full implementation, load from SORTER.RSC, etc.

    SDL_Log("Neptune: Loading assets...");
}

void NeptuneGameState::loadRooms() {
    // Create test room structure
    NeptuneRoom room1;
    room1.id = 0;
    room1.name = "Starting Area";
    room1.backgroundId = 63968;

    // Add some canisters
    Canister c1;
    c1.x = 200; c1.y = 300; c1.roomId = 0;
    room1.canisters.push_back(c1);

    Canister c2;
    c2.x = 400; c2.y = 200; c2.roomId = 0;
    room1.canisters.push_back(c2);

    // Add oxygen station
    ResourceStation station;
    station.x = 100; station.y = 400;
    station.roomId = 0;
    station.type = ResourceStation::Oxygen;
    room1.stations.push_back(station);

    // Add exit to next room
    NeptuneRoom::Exit exit1;
    exit1.x = 600; exit1.y = 200;
    exit1.width = 40; exit1.height = 100;
    exit1.targetRoom = 1;
    exit1.targetX = 40; exit1.targetY = 240;
    room1.exits.push_back(exit1);

    rooms_[0] = room1;

    // Create second room
    NeptuneRoom room2;
    room2.id = 1;
    room2.name = "Deep Waters";
    room2.backgroundId = 63978;

    Canister c3;
    c3.x = 300; c3.y = 250; c3.roomId = 1;
    c3.puzzleRequired = 0;  // Requires solving sorting puzzle
    room2.canisters.push_back(c3);

    // Add hazard
    Hazard hazard;
    hazard.x = 400; hazard.y = 300;
    hazard.width = 60; hazard.height = 60;
    hazard.roomId = 1;
    hazard.type = Hazard::Eel;
    hazard.damage = 15.0f;
    room2.hazards.push_back(hazard);

    rooms_[1] = room2;

    submarine_.totalCanisters = 3;

    SDL_Log("Neptune: Loaded %zu rooms", rooms_.size());
}

void NeptuneGameState::loadRoom(int roomId) {
    auto it = rooms_.find(roomId);
    if (it != rooms_.end()) {
        currentRoom_ = &it->second;
        SDL_Log("Neptune: Loaded room %d - %s", roomId, currentRoom_->name.c_str());
    }
}

void NeptuneGameState::updateSubmarine(float dt) {
    // Apply drag
    submarine_.velocityX *= SubmarineState::DRAG;
    submarine_.velocityY *= SubmarineState::DRAG;

    // Clamp speed
    float speed = std::sqrt(submarine_.velocityX * submarine_.velocityX +
                           submarine_.velocityY * submarine_.velocityY);
    if (speed > SubmarineState::MAX_SPEED) {
        float scale = SubmarineState::MAX_SPEED / speed;
        submarine_.velocityX *= scale;
        submarine_.velocityY *= scale;
    }

    // Update position
    submarine_.x += submarine_.velocityX * dt;
    submarine_.y += submarine_.velocityY * dt;

    // Keep in bounds
    submarine_.x = std::clamp(submarine_.x, 20.0f, 620.0f);
    submarine_.y = std::clamp(submarine_.y, 20.0f, 460.0f);

    // Drain oxygen (always)
    submarine_.oxygen -= SubmarineState::OXYGEN_DRAIN_RATE * dt;
    submarine_.oxygen = std::max(0.0f, submarine_.oxygen);

    // Drain fuel (when moving)
    if (speed > 5.0f) {
        submarine_.fuel -= SubmarineState::FUEL_DRAIN_RATE * dt;
        submarine_.fuel = std::max(0.0f, submarine_.fuel);
    }

    // Update rotation based on velocity
    if (speed > 1.0f) {
        submarine_.rotation = std::atan2(submarine_.velocityY, submarine_.velocityX) * 180.0f / 3.14159f;
    }
}

void NeptuneGameState::updateHazards(float dt) {
    if (!currentRoom_) return;

    // Animate/update hazards
    for (auto& hazard : currentRoom_->hazards) {
        if (!hazard.active) continue;

        // Eels move around
        if (hazard.type == Hazard::Eel) {
            hazard.x += std::sin(SDL_GetTicks() * 0.002f) * 50.0f * dt;
        }
    }
}

void NeptuneGameState::checkCollisions() {
    if (!currentRoom_) return;

    // Check hazard collisions
    SDL_FRect subRect = {
        submarine_.x - 20, submarine_.y - 15,
        40, 30
    };

    for (auto& hazard : currentRoom_->hazards) {
        if (!hazard.active) continue;

        SDL_FRect hazardRect = {
            hazard.x - hazard.width/2, hazard.y - hazard.height/2,
            hazard.width, hazard.height
        };

        // Simple AABB collision
        if (subRect.x < hazardRect.x + hazardRect.w &&
            subRect.x + subRect.w > hazardRect.x &&
            subRect.y < hazardRect.y + hazardRect.h &&
            subRect.y + subRect.h > hazardRect.y) {

            // Take damage (drain oxygen)
            submarine_.oxygen -= hazard.damage * game_->getDeltaTime();

            // Knockback
            float dx = submarine_.x - hazard.x;
            float dy = submarine_.y - hazard.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist > 0) {
                submarine_.velocityX += (dx / dist) * 100.0f;
                submarine_.velocityY += (dy / dist) * 100.0f;
            }
        }
    }
}

void NeptuneGameState::checkCanisterCollection() {
    if (!currentRoom_) return;

    SDL_FRect subRect = {
        submarine_.x - 20, submarine_.y - 15,
        40, 30
    };

    for (auto& canister : currentRoom_->canisters) {
        if (canister.collected) continue;

        // Check if puzzle required
        if (canister.puzzleRequired >= 0) {
            // TODO: Check if puzzle is solved
            continue;
        }

        SDL_FRect canRect = {
            canister.x - 15, canister.y - 20,
            30, 40
        };

        if (subRect.x < canRect.x + canRect.w &&
            subRect.x + subRect.w > canRect.x &&
            subRect.y < canRect.y + canRect.h &&
            subRect.y + subRect.h > canRect.y) {

            canister.collected = true;
            submarine_.canistersCollected++;

            SDL_Log("Neptune: Collected canister! (%d/%d)",
                   submarine_.canistersCollected, submarine_.totalCanisters);

            // Play collection sound
            AudioSystem* audio = game_->getAudio();
            if (audio) {
                // audio->playSound("canister_collect");
            }
        }
    }
}

void NeptuneGameState::checkStationRefill(float dt) {
    if (!currentRoom_) return;

    SDL_FRect subRect = {
        submarine_.x - 20, submarine_.y - 15,
        40, 30
    };

    for (auto& station : currentRoom_->stations) {
        if (!station.available) continue;

        SDL_FRect stationRect = {
            station.x - 25, station.y - 25,
            50, 50
        };

        if (subRect.x < stationRect.x + stationRect.w &&
            subRect.x + subRect.w > stationRect.x &&
            subRect.y < stationRect.y + stationRect.h &&
            subRect.y + subRect.h > stationRect.y) {

            // Refill resources
            switch (station.type) {
                case ResourceStation::Oxygen:
                    submarine_.oxygen = std::min(100.0f,
                        submarine_.oxygen + station.refillRate * dt);
                    break;
                case ResourceStation::Fuel:
                    submarine_.fuel = std::min(100.0f,
                        submarine_.fuel + station.refillRate * dt);
                    break;
                case ResourceStation::Both:
                    submarine_.oxygen = std::min(100.0f,
                        submarine_.oxygen + station.refillRate * dt);
                    submarine_.fuel = std::min(100.0f,
                        submarine_.fuel + station.refillRate * dt);
                    break;
            }
        }
    }
}

void NeptuneGameState::checkRoomTransition() {
    if (!currentRoom_) return;

    SDL_FRect subRect = {
        submarine_.x - 20, submarine_.y - 15,
        40, 30
    };

    for (const auto& exit : currentRoom_->exits) {
        SDL_FRect exitRect = {
            exit.x, exit.y,
            exit.width, exit.height
        };

        if (subRect.x < exitRect.x + exitRect.w &&
            subRect.x + subRect.w > exitRect.x &&
            subRect.y < exitRect.y + exitRect.h &&
            subRect.y + subRect.h > exitRect.y) {

            // Transition to target room
            loadRoom(exit.targetRoom);
            submarine_.x = exit.targetX;
            submarine_.y = exit.targetY;
            submarine_.velocityX = 0;
            submarine_.velocityY = 0;

            SDL_Log("Neptune: Transitioning to room %d", exit.targetRoom);
            break;
        }
    }
}

void NeptuneGameState::renderBackground() {
    Renderer* renderer = game_->getRenderer();

    // Draw gradient background for underwater effect
    SDL_Color topColor = {0, 20, 60, 255};
    SDL_Color bottomColor = {0, 40, 100, 255};

    // For now, just clear to a solid color
    renderer->clear({0, 30, 80, 255});

    // TODO: Draw actual background from LABRNTH resources
}

void NeptuneGameState::renderSubmarine() {
    Renderer* renderer = game_->getRenderer();

    // Draw submarine as a simple shape for now
    Rect subRect(
        static_cast<int>(submarine_.x - 20),
        static_cast<int>(submarine_.y - 15),
        40, 30
    );

    // Body
    renderer->fillRect(subRect, Color(255, 200, 50, 255));

    // Viewport
    Rect viewport(
        static_cast<int>(submarine_.x - 8),
        static_cast<int>(submarine_.y - 5),
        16, 10
    );
    renderer->fillRect(viewport, Color(100, 200, 255, 255));

    // Propeller indicator based on velocity
    float speed = std::sqrt(submarine_.velocityX * submarine_.velocityX +
                           submarine_.velocityY * submarine_.velocityY);
    if (speed > 5.0f) {
        // Draw propeller "wake"
        Rect wake(
            static_cast<int>(submarine_.x - 30),
            static_cast<int>(submarine_.y - 3),
            10, 6
        );
        renderer->fillRect(wake, Color(150, 200, 255, 128));
    }
}

void NeptuneGameState::renderCanisters() {
    if (!currentRoom_) return;

    Renderer* renderer = game_->getRenderer();

    for (const auto& canister : currentRoom_->canisters) {
        if (canister.collected) continue;

        // Draw canister as simple shape
        Rect canRect(
            static_cast<int>(canister.x - 10),
            static_cast<int>(canister.y - 15),
            20, 30
        );

        // Locked canisters are darker
        if (canister.puzzleRequired >= 0) {
            renderer->fillRect(canRect, Color(100, 100, 100, 255));
        } else {
            renderer->fillRect(canRect, Color(50, 255, 50, 255));
        }
    }

    // Draw stations
    for (const auto& station : currentRoom_->stations) {
        Rect stationRect(
            static_cast<int>(station.x - 20),
            static_cast<int>(station.y - 20),
            40, 40
        );

        switch (station.type) {
            case ResourceStation::Oxygen:
                renderer->fillRect(stationRect, Color(100, 150, 255, 255));
                break;
            case ResourceStation::Fuel:
                renderer->fillRect(stationRect, Color(255, 150, 50, 255));
                break;
            case ResourceStation::Both:
                renderer->fillRect(stationRect, Color(150, 255, 150, 255));
                break;
        }
    }
}

void NeptuneGameState::renderHazards() {
    if (!currentRoom_) return;

    Renderer* renderer = game_->getRenderer();

    for (const auto& hazard : currentRoom_->hazards) {
        if (!hazard.active) continue;

        Rect hazardRect(
            static_cast<int>(hazard.x - hazard.width/2),
            static_cast<int>(hazard.y - hazard.height/2),
            static_cast<int>(hazard.width),
            static_cast<int>(hazard.height)
        );

        Color color;
        switch (hazard.type) {
            case Hazard::Rock:
                color = Color(80, 80, 80, 255);
                break;
            case Hazard::Eel:
                color = Color(255, 100, 100, 255);
                break;
            case Hazard::Current:
                color = Color(100, 100, 255, 128);
                break;
            case Hazard::Pressure:
                color = Color(255, 50, 50, 128);
                break;
        }

        renderer->fillRect(hazardRect, color);
    }
}

void NeptuneGameState::renderHUD() {
    Renderer* renderer = game_->getRenderer();
    TextRenderer* text = game_->getTextRenderer();

    // HUD background
    Rect hudBg(5, 5, 200, 80);
    renderer->fillRect(hudBg, Color(0, 0, 0, 180));

    // Oxygen bar
    Rect oxyBg(10, 10, 100, 15);
    renderer->fillRect(oxyBg, Color(50, 50, 50, 255));

    Rect oxyFill(10, 10, static_cast<int>(submarine_.oxygen), 15);
    renderer->fillRect(oxyFill, Color(100, 150, 255, 255));

    // Fuel bar
    Rect fuelBg(10, 30, 100, 15);
    renderer->fillRect(fuelBg, Color(50, 50, 50, 255));

    Rect fuelFill(10, 30, static_cast<int>(submarine_.fuel), 15);
    renderer->fillRect(fuelFill, Color(255, 150, 50, 255));

    // Canister count
    if (text) {
        char buf[64];
        snprintf(buf, sizeof(buf), "O2");
        text->drawText(renderer, buf, 115, 10, TextColor(255, 255, 255, 255));

        snprintf(buf, sizeof(buf), "FUEL");
        text->drawText(renderer, buf, 115, 30, TextColor(255, 255, 255, 255));

        snprintf(buf, sizeof(buf), "Canisters: %d/%d",
                submarine_.canistersCollected, submarine_.totalCanisters);
        text->drawText(renderer, buf, 10, 55, TextColor(255, 255, 255, 255));
    }

    // Room name
    if (currentRoom_ && text) {
        text->drawText(renderer, currentRoom_->name.c_str(), 10, 465, TextColor(200, 200, 200, 255));
    }
}

void NeptuneGameState::startSortingPuzzle(int puzzleId) {
    currentPuzzleId_ = puzzleId;
    auto puzzleState = std::make_unique<SortingPuzzleState>(game_, puzzleId);
    game_->pushState(std::move(puzzleState));
}

void NeptuneGameState::startReaderPuzzle(int puzzleId) {
    currentPuzzleId_ = puzzleId;
    auto puzzleState = std::make_unique<ReaderPuzzleState>(game_, puzzleId);
    game_->pushState(std::move(puzzleState));
}

void NeptuneGameState::startMathPuzzle(int puzzleId) {
    currentPuzzleId_ = puzzleId;
    auto puzzleState = std::make_unique<MathPuzzleState>(game_, puzzleId);
    game_->pushState(std::move(puzzleState));
}

void NeptuneGameState::onPuzzleComplete(bool success) {
    SDL_Log("Neptune: Puzzle %d complete - %s", currentPuzzleId_,
           success ? "SUCCESS" : "FAILED");
    currentPuzzleId_ = -1;
}

// ============================================
// LabyrinthGameState Implementation
// ============================================

LabyrinthGameState::LabyrinthGameState(Game* game, int levelId)
    : game_(game), levelId_(levelId) {
}

LabyrinthGameState::~LabyrinthGameState() {
}

void LabyrinthGameState::enter() {
    SDL_Log("Neptune: Entering labyrinth level %d", levelId_);
    loadLabyrinth(levelId_);

    playerX_ = 50.0f;
    playerY_ = 400.0f;
}

void LabyrinthGameState::exit() {
    SDL_Log("Neptune: Exiting labyrinth");
}

void LabyrinthGameState::update(float dt) {
    updatePlayer(dt);
    checkCollisions();
    checkGoal();
}

void LabyrinthGameState::render() {
    Renderer* renderer = game_->getRenderer();

    // Clear to black
    renderer->clear(Color(20, 20, 30, 255));

    // Draw background if available
    if (backgroundTexture_) {
        renderer->drawSprite(backgroundTexture_, 0, 0);
    }

    // Draw walls
    for (const auto& wall : walls_) {
        Rect wallRect(wall.x, wall.y, wall.w, wall.h);
        renderer->fillRect(wallRect, Color(60, 60, 80, 255));
    }

    // Draw goal
    Rect goalRect(goal_.x, goal_.y, goal_.w, goal_.h);
    renderer->fillRect(goalRect, Color(50, 255, 50, 255));

    // Draw player
    Rect playerRect(
        static_cast<int>(playerX_ - 10),
        static_cast<int>(playerY_ - 10),
        20, 20
    );
    renderer->fillRect(playerRect, Color(255, 200, 50, 255));
}

void LabyrinthGameState::handleInput() {
    InputSystem* input = game_->getInput();

    float speed = 150.0f;

    if (input->isKeyPressed(SDL_SCANCODE_LEFT) || input->isKeyPressed(SDL_SCANCODE_A)) {
        playerVelX_ = -speed;
    } else if (input->isKeyPressed(SDL_SCANCODE_RIGHT) || input->isKeyPressed(SDL_SCANCODE_D)) {
        playerVelX_ = speed;
    } else {
        playerVelX_ = 0;
    }

    if (input->isKeyPressed(SDL_SCANCODE_UP) || input->isKeyPressed(SDL_SCANCODE_W)) {
        playerVelY_ = -speed;
    } else if (input->isKeyPressed(SDL_SCANCODE_DOWN) || input->isKeyPressed(SDL_SCANCODE_S)) {
        playerVelY_ = speed;
    } else {
        playerVelY_ = 0;
    }

    if (input->isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        game_->popState();
    }
}

void LabyrinthGameState::loadLabyrinth(int levelId) {
    // Create simple test labyrinth
    walls_.clear();

    // Border walls
    walls_.push_back({0, 0, 640, 10});     // Top
    walls_.push_back({0, 470, 640, 10});   // Bottom
    walls_.push_back({0, 0, 10, 480});     // Left
    walls_.push_back({630, 0, 10, 480});   // Right

    // Internal walls
    walls_.push_back({100, 100, 200, 20});
    walls_.push_back({200, 200, 20, 200});
    walls_.push_back({300, 50, 20, 150});
    walls_.push_back({400, 150, 150, 20});
    walls_.push_back({100, 350, 300, 20});
    walls_.push_back({500, 250, 20, 200});

    // Goal in corner
    goal_ = {580, 30, 40, 40};
}

void LabyrinthGameState::updatePlayer(float dt) {
    playerX_ += playerVelX_ * dt;
    playerY_ += playerVelY_ * dt;

    // Keep in bounds
    playerX_ = std::clamp(playerX_, 15.0f, 625.0f);
    playerY_ = std::clamp(playerY_, 15.0f, 465.0f);
}

void LabyrinthGameState::checkCollisions() {
    SDL_Rect playerRect = {
        static_cast<int>(playerX_ - 10),
        static_cast<int>(playerY_ - 10),
        20, 20
    };

    for (const auto& wall : walls_) {
        if (SDL_HasIntersection(&playerRect, &wall)) {
            // Simple collision response - push back
            SDL_Rect intersection;
            SDL_IntersectRect(&playerRect, &wall, &intersection);

            if (intersection.w < intersection.h) {
                // Horizontal collision
                if (playerX_ < wall.x + wall.w / 2) {
                    playerX_ -= intersection.w;
                } else {
                    playerX_ += intersection.w;
                }
            } else {
                // Vertical collision
                if (playerY_ < wall.y + wall.h / 2) {
                    playerY_ -= intersection.h;
                } else {
                    playerY_ += intersection.h;
                }
            }
        }
    }
}

void LabyrinthGameState::checkGoal() {
    SDL_Rect playerRect = {
        static_cast<int>(playerX_ - 10),
        static_cast<int>(playerY_ - 10),
        20, 20
    };

    if (SDL_HasIntersection(&playerRect, &goal_)) {
        SDL_Log("Neptune: Labyrinth complete!");
        game_->popState();
    }
}

// ============================================
// SortingPuzzleState Implementation
// ============================================

SortingPuzzleState::SortingPuzzleState(Game* game, int puzzleId)
    : game_(game), puzzleId_(puzzleId) {
}

SortingPuzzleState::~SortingPuzzleState() {
}

void SortingPuzzleState::enter() {
    SDL_Log("Neptune: Starting sorting puzzle %d", puzzleId_);
    loadPuzzle(puzzleId_);
}

void SortingPuzzleState::exit() {
    SDL_Log("Neptune: Exiting sorting puzzle");
}

void SortingPuzzleState::loadPuzzle(int puzzleId) {
    // Create test sorting puzzle
    categories_ = {"Fish", "Mammals", "Plants"};

    // Category bins
    categoryBins_.push_back({50, 350, 150, 100});
    categoryBins_.push_back({245, 350, 150, 100});
    categoryBins_.push_back({440, 350, 150, 100});

    // Items to sort
    SortingItem item1 = {0, "Salmon", 0, 35283, 100, 150, false};
    SortingItem item2 = {1, "Dolphin", 1, 35284, 200, 150, false};
    SortingItem item3 = {2, "Seaweed", 2, 35285, 300, 150, false};
    SortingItem item4 = {3, "Tuna", 0, 35286, 400, 150, false};
    SortingItem item5 = {4, "Whale", 1, 35287, 500, 150, false};
    SortingItem item6 = {5, "Kelp", 2, 35288, 150, 220, false};

    items_.push_back(item1);
    items_.push_back(item2);
    items_.push_back(item3);
    items_.push_back(item4);
    items_.push_back(item5);
    items_.push_back(item6);
}

void SortingPuzzleState::update(float dt) {
    if (complete_) return;

    // Check if all items are sorted
    bool allSorted = true;
    for (const auto& item : items_) {
        if (!item.sorted) {
            allSorted = false;
            break;
        }
    }

    if (allSorted) {
        success_ = checkSolution();
        complete_ = true;

        SDL_Log("Neptune: Sorting puzzle %s", success_ ? "SOLVED" : "FAILED");
    }
}

void SortingPuzzleState::render() {
    Renderer* renderer = game_->getRenderer();
    TextRenderer* text = game_->getTextRenderer();

    renderer->clear(Color(40, 60, 80, 255));

    // Draw category bins
    for (size_t i = 0; i < categoryBins_.size(); i++) {
        Rect binRect(categoryBins_[i].x, categoryBins_[i].y,
                    categoryBins_[i].w, categoryBins_[i].h);
        renderer->fillRect(binRect, Color(60, 80, 100, 255));

        if (text && i < categories_.size()) {
            text->drawText(renderer, categories_[i].c_str(),
                           categoryBins_[i].x + 20,
                           categoryBins_[i].y + 40,
                           TextColor(255, 255, 255, 255));
        }
    }

    // Draw items
    for (size_t i = 0; i < items_.size(); i++) {
        const auto& item = items_[i];

        Rect itemRect(
            static_cast<int>(item.x - 30),
            static_cast<int>(item.y - 20),
            60, 40
        );

        Color color = (static_cast<int>(i) == selectedItem_)
            ? Color(255, 255, 100, 255)
            : Color(200, 200, 200, 255);

        renderer->fillRect(itemRect, color);

        if (text) {
            text->drawText(renderer, item.name.c_str(),
                           itemRect.x + 5,
                           itemRect.y + 10,
                           TextColor(0, 0, 0, 255));
        }
    }

    // Instructions
    if (text) {
        text->drawText(renderer, "Drag items to the correct category bins",
                        150, 20, TextColor(255, 255, 255, 255));
    }
}

void SortingPuzzleState::handleInput() {
    InputSystem* input = game_->getInput();

    int mouseX = input->getMouseX();
    int mouseY = input->getMouseY();

    if (input->isMouseButtonPressed(MouseButton::Left)) {
        // Check if clicking an item
        for (size_t i = 0; i < items_.size(); i++) {
            if (items_[i].sorted) continue;

            SDL_Rect itemRect = {
                static_cast<int>(items_[i].x - 30),
                static_cast<int>(items_[i].y - 20),
                60, 40
            };

            if (mouseX >= itemRect.x && mouseX < itemRect.x + itemRect.w &&
                mouseY >= itemRect.y && mouseY < itemRect.y + itemRect.h) {
                selectedItem_ = static_cast<int>(i);
                dragOffsetX_ = items_[i].x - mouseX;
                dragOffsetY_ = items_[i].y - mouseY;
                break;
            }
        }
    }

    if (input->isMouseButtonDown(MouseButton::Left) && selectedItem_ >= 0) {
        // Drag item
        items_[selectedItem_].x = mouseX + dragOffsetX_;
        items_[selectedItem_].y = mouseY + dragOffsetY_;
    }

    if (input->isMouseButtonReleased(MouseButton::Left) && selectedItem_ >= 0) {
        // Check if dropped in a bin
        for (size_t i = 0; i < categoryBins_.size(); i++) {
            if (mouseX >= categoryBins_[i].x &&
                mouseX < categoryBins_[i].x + categoryBins_[i].w &&
                mouseY >= categoryBins_[i].y &&
                mouseY < categoryBins_[i].y + categoryBins_[i].h) {

                // Snap to bin
                items_[selectedItem_].x = categoryBins_[i].x + categoryBins_[i].w / 2;
                items_[selectedItem_].y = categoryBins_[i].y + 20;
                items_[selectedItem_].sorted = true;
                // Store which category it was dropped in
                items_[selectedItem_].category = static_cast<int>(i);
                break;
            }
        }
        selectedItem_ = -1;
    }

    if (input->isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        complete_ = true;
        success_ = false;
        game_->popState();
    }
}

void SortingPuzzleState::selectItem(int index) {
    selectedItem_ = index;
}

void SortingPuzzleState::dropItem(int categoryIndex) {
    if (selectedItem_ >= 0 && categoryIndex < static_cast<int>(categories_.size())) {
        items_[selectedItem_].sorted = true;
        selectedItem_ = -1;
    }
}

bool SortingPuzzleState::checkSolution() {
    // For test puzzle: Fish=0, Mammals=1, Plants=2
    // Items: Salmon(0), Dolphin(1), Seaweed(2), Tuna(0), Whale(1), Kelp(2)
    // Verify each item is in its correct category

    // In real implementation, would check actual categories
    return true;  // Simplified for now
}

// ============================================
// ReaderPuzzleState Implementation
// ============================================

ReaderPuzzleState::ReaderPuzzleState(Game* game, int puzzleId)
    : game_(game), puzzleId_(puzzleId) {
}

ReaderPuzzleState::~ReaderPuzzleState() {
}

void ReaderPuzzleState::enter() {
    SDL_Log("Neptune: Starting reader puzzle %d", puzzleId_);
    loadPuzzle(puzzleId_);
}

void ReaderPuzzleState::exit() {
    SDL_Log("Neptune: Exiting reader puzzle");
}

void ReaderPuzzleState::loadPuzzle(int puzzleId) {
    // Create test reading comprehension puzzle
    question_.passage = "The submarine dove deeper into the ocean. "
                       "Schools of fish swam past the viewport. "
                       "Captain Nero checked the oxygen levels.";

    question_.question = "What did Captain Nero check?";
    question_.choices = {"The fish", "The oxygen levels", "The depth", "The engine"};
    question_.correctAnswer = 1;
    question_.difficulty = 1;

    // Answer buttons
    for (int i = 0; i < 4; i++) {
        answerButtons_.push_back({100, 280 + i * 45, 440, 40});
    }
}

void ReaderPuzzleState::update(float dt) {
    // Nothing to update in reading puzzle
}

void ReaderPuzzleState::render() {
    Renderer* renderer = game_->getRenderer();
    TextRenderer* text = game_->getTextRenderer();

    renderer->clear(Color(50, 50, 70, 255));

    if (!text) return;

    // Draw passage box
    Rect passageBox(50, 30, 540, 100);
    renderer->fillRect(passageBox, Color(40, 40, 50, 255));
    text->drawText(renderer, question_.passage.c_str(), 60, 50, TextColor(255, 255, 255, 255));

    // Draw question
    text->drawText(renderer, question_.question.c_str(), 100, 180, TextColor(255, 255, 200, 255));

    // Draw answer choices
    for (size_t i = 0; i < answerButtons_.size() && i < question_.choices.size(); i++) {
        Color bgColor = (static_cast<int>(i) == selectedAnswer_)
            ? Color(100, 150, 200, 255)
            : Color(70, 70, 90, 255);

        Rect btnRect(answerButtons_[i].x, answerButtons_[i].y,
                    answerButtons_[i].w, answerButtons_[i].h);
        renderer->fillRect(btnRect, bgColor);

        char label[256];
        snprintf(label, sizeof(label), "%c) %s",
                'A' + static_cast<int>(i), question_.choices[i].c_str());
        text->drawText(renderer, label,
                        answerButtons_[i].x + 10,
                        answerButtons_[i].y + 10,
                        TextColor(255, 255, 255, 255));
    }

    // Instructions
    text->drawText(renderer, "Click an answer, then press ENTER to submit",
                    150, 460, TextColor(200, 200, 200, 255));
}

void ReaderPuzzleState::handleInput() {
    InputSystem* input = game_->getInput();

    int mouseX = input->getMouseX();
    int mouseY = input->getMouseY();

    if (input->isMouseButtonPressed(MouseButton::Left)) {
        // Check answer button clicks
        for (size_t i = 0; i < answerButtons_.size(); i++) {
            if (mouseX >= answerButtons_[i].x &&
                mouseX < answerButtons_[i].x + answerButtons_[i].w &&
                mouseY >= answerButtons_[i].y &&
                mouseY < answerButtons_[i].y + answerButtons_[i].h) {
                selectedAnswer_ = static_cast<int>(i);
                break;
            }
        }
    }

    // Keyboard shortcuts
    if (input->isKeyPressed(SDL_SCANCODE_1)) selectedAnswer_ = 0;
    if (input->isKeyPressed(SDL_SCANCODE_2)) selectedAnswer_ = 1;
    if (input->isKeyPressed(SDL_SCANCODE_3)) selectedAnswer_ = 2;
    if (input->isKeyPressed(SDL_SCANCODE_4)) selectedAnswer_ = 3;

    if (input->isKeyPressed(SDL_SCANCODE_RETURN) && selectedAnswer_ >= 0) {
        submitAnswer();
    }

    if (input->isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        complete_ = true;
        success_ = false;
        game_->popState();
    }
}

void ReaderPuzzleState::selectAnswer(int index) {
    if (index >= 0 && index < static_cast<int>(question_.choices.size())) {
        selectedAnswer_ = index;
    }
}

void ReaderPuzzleState::submitAnswer() {
    success_ = (selectedAnswer_ == question_.correctAnswer);
    complete_ = true;

    SDL_Log("Neptune: Reader puzzle %s (answer %d, correct %d)",
           success_ ? "CORRECT" : "WRONG",
           selectedAnswer_, question_.correctAnswer);

    game_->popState();
}

// ============================================
// MathPuzzleState Implementation
// ============================================

MathPuzzleState::MathPuzzleState(Game* game, int puzzleId)
    : game_(game), puzzleId_(puzzleId) {
}

MathPuzzleState::~MathPuzzleState() {
}

void MathPuzzleState::enter() {
    SDL_Log("Neptune: Starting math puzzle %d", puzzleId_);
    loadPuzzle(puzzleId_);
}

void MathPuzzleState::exit() {
    SDL_Log("Neptune: Exiting math puzzle");
}

void MathPuzzleState::loadPuzzle(int puzzleId) {
    // Generate random math problem based on difficulty
    int difficulty = (puzzleId % 3) + 1;
    generateProblem(difficulty);

    // Answer buttons
    for (int i = 0; i < 4; i++) {
        answerButtons_.push_back({100 + i * 130, 300, 110, 50});
    }
}

void MathPuzzleState::generateProblem(int difficulty) {
    // Generate problem based on difficulty
    int a, b;

    switch (difficulty) {
        case 1:  // Easy: addition/subtraction 1-20
            a = rand() % 10 + 1;
            b = rand() % 10 + 1;
            if (rand() % 2 == 0) {
                problem_.problem = std::to_string(a) + " + " + std::to_string(b) + " = ?";
                problem_.answer = a + b;
                problem_.type = MathProblem::Addition;
            } else {
                if (a < b) std::swap(a, b);
                problem_.problem = std::to_string(a) + " - " + std::to_string(b) + " = ?";
                problem_.answer = a - b;
                problem_.type = MathProblem::Subtraction;
            }
            break;

        case 2:  // Medium: multiplication 1-10
            a = rand() % 10 + 1;
            b = rand() % 10 + 1;
            problem_.problem = std::to_string(a) + " x " + std::to_string(b) + " = ?";
            problem_.answer = a * b;
            problem_.type = MathProblem::Multiplication;
            break;

        case 3:  // Hard: division
            b = rand() % 9 + 2;  // Divisor 2-10
            problem_.answer = rand() % 10 + 1;
            a = b * problem_.answer;
            problem_.problem = std::to_string(a) + " / " + std::to_string(b) + " = ?";
            problem_.type = MathProblem::Division;
            break;
    }

    problem_.difficulty = difficulty;

    // Generate answer choices
    problem_.choices.clear();
    problem_.choices.push_back(problem_.answer);

    // Add wrong answers
    while (problem_.choices.size() < 4) {
        int wrong = problem_.answer + (rand() % 11) - 5;
        if (wrong != problem_.answer && wrong >= 0) {
            bool duplicate = false;
            for (int c : problem_.choices) {
                if (c == wrong) { duplicate = true; break; }
            }
            if (!duplicate) {
                problem_.choices.push_back(wrong);
            }
        }
    }

    // Shuffle choices
    for (size_t i = problem_.choices.size() - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        std::swap(problem_.choices[i], problem_.choices[j]);
    }

    // Find correct choice index
    for (size_t i = 0; i < problem_.choices.size(); i++) {
        if (problem_.choices[i] == problem_.answer) {
            problem_.correctChoice = static_cast<int>(i);
            break;
        }
    }
}

void MathPuzzleState::update(float dt) {
    // Nothing to update
}

void MathPuzzleState::render() {
    Renderer* renderer = game_->getRenderer();
    TextRenderer* text = game_->getTextRenderer();

    renderer->clear(Color(50, 60, 50, 255));

    if (!text) return;

    // Draw problem (centered, larger for visibility)
    text->drawText(renderer, problem_.problem.c_str(), 200, 150, TextColor(255, 255, 255, 255));

    // Draw answer choices
    for (size_t i = 0; i < answerButtons_.size() && i < problem_.choices.size(); i++) {
        Color bgColor = (static_cast<int>(i) == selectedAnswer_)
            ? Color(100, 200, 100, 255)
            : Color(70, 90, 70, 255);

        Rect btnRect(answerButtons_[i].x, answerButtons_[i].y,
                    answerButtons_[i].w, answerButtons_[i].h);
        renderer->fillRect(btnRect, bgColor);

        char label[32];
        snprintf(label, sizeof(label), "%d", problem_.choices[i]);
        text->drawText(renderer, label,
                        answerButtons_[i].x + 40,
                        answerButtons_[i].y + 15,
                        TextColor(255, 255, 255, 255));
    }

    // Instructions
    text->drawText(renderer, "Click an answer or press 1-4",
                    180, 400, TextColor(200, 200, 200, 255));
}

void MathPuzzleState::handleInput() {
    InputSystem* input = game_->getInput();

    int mouseX = input->getMouseX();
    int mouseY = input->getMouseY();

    if (input->isMouseButtonPressed(MouseButton::Left)) {
        for (size_t i = 0; i < answerButtons_.size(); i++) {
            if (mouseX >= answerButtons_[i].x &&
                mouseX < answerButtons_[i].x + answerButtons_[i].w &&
                mouseY >= answerButtons_[i].y &&
                mouseY < answerButtons_[i].y + answerButtons_[i].h) {
                selectedAnswer_ = static_cast<int>(i);
                submitAnswer();
                break;
            }
        }
    }

    // Number keys
    if (input->isKeyPressed(SDL_SCANCODE_1)) { selectedAnswer_ = 0; submitAnswer(); }
    if (input->isKeyPressed(SDL_SCANCODE_2)) { selectedAnswer_ = 1; submitAnswer(); }
    if (input->isKeyPressed(SDL_SCANCODE_3)) { selectedAnswer_ = 2; submitAnswer(); }
    if (input->isKeyPressed(SDL_SCANCODE_4)) { selectedAnswer_ = 3; submitAnswer(); }

    if (input->isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        complete_ = true;
        success_ = false;
        game_->popState();
    }
}

void MathPuzzleState::selectAnswer(int index) {
    if (index >= 0 && index < static_cast<int>(problem_.choices.size())) {
        selectedAnswer_ = index;
    }
}

void MathPuzzleState::submitAnswer() {
    if (selectedAnswer_ < 0) return;

    success_ = (selectedAnswer_ == problem_.correctChoice);
    complete_ = true;

    SDL_Log("Neptune: Math puzzle %s (selected %d = %d, correct %d = %d)",
           success_ ? "CORRECT" : "WRONG",
           selectedAnswer_, problem_.choices[selectedAnswer_],
           problem_.correctChoice, problem_.answer);

    game_->popState();
}

// ============================================
// NeptuneResourceLoader Implementation
// ============================================

bool NeptuneResourceLoader::loadSorterSprites(AssetCache* cache,
                                              std::vector<SDL_Texture*>& sprites) {
    // Load sprites from SORTER.RSC
    // In full implementation, would extract from RSC file

    SDL_Log("Neptune: Loading SORTER sprites...");

    // For now, return false (not implemented)
    return false;
}

bool NeptuneResourceLoader::loadLabyrinthBackground(AssetCache* cache, int levelId,
                                                    SDL_Texture** background) {
    // Load background from LABRNTH*.RSC
    // Resource IDs: 63968, 63978, 63988, 63998

    SDL_Log("Neptune: Loading labyrinth background for level %d", levelId);

    // For now, return false
    return false;
}

bool NeptuneResourceLoader::loadLabyrinthSprites(AssetCache* cache, int levelId,
                                                 std::vector<SDL_Texture*>& sprites) {
    // Load sprites from LABRNTH*.RSC
    // Resource IDs: 63969, 63970

    SDL_Log("Neptune: Loading labyrinth sprites for level %d", levelId);

    return false;
}

bool NeptuneResourceLoader::loadReaderBackground(AssetCache* cache, int puzzleId,
                                                 SDL_Texture** background) {
    // Load from READER*.RSC
    // Resource IDs: 63868, 63878, etc.

    SDL_Log("Neptune: Loading reader background for puzzle %d", puzzleId);

    return false;
}

bool NeptuneResourceLoader::loadPalette(AssetCache* cache, const std::string& rscFile,
                                        int resourceId, std::vector<uint8_t>& palette) {
    // Load doubled-byte palette from RSC resource
    // Format: 00 R 00 G 00 B (6 bytes per color, 1536 bytes total)

    std::vector<uint8_t> data = cache->getData(rscFile + ":" + std::to_string(resourceId));
    if (data.size() < 1536) {
        return false;
    }

    palette.resize(768);  // 256 * 3 bytes
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = data[i * 6 + 1];  // R
        palette[i * 3 + 1] = data[i * 6 + 3];  // G
        palette[i * 3 + 2] = data[i * 6 + 5];  // B
    }

    return true;
}

SDL_Texture* NeptuneResourceLoader::decodeRLESprite(SDL_Renderer* renderer,
                                                    const std::vector<uint8_t>& data,
                                                    int width, int height,
                                                    const std::vector<uint8_t>& palette) {
    if (data.empty() || width <= 0 || height <= 0 || palette.size() < 768) {
        return nullptr;
    }

    // Create surface
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!surface) {
        return nullptr;
    }

    // Fill with transparent
    SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

    uint32_t* pixels = static_cast<uint32_t*>(surface->pixels);
    int pitch = surface->pitch / 4;

    int x = 0, y = 0;
    size_t i = 0;

    while (i < data.size() && y < height) {
        uint8_t byte = data[i++];

        if (byte == 0x00) {
            // Row terminator
            x = 0;
            y++;
        } else if (byte == 0xFF && i + 1 < data.size()) {
            // RLE: FF <pixel> <count>
            uint8_t pixel = data[i++];
            uint8_t count = data[i++];

            for (int j = 0; j <= count && x < width; j++) {
                if (pixel != 0) {  // 0 is transparent
                    uint32_t color = SDL_MapRGBA(surface->format,
                        palette[pixel * 3 + 0],
                        palette[pixel * 3 + 1],
                        palette[pixel * 3 + 2],
                        255);
                    pixels[y * pitch + x] = color;
                }
                x++;
            }
        } else {
            // Literal pixel
            if (byte != 0 && x < width) {
                uint32_t color = SDL_MapRGBA(surface->format,
                    palette[byte * 3 + 0],
                    palette[byte * 3 + 1],
                    palette[byte * 3 + 2],
                    255);
                pixels[y * pitch + x] = color;
            }
            x++;
        }
    }

    // Create texture from surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}

} // namespace opengg
