#include "puzzle.h"
#include "renderer.h"
#include "input.h"
#include "audio.h"
#include "asset_cache.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace opengg {

// Base Puzzle implementation
Puzzle::Puzzle() = default;
Puzzle::~Puzzle() = default;

bool Puzzle::init(int difficulty, AssetCache* assetCache) {
    difficulty_ = difficulty;
    result_ = PuzzleResult::InProgress;
    progress_ = 0.0f;
    return true;
}

void Puzzle::enter() {
    timeRemaining_ = timeLimit_;
}

void Puzzle::exit() {
}

void Puzzle::update(float dt) {
    if (hasTimeLimit_ && timeRemaining_ > 0) {
        timeRemaining_ -= dt;
        if (timeRemaining_ <= 0) {
            timeRemaining_ = 0;
            complete(PuzzleResult::Failure);
        }
    }
}

void Puzzle::render(Renderer* renderer) {
    // Base implementation - draw background
    renderer->clear(Color(40, 40, 60));

    // Draw time remaining if applicable
    if (hasTimeLimit_) {
        float pct = timeRemaining_ / timeLimit_;
        int barWidth = static_cast<int>(pct * 200);
        renderer->fillRect(Rect(220, 10, 200, 20), Color(60, 60, 60));
        renderer->fillRect(Rect(220, 10, barWidth, 20),
                          pct > 0.25f ? Color(50, 200, 50) : Color(200, 50, 50));
    }
}

void Puzzle::handleInput(InputSystem* input) {
    // Check for skip/cancel
    if (input->isActionPressed(GameAction::Cancel)) {
        complete(PuzzleResult::Skipped);
    }
}

int Puzzle::getTimeRemaining() const {
    return static_cast<int>(timeRemaining_);
}

void Puzzle::complete(PuzzleResult result) {
    result_ = result;
    if (completionCallback_) {
        completionCallback_(result);
    }
}

void Puzzle::playSound(const std::string& sound) {
    if (audio_) {
        audio_->playSound(sound);
    }
}

// Balance Puzzle
BalancePuzzle::BalancePuzzle() {
    type_ = PuzzleType::Balance;
}

bool BalancePuzzle::init(int difficulty, AssetCache* assetCache) {
    Puzzle::init(difficulty, assetCache);

    // Generate weights based on difficulty
    int numWeights = 4 + difficulty * 2;
    weights_.clear();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    for (int i = 0; i < numWeights; ++i) {
        Weight w;
        w.value = dis(gen);
        w.side = -1;  // Not placed
        w.x = 100.0f + (i % 4) * 50.0f;
        w.y = 350.0f + (i / 4) * 50.0f;
        w.selected = false;
        weights_.push_back(w);
    }

    // Set target balance based on difficulty
    targetBalance_ = difficulty > 1 ? 0 : 5;  // Exact balance or within tolerance

    return true;
}

void BalancePuzzle::enter() {
    Puzzle::enter();
    selectedWeight_ = -1;
    leftTotal_ = 0;
    rightTotal_ = 0;
    balanceAngle_ = 0.0f;
}

void BalancePuzzle::update(float dt) {
    Puzzle::update(dt);
    updateBalance();

    // Smooth animation of balance angle
    float targetAngle = static_cast<float>(rightTotal_ - leftTotal_) * 2.0f;
    targetAngle = std::clamp(targetAngle, -30.0f, 30.0f);
    balanceAngle_ += (targetAngle - balanceAngle_) * dt * 5.0f;

    // Check win condition
    if (isBalanced() && leftTotal_ > 0 && rightTotal_ > 0) {
        complete(PuzzleResult::Success);
    }
}

void BalancePuzzle::updateBalance() {
    leftTotal_ = 0;
    rightTotal_ = 0;

    for (const auto& w : weights_) {
        if (w.side == 0) leftTotal_ += w.value;
        else if (w.side == 1) rightTotal_ += w.value;
    }

    progress_ = 1.0f - std::abs(rightTotal_ - leftTotal_) / 50.0f;
    progress_ = std::clamp(progress_, 0.0f, 1.0f);
}

