#pragma once

#include "formats/dat_format.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

struct SDL_Texture;

namespace opengg {

class Renderer;
class InputSystem;
class AudioSystem;
class AssetCache;

// Puzzle result
enum class PuzzleResult {
    InProgress,
    Success,
    Failure,
    Skipped
};

// Base puzzle class
class Puzzle {
public:
    Puzzle();
    virtual ~Puzzle();

    // Initialize puzzle
    virtual bool init(int difficulty, AssetCache* assetCache);

    // Lifecycle
    virtual void enter();
    virtual void exit();
    virtual void update(float dt);
    virtual void render(Renderer* renderer);
    virtual void handleInput(InputSystem* input);

    // Get puzzle result
    PuzzleResult getResult() const { return result_; }
    bool isComplete() const { return result_ != PuzzleResult::InProgress; }

    // Puzzle info
    PuzzleType getType() const { return type_; }
    int getDifficulty() const { return difficulty_; }
    int getTimeRemaining() const;
    float getProgress() const { return progress_; }

    // Audio
    void setAudioSystem(AudioSystem* audio) { audio_ = audio; }

    // Callbacks
    using CompletionCallback = std::function<void(PuzzleResult)>;
    void setCompletionCallback(CompletionCallback cb) { completionCallback_ = std::move(cb); }

protected:
    void complete(PuzzleResult result);
    void playSound(const std::string& sound);

    PuzzleType type_ = PuzzleType::Balance;
    int difficulty_ = 1;
    float progress_ = 0.0f;
    PuzzleResult result_ = PuzzleResult::InProgress;

    // Time limit
    bool hasTimeLimit_ = false;
    float timeLimit_ = 0.0f;
    float timeRemaining_ = 0.0f;

    // Audio
    AudioSystem* audio_ = nullptr;

    // Callbacks
    CompletionCallback completionCallback_;
};

// Balance puzzle - weight the scales
class BalancePuzzle : public Puzzle {
public:
    BalancePuzzle();

    bool init(int difficulty, AssetCache* assetCache) override;
    void enter() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    void handleInput(InputSystem* input) override;

private:
    struct Weight {
        int value;
        int side;  // 0 = left, 1 = right, -1 = available
        float x, y;
        bool selected;
    };

    void updateBalance();
    bool isBalanced() const;

    std::vector<Weight> weights_;
    int leftTotal_ = 0;
    int rightTotal_ = 0;
    int targetBalance_ = 0;
    float balanceAngle_ = 0.0f;
    int selectedWeight_ = -1;
};

// Electricity puzzle - complete the circuit
class ElectricityPuzzle : public Puzzle {
public:
    ElectricityPuzzle();

    bool init(int difficulty, AssetCache* assetCache) override;
    void enter() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    void handleInput(InputSystem* input) override;

private:
    struct Wire {
        int x, y;
        int connections;  // Bitmask: up, right, down, left
        int rotation;     // 0-3
        bool powered;
    };

    void updatePower();
    bool isCircuitComplete() const;
    void rotateWire(int x, int y);

    std::vector<Wire> wires_;
    int gridWidth_ = 5;
    int gridHeight_ = 5;
    int startX_ = 0, startY_ = 0;
    int endX_ = 0, endY_ = 0;
};

// Gear puzzle - connect the gears
class GearPuzzle : public Puzzle {
public:
    GearPuzzle();

    bool init(int difficulty, AssetCache* assetCache) override;
    void enter() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    void handleInput(InputSystem* input) override;

private:
    struct Gear {
        float x, y;
        float radius;
        float rotation;
        float speed;
        bool connected;
        int slot;  // -1 = available
    };

    void updateGears(float dt);
    bool isConnected() const;
    void placeGear(int gearIndex, int slotIndex);

    std::vector<Gear> gears_;
    std::vector<std::pair<float, float>> slots_;
    int driverGear_ = 0;
    int outputGear_ = 0;
    int selectedGear_ = -1;
};

// Jigsaw puzzle
class JigsawPuzzle : public Puzzle {
public:
    JigsawPuzzle();

