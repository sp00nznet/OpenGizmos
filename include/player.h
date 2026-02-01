#pragma once

#include "entity.h"
#include "formats/dat_format.h"
#include <vector>
#include <array>
#include <cstdint>

namespace opengg {

class Room;
class InputSystem;
class AudioSystem;
class Renderer;

// Player state
enum class PlayerState {
    Idle,
    Walking,
    Running,
    Jumping,
    Falling,
    Climbing,
    ClimbingIdle,
    Ducking,
    Puzzle,       // In puzzle minigame
    Building,     // Building vehicle
    Racing,       // Racing vehicle
    Celebrating,
    Hurt,
    Dead
};

// Direction
enum class Direction {
    Left = 0,
    Right = 1
};

// Collected part info
struct CollectedPart {
    int partType;
    int category;
    bool used;
};

// Player class - the main character (Super Solver)
class Player : public Entity {
public:
    Player();
    ~Player() override;

    // Initialize player
    void init() override;

    // Update player (call each frame)
    void update(float dt) override;

    // Render player
    void render(Renderer* renderer) override;

    // Handle input (call before update)
    void handleInput(InputSystem* input);

    // State
    PlayerState getState() const { return state_; }
    void setState(PlayerState state);
    bool isOnGround() const { return onGround_; }
    bool isClimbing() const { return state_ == PlayerState::Climbing || state_ == PlayerState::ClimbingIdle; }

    // Direction
    Direction getDirection() const { return direction_; }
    void setDirection(Direction dir);

    // Movement parameters
    void setWalkSpeed(float speed) { walkSpeed_ = speed; }
    void setJumpForce(float force) { jumpForce_ = force; }
    void setGravity(float gravity) { gravity_ = gravity; }

    // Room interaction
    void setRoom(Room* room) override;
    void enterRoom(Room* room, int x, int y);

    // Parts collection
    void collectPart(int partType, int category);
    bool hasPart(int partType) const;
    int getPartCount(int category) const;
    int getTotalPartCount() const;
    const std::vector<CollectedPart>& getCollectedParts() const { return collectedParts_; }

    // Vehicle building
    bool canBuildVehicle(int vehicleType) const;
    void usePart(int partType);

    // Puzzles
    void enterPuzzle(int puzzleId);
    void exitPuzzle(bool success);
    int getCurrentPuzzleId() const { return currentPuzzleId_; }

    // Health/lives
    int getLives() const { return lives_; }
    void setLives(int lives) { lives_ = lives; }
    void loseLife();
    void gainLife();
    bool isDead() const { return lives_ <= 0; }

    // Score
    int getScore() const { return score_; }
    void addScore(int points);

    // Audio
    void setAudioSystem(AudioSystem* audio) { audio_ = audio; }

    // Interaction
    void interact();  // Try to interact with nearby objects

    // Collision callbacks
    void onCollision(Entity* other);
    void onPartCollected(PartEntity* part);
    void onDoorEntered(DoorEntity* door);

private:
    void updateMovement(float dt);
    void updatePhysics(float dt);
    void updateAnimation();
    void checkCollisions();
    bool checkGroundCollision();
    bool checkCeilingCollision();
    bool checkWallCollision(float dx);
    void applyGravity(float dt);
    void jump();
    void startClimbing();
    void stopClimbing();

    // State
    PlayerState state_ = PlayerState::Idle;
    PlayerState prevState_ = PlayerState::Idle;
    Direction direction_ = Direction::Right;

    // Physics
    float walkSpeed_ = 150.0f;
    float runSpeed_ = 250.0f;
    float jumpForce_ = 350.0f;
    float gravity_ = 800.0f;
    float climbSpeed_ = 100.0f;
    bool onGround_ = false;
    bool wasOnGround_ = false;

    // Input state
    bool wantMoveLeft_ = false;
    bool wantMoveRight_ = false;
    bool wantJump_ = false;
    bool wantClimb_ = false;
    bool wantDuck_ = false;
    bool wantInteract_ = false;
    bool wantRun_ = false;

    // Jump state
    bool jumping_ = false;
    float jumpTimer_ = 0.0f;
    float maxJumpTime_ = 0.2f;  // Variable height jump

    // Animation
    int currentAnimation_ = 0;
    float animTimer_ = 0.0f;

    // Invincibility (after being hurt)
    float invincibleTimer_ = 0.0f;
    float invincibleDuration_ = 2.0f;

    // Game progress
    int lives_ = 3;
    int score_ = 0;
    std::vector<CollectedPart> collectedParts_;

    // Puzzles
    int currentPuzzleId_ = -1;

    // Audio
    AudioSystem* audio_ = nullptr;
};

} // namespace opengg