bool BalancePuzzle::isBalanced() const {
    return std::abs(rightTotal_ - leftTotal_) <= targetBalance_;
}

void BalancePuzzle::render(Renderer* renderer) {
    Puzzle::render(renderer);

    int centerX = 320;
    int centerY = 200;

    // Draw fulcrum (triangle)
    renderer->fillRect(Rect(centerX - 20, centerY + 50, 40, 60), Color(100, 80, 60));

    // Draw balance beam (rotated rectangle - approximated)
    float angleRad = balanceAngle_ * 3.14159f / 180.0f;
    int beamHalfWidth = 150;

    int leftX = centerX - static_cast<int>(std::cos(angleRad) * beamHalfWidth);
    int leftY = centerY - static_cast<int>(std::sin(angleRad) * beamHalfWidth);
    int rightX = centerX + static_cast<int>(std::cos(angleRad) * beamHalfWidth);
    int rightY = centerY + static_cast<int>(std::sin(angleRad) * beamHalfWidth);

    renderer->drawLine(leftX, leftY, rightX, rightY, Color(139, 90, 43));

    // Draw pans
    renderer->fillRect(Rect(leftX - 40, leftY, 80, 10), Color(180, 150, 100));
    renderer->fillRect(Rect(rightX - 40, rightY, 80, 10), Color(180, 150, 100));

    // Draw weights on pans
    int leftCount = 0, rightCount = 0;
    for (size_t i = 0; i < weights_.size(); ++i) {
        const auto& w = weights_[i];
        int drawX, drawY;

        if (w.side == 0) {
            drawX = leftX - 30 + (leftCount % 3) * 25;
            drawY = leftY + 15 + (leftCount / 3) * 20;
            leftCount++;
        } else if (w.side == 1) {
            drawX = rightX - 30 + (rightCount % 3) * 25;
            drawY = rightY + 15 + (rightCount / 3) * 20;
            rightCount++;
        } else {
            drawX = static_cast<int>(w.x);
            drawY = static_cast<int>(w.y);
        }

        // Draw weight
        Color color = (selectedWeight_ == static_cast<int>(i)) ?
                      Color(255, 200, 100) : Color(200, 100, 50);
        int size = 15 + w.value * 2;
        renderer->fillRect(Rect(drawX, drawY, size, size), color);

        // Draw value
        // Would draw text here if font available
    }

    // Draw totals
    renderer->fillRect(Rect(50, 150, 60, 30), Color(60, 60, 80));
    renderer->fillRect(Rect(530, 150, 60, 30), Color(60, 60, 80));

    // Draw balance indicator
    Color balanceColor = isBalanced() ? Color(100, 255, 100) : Color(255, 100, 100);
    renderer->fillRect(Rect(centerX - 5, centerY - 30, 10, 20), balanceColor);
}

void BalancePuzzle::handleInput(InputSystem* input) {
    Puzzle::handleInput(input);

    int mx = input->getMouseX();
    int my = input->getMouseY();

    // Check for weight selection
    if (input->isMouseButtonPressed(MouseButton::Left)) {
        for (size_t i = 0; i < weights_.size(); ++i) {
            auto& w = weights_[i];
            float wx = w.side < 0 ? w.x : (w.side == 0 ? 170.0f : 470.0f);
            float wy = w.side < 0 ? w.y : 220.0f;

            if (mx >= wx && mx < wx + 30 && my >= wy && my < wy + 30) {
                selectedWeight_ = static_cast<int>(i);
                playSound("click");
                break;
            }
        }
    }

    // Place selected weight
    if (selectedWeight_ >= 0) {
        if (input->isMouseButtonReleased(MouseButton::Left)) {
            // Determine which side
            if (mx < 320) {
                weights_[selectedWeight_].side = 0;  // Left
            } else {
                weights_[selectedWeight_].side = 1;  // Right
            }
            playSound("weight_place");
            selectedWeight_ = -1;
        }
    }
}