    bool init(int difficulty, AssetCache* assetCache) override;
    void enter() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    void handleInput(InputSystem* input) override;

private:
    struct Piece {
        int correctX, correctY;
        float currentX, currentY;
        bool placed;
        SDL_Texture* texture;
    };

    bool isPieceInPlace(int index) const;
    bool isComplete() const;
    void snapPiece(int index);

    std::vector<Piece> pieces_;
    int gridWidth_ = 4;
    int gridHeight_ = 3;
    int pieceWidth_ = 80;
    int pieceHeight_ = 80;
    int selectedPiece_ = -1;
    float dragOffsetX_ = 0, dragOffsetY_ = 0;
};

// Simple machine puzzle - levers and pulleys
class SimpleMachinePuzzle : public Puzzle {
public:
    SimpleMachinePuzzle();

    bool init(int difficulty, AssetCache* assetCache) override;
    void enter() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    void handleInput(InputSystem* input) override;

private:
    struct Lever {
        float x, y;
        float length;
        float angle;
        float fulcrumPos;  // 0.0 - 1.0 along length
    };

    struct Pulley {
        float x, y;
        float radius;
        float ropeLength;
    };

    void simulate(float dt);
    bool isGoalReached() const;

    std::vector<Lever> levers_;
    std::vector<Pulley> pulleys_;
    float objectY_ = 0.0f;
    float goalY_ = 0.0f;
    bool simulating_ = false;
};

// Magnet puzzle - attract/repel
class MagnetPuzzle : public Puzzle {
public:
    MagnetPuzzle();

    bool init(int difficulty, AssetCache* assetCache) override;
    void enter() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    void handleInput(InputSystem* input) override;

private:
    struct Magnet {
        float x, y;
        bool northUp;  // Polarity
        bool moveable;
    };

    struct Ball {
        float x, y;
        float velX, velY;
        bool magnetic;
    };

    void simulate(float dt);
    bool isGoalReached() const;
    void toggleMagnet(int index);

    std::vector<Magnet> magnets_;
    Ball ball_;
    float goalX_ = 0, goalY_ = 0;
    float goalRadius_ = 30.0f;
};

// Force puzzle - apply forces to move objects
class ForcePuzzle : public Puzzle {
public:
    ForcePuzzle();

    bool init(int difficulty, AssetCache* assetCache) override;
    void enter() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    void handleInput(InputSystem* input) override;

private:
    struct ForceArrow {
        float x, y;
        float angle;
        float magnitude;
    };

    struct PhysicsObject {
        float x, y;
        float velX, velY;
        float mass;
    };

    void simulate(float dt);
    bool isGoalReached() const;

    std::vector<ForceArrow> arrows_;
    PhysicsObject object_;
    float goalX_ = 0, goalY_ = 0;
    float goalRadius_ = 30.0f;
    bool simulating_ = false;
    int selectedArrow_ = -1;
};

// Energy puzzle - transfer energy between objects
class EnergyPuzzle : public Puzzle {
public:
    EnergyPuzzle();

    bool init(int difficulty, AssetCache* assetCache) override;
    void enter() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    void handleInput(InputSystem* input) override;

private:
    struct EnergyNode {
        float x, y;
        float energy;
        float maxEnergy;
        bool source;
        bool target;
        std::vector<int> connections;
    };

    void transferEnergy(int from, int to, float amount);
    bool isGoalReached() const;

    std::vector<EnergyNode> nodes_;
    int selectedNode_ = -1;
    float targetEnergy_ = 100.0f;
};

// Puzzle factory
class PuzzleFactory {
public:
    static std::unique_ptr<Puzzle> create(PuzzleType type);
    static std::unique_ptr<Puzzle> create(PuzzleType type, int difficulty, AssetCache* assetCache);
};

} // namespace opengg