// Gear Puzzle
GearPuzzle::GearPuzzle() {
    type_ = PuzzleType::Gear;
}

bool GearPuzzle::init(int difficulty, AssetCache* assetCache) {
    Puzzle::init(difficulty, assetCache);

    // Create slots for gears
    slots_.clear();
    int numSlots = 4 + difficulty;

    for (int i = 0; i < numSlots; ++i) {
        float x = 150.0f + (i % 3) * 120.0f;
        float y = 150.0f + (i / 3) * 100.0f;
        slots_.push_back({x, y});
    }

    // Create gears
    gears_.clear();
    for (int i = 0; i < numSlots - 1; ++i) {
        Gear g;
        g.x = 450.0f + (i % 2) * 80.0f;
        g.y = 300.0f + (i / 2) * 60.0f;
        g.radius = 25.0f + (i % 3) * 10.0f;
        g.rotation = 0.0f;
        g.speed = 0.0f;
        g.connected = false;
        g.slot = -1;
        gears_.push_back(g);
    }

    // Add driver and output gears
    Gear driver;
    driver.x = slots_[0].first;
    driver.y = slots_[0].second;
    driver.radius = 30.0f;
    driver.rotation = 0.0f;
    driver.speed = 100.0f;  // Constant rotation
    driver.connected = true;
    driver.slot = 0;
    gears_.insert(gears_.begin(), driver);
    driverGear_ = 0;

    Gear output;
    output.x = slots_.back().first;
    output.y = slots_.back().second;
    output.radius = 35.0f;
    output.rotation = 0.0f;
    output.speed = 0.0f;
    output.connected = false;
    output.slot = static_cast<int>(slots_.size()) - 1;
    gears_.push_back(output);
    outputGear_ = static_cast<int>(gears_.size()) - 1;

    return true;
}

void GearPuzzle::enter() {
    Puzzle::enter();
    selectedGear_ = -1;
}

void GearPuzzle::update(float dt) {
    Puzzle::update(dt);
    updateGears(dt);

    if (isConnected()) {
        complete(PuzzleResult::Success);
    }
}

void GearPuzzle::updateGears(float dt) {
    // Rotate driver gear
    gears_[driverGear_].rotation += gears_[driverGear_].speed * dt;

    // Propagate rotation through connected gears
    // (Simplified - would need proper mesh detection)
    for (auto& gear : gears_) {
        if (gear.slot >= 0 && gear.connected) {
            gear.rotation += gear.speed * dt;
        }
    }
}

bool GearPuzzle::isConnected() const {
    return gears_[outputGear_].connected && std::abs(gears_[outputGear_].speed) > 0.1f;
}

void GearPuzzle::render(Renderer* renderer) {
    Puzzle::render(renderer);

    // Draw slots
    for (const auto& slot : slots_) {
        renderer->fillRect(Rect(static_cast<int>(slot.first) - 5,
                               static_cast<int>(slot.second) - 5, 10, 10),
                          Color(80, 80, 80));
    }

    // Draw gears
    for (size_t i = 0; i < gears_.size(); ++i) {
        const auto& g = gears_[i];

        Color color;
        if (static_cast<int>(i) == driverGear_) {
            color = Color(100, 200, 100);  // Green driver
        } else if (static_cast<int>(i) == outputGear_) {
            color = Color(200, 100, 100);  // Red output
        } else if (static_cast<int>(i) == selectedGear_) {
            color = Color(255, 255, 100);  // Yellow selected
        } else {
            color = Color(150, 150, 150);  // Gray normal
        }

        // Draw gear as circle (simplified)
        int cx = static_cast<int>(g.x);
        int cy = static_cast<int>(g.y);
        int r = static_cast<int>(g.radius);

        // Draw gear body
        renderer->fillRect(Rect(cx - r, cy - r, r * 2, r * 2), color);

        // Draw gear teeth (simplified as lines)
        int teeth = static_cast<int>(g.radius / 5);
        for (int t = 0; t < teeth; ++t) {
            float angle = g.rotation + t * 360.0f / teeth;
            float rad = angle * 3.14159f / 180.0f;
            int tx = cx + static_cast<int>(std::cos(rad) * (r + 5));
            int ty = cy + static_cast<int>(std::sin(rad) * (r + 5));
            renderer->drawLine(cx + static_cast<int>(std::cos(rad) * r),
                              cy + static_cast<int>(std::sin(rad) * r),
                              tx, ty, color);
        }
    }
}

void GearPuzzle::handleInput(InputSystem* input) {
    Puzzle::handleInput(input);

    int mx = input->getMouseX();
    int my = input->getMouseY();

    if (input->isMouseButtonPressed(MouseButton::Left)) {
        // Check for gear selection (excluding driver and output)
        for (size_t i = 1; i < gears_.size() - 1; ++i) {
            auto& g = gears_[i];
            float dx = mx - g.x;
            float dy = my - g.y;
            if (dx * dx + dy * dy < g.radius * g.radius) {
                selectedGear_ = static_cast<int>(i);
                playSound("gear_click");
                break;
            }
        }
    }

    if (selectedGear_ >= 0 && input->isMouseButtonReleased(MouseButton::Left)) {
        // Try to place on a slot
        for (size_t si = 1; si < slots_.size() - 1; ++si) {
            float dx = mx - slots_[si].first;
            float dy = my - slots_[si].second;
            if (dx * dx + dy * dy < 400) {
                placeGear(selectedGear_, static_cast<int>(si));
                break;
            }
        }
        selectedGear_ = -1;
    }
}

void GearPuzzle::placeGear(int gearIndex, int slotIndex) {
    auto& gear = gears_[gearIndex];
    gear.slot = slotIndex;
    gear.x = slots_[slotIndex].first;
    gear.y = slots_[slotIndex].second;
    gear.connected = true;  // Simplified - would check actual connections
    gear.speed = 80.0f;     // Would calculate based on gear ratios
    playSound("gear_place");
}

// Puzzle Factory
std::unique_ptr<Puzzle> PuzzleFactory::create(PuzzleType type) {
    switch (type) {
        case PuzzleType::Balance:
            return std::make_unique<BalancePuzzle>();
        case PuzzleType::Gear:
            return std::make_unique<GearPuzzle>();
        case PuzzleType::Electricity:
            return std::make_unique<ElectricityPuzzle>();
        case PuzzleType::Jigsaw:
            return std::make_unique<JigsawPuzzle>();
        case PuzzleType::SimpleMachine:
            return std::make_unique<SimpleMachinePuzzle>();
        case PuzzleType::Magnet:
            return std::make_unique<MagnetPuzzle>();
        case PuzzleType::Force:
            return std::make_unique<ForcePuzzle>();
        case PuzzleType::Energy:
            return std::make_unique<EnergyPuzzle>();
        default:
            return std::make_unique<BalancePuzzle>();
    }
}

std::unique_ptr<Puzzle> PuzzleFactory::create(PuzzleType type, int difficulty, AssetCache* assetCache) {
    auto puzzle = create(type);
    if (puzzle) {
        puzzle->init(difficulty, assetCache);
    }
    return puzzle;
}

// Stub implementations for remaining puzzle types
ElectricityPuzzle::ElectricityPuzzle() { type_ = PuzzleType::Electricity; }
bool ElectricityPuzzle::init(int d, AssetCache* a) { return Puzzle::init(d, a); }
void ElectricityPuzzle::enter() { Puzzle::enter(); }
void ElectricityPuzzle::update(float dt) { Puzzle::update(dt); }
void ElectricityPuzzle::render(Renderer* r) { Puzzle::render(r); }
void ElectricityPuzzle::handleInput(InputSystem* i) { Puzzle::handleInput(i); }
void ElectricityPuzzle::updatePower() {}
bool ElectricityPuzzle::isCircuitComplete() const { return false; }
void ElectricityPuzzle::rotateWire(int, int) {}

JigsawPuzzle::JigsawPuzzle() { type_ = PuzzleType::Jigsaw; }
bool JigsawPuzzle::init(int d, AssetCache* a) { return Puzzle::init(d, a); }
void JigsawPuzzle::enter() { Puzzle::enter(); }
void JigsawPuzzle::update(float dt) { Puzzle::update(dt); }
void JigsawPuzzle::render(Renderer* r) { Puzzle::render(r); }
void JigsawPuzzle::handleInput(InputSystem* i) { Puzzle::handleInput(i); }
bool JigsawPuzzle::isPieceInPlace(int) const { return false; }
bool JigsawPuzzle::isComplete() const { return false; }
void JigsawPuzzle::snapPiece(int) {}

SimpleMachinePuzzle::SimpleMachinePuzzle() { type_ = PuzzleType::SimpleMachine; }
bool SimpleMachinePuzzle::init(int d, AssetCache* a) { return Puzzle::init(d, a); }
void SimpleMachinePuzzle::enter() { Puzzle::enter(); }
void SimpleMachinePuzzle::update(float dt) { Puzzle::update(dt); }
void SimpleMachinePuzzle::render(Renderer* r) { Puzzle::render(r); }
void SimpleMachinePuzzle::handleInput(InputSystem* i) { Puzzle::handleInput(i); }
void SimpleMachinePuzzle::simulate(float) {}
bool SimpleMachinePuzzle::isGoalReached() const { return false; }

MagnetPuzzle::MagnetPuzzle() { type_ = PuzzleType::Magnet; }
bool MagnetPuzzle::init(int d, AssetCache* a) { return Puzzle::init(d, a); }
void MagnetPuzzle::enter() { Puzzle::enter(); }
void MagnetPuzzle::update(float dt) { Puzzle::update(dt); }
void MagnetPuzzle::render(Renderer* r) { Puzzle::render(r); }
void MagnetPuzzle::handleInput(InputSystem* i) { Puzzle::handleInput(i); }
void MagnetPuzzle::simulate(float) {}
bool MagnetPuzzle::isGoalReached() const { return false; }
void MagnetPuzzle::toggleMagnet(int) {}

ForcePuzzle::ForcePuzzle() { type_ = PuzzleType::Force; }
bool ForcePuzzle::init(int d, AssetCache* a) { return Puzzle::init(d, a); }
void ForcePuzzle::enter() { Puzzle::enter(); }
void ForcePuzzle::update(float dt) { Puzzle::update(dt); }
void ForcePuzzle::render(Renderer* r) { Puzzle::render(r); }
void ForcePuzzle::handleInput(InputSystem* i) { Puzzle::handleInput(i); }
void ForcePuzzle::simulate(float) {}
bool ForcePuzzle::isGoalReached() const { return false; }

EnergyPuzzle::EnergyPuzzle() { type_ = PuzzleType::Energy; }
bool EnergyPuzzle::init(int d, AssetCache* a) { return Puzzle::init(d, a); }
void EnergyPuzzle::enter() { Puzzle::enter(); }
void EnergyPuzzle::update(float dt) { Puzzle::update(dt); }
void EnergyPuzzle::render(Renderer* r) { Puzzle::render(r); }
void EnergyPuzzle::handleInput(InputSystem* i) { Puzzle::handleInput(i); }
void EnergyPuzzle::transferEnergy(int, int, float) {}
bool EnergyPuzzle::isGoalReached() const { return false; }

} // namespace opengg
