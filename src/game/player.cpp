#include "player.h"
#include "room.h"
#include "input.h"
#include "audio.h"
#include "renderer.h"
#include <cmath>
#include <algorithm>

namespace opengg {

Player::Player() {
    type_ = EntityType::Player;
    setSize(24, 48);  // Player collision box
    addFlags(EntityFlags::Solid);
}

Player::~Player() = default;

void Player::init() {
    Entity::init();
    state_ = PlayerState::Idle;
    direction_ = Direction::Right;
    lives_ = 3;
    score_ = 0;
    collectedParts_.clear();
}

void Player::handleInput(InputSystem* input) {
    if (!input) return;

    // Don't handle movement input during certain states
    if (state_ == PlayerState::Puzzle ||
        state_ == PlayerState::Building ||
        state_ == PlayerState::Racing ||
        state_ == PlayerState::Dead) {
        return;
    }

    // Read input state
    wantMoveLeft_ = input->isActionDown(GameAction::MoveLeft);
    wantMoveRight_ = input->isActionDown(GameAction::MoveRight);
    wantJump_ = input->isActionPressed(GameAction::Jump);
    wantClimb_ = input->isActionDown(GameAction::MoveUp) || input->isActionDown(GameAction::MoveDown);
    wantDuck_ = input->isActionDown(GameAction::MoveDown) && !isClimbing();
    wantInteract_ = input->isActionPressed(GameAction::Action);
    wantRun_ = input->isActionDown(GameAction::Climb);  // Shift to run

    // Handle interaction
    if (wantInteract_) {
        interact();
    }
}

void Player::update(float dt) {
    prevState_ = state_;
    wasOnGround_ = onGround_;

    // Update invincibility
    if (invincibleTimer_ > 0) {
        invincibleTimer_ -= dt;
    }

    // Update based on current state
    switch (state_) {
        case PlayerState::Puzzle:
        case PlayerState::Building:
        case PlayerState::Racing:
        case PlayerState::Dead:
            // Don't update movement in these states
            break;

        default:
            updateMovement(dt);
            updatePhysics(dt);
            checkCollisions();
            break;
    }

    // Update animation
    updateAnimation();

    // Keep player in room bounds
    if (room_) {
        x_ = std::clamp(x_, 0.0f, static_cast<float>(room_->getWidth() - width_));
        y_ = std::clamp(y_, 0.0f, static_cast<float>(room_->getHeight() - height_));
    }
}

void Player::updateMovement(float dt) {
    float speed = wantRun_ ? runSpeed_ : walkSpeed_;

    // Horizontal movement
    if (isClimbing()) {
        // No horizontal movement while climbing (or limited)
        velX_ = 0;

        // Vertical climbing
        if (wantMoveLeft_ || wantMoveRight_) {
            // Allow jumping off ladder
            stopClimbing();
        } else if (room_ && room_->isLadderAt(x_ + width_ / 2, y_ + height_ / 2)) {
            if (wantClimb_) {
                // Check for up/down input
                // This is handled via the wantClimb_ flag
            }
        } else {
            // No longer on ladder
            stopClimbing();
        }
    } else {
        // Normal horizontal movement
        if (wantMoveLeft_ && !wantMoveRight_) {
            velX_ = -speed;
            direction_ = Direction::Left;
        } else if (wantMoveRight_ && !wantMoveLeft_) {
            velX_ = speed;
            direction_ = Direction::Right;
        } else {
            // Decelerate
            velX_ *= 0.8f;
            if (std::abs(velX_) < 1.0f) {
                velX_ = 0;
            }
        }
    }

    // Climbing
    if (isClimbing()) {
        if (wantClimb_) {
            // Determine climb direction based on up/down
            // We need to check actual up/down input here
            velY_ = -climbSpeed_;  // Climb up by default when holding up

            // Check if reached top or bottom of ladder
            if (room_) {
                if (!room_->isLadderAt(x_ + width_ / 2, y_ - 1)) {
                    // At top of ladder
                    y_ -= 10;  // Step off top
                    stopClimbing();
                }
            }
        } else {
            velY_ = 0;
            state_ = PlayerState::ClimbingIdle;
        }
    }

    // Jumping
    if (wantJump_ && (onGround_ || isClimbing())) {
        jump();
    }

    // Variable height jump - release to cut jump short
    if (jumping_ && velY_ < 0) {
        jumpTimer_ += dt;
        if (jumpTimer_ >= maxJumpTime_) {
            jumping_ = false;
        }
    }

    // Check for ladder to climb
    if (wantClimb_ && !isClimbing() && room_) {
        if (room_->isLadderAt(x_ + width_ / 2, y_ + height_)) {
            startClimbing();
        }
    }

    // Ducking
    if (wantDuck_ && onGround_ && !isClimbing()) {
        state_ = PlayerState::Ducking;
        // Reduce collision height?
    } else if (state_ == PlayerState::Ducking && !wantDuck_) {
        state_ = PlayerState::Idle;
    }
}

void Player::updatePhysics(float dt) {
    // Apply gravity (unless climbing)
    if (!isClimbing()) {
        applyGravity(dt);
    }

    // Apply velocity
    float newX = x_ + velX_ * dt;
    float newY = y_ + velY_ * dt;

    // Horizontal collision
    if (!checkWallCollision(velX_ * dt)) {
        x_ = newX;
    } else {
        velX_ = 0;
    }

    // Vertical collision
    y_ = newY;

    onGround_ = checkGroundCollision();
    if (onGround_) {
        if (velY_ > 0) {
            velY_ = 0;
        }
        if (state_ == PlayerState::Falling || state_ == PlayerState::Jumping) {
            state_ = PlayerState::Idle;
        }
    }

    if (checkCeilingCollision()) {
        velY_ = 0;
    }
}

void Player::applyGravity(float dt) {
    velY_ += gravity_ * dt;

    // Terminal velocity
    if (velY_ > 500.0f) {
        velY_ = 500.0f;
    }

    // Update state
    if (velY_ > 50.0f && !onGround_) {
        state_ = PlayerState::Falling;
    }
}

void Player::jump() {
    if (isClimbing()) {
        stopClimbing();
    }

    velY_ = -jumpForce_;
    onGround_ = false;
    jumping_ = true;
    jumpTimer_ = 0.0f;
    state_ = PlayerState::Jumping;

    if (audio_) {
        audio_->playSound("player_jump");
    }
}

void Player::startClimbing() {
    state_ = PlayerState::Climbing;
    velY_ = 0;
    onGround_ = false;
}

void Player::stopClimbing() {
    if (isClimbing()) {
        state_ = PlayerState::Falling;
    }
}

bool Player::checkGroundCollision() {
    if (!room_) return false;

    // Check slightly below feet
    float checkY = y_ + height_ + 1;

    for (float checkX = x_ + 2; checkX < x_ + width_ - 2; checkX += 8) {
        if (room_->isSolidAt(checkX, checkY)) {
            // Snap to ground
            int tileY = static_cast<int>(checkY) / 32 * 32;
            y_ = tileY - height_;
            return true;
        }
    }

    return false;
}

bool Player::checkCeilingCollision() {
    if (!room_) return false;

    float checkY = y_ - 1;

    for (float checkX = x_ + 2; checkX < x_ + width_ - 2; checkX += 8) {
        if (room_->isSolidAt(checkX, checkY)) {
            return true;
        }
    }

    return false;
}

bool Player::checkWallCollision(float dx) {
    if (!room_) return false;

    float checkX = (dx > 0) ? (x_ + width_ + 1) : (x_ - 1);

    for (float checkY = y_ + 2; checkY < y_ + height_ - 2; checkY += 8) {
        if (room_->isSolidAt(checkX, checkY)) {
            return true;
        }
    }

    return false;
}

void Player::checkCollisions() {
    if (!room_) return;

    // Get colliding entities
    auto collisions = room_->getCollidingEntities(*this);

    for (Entity* other : collisions) {
        onCollision(other);
    }
}

void Player::onCollision(Entity* other) {
    if (!other) return;

    switch (other->getType()) {
        case EntityType::Part: {
            auto* part = static_cast<PartEntity*>(other);
            if (!part->isCollected()) {
                onPartCollected(part);
            }
            break;
        }

        case EntityType::Door: {
            auto* door = static_cast<DoorEntity*>(other);
            if (wantInteract_ || (door->isOpen() && wantMoveRight_)) {
                onDoorEntered(door);
            }
            break;
        }

        case EntityType::Obstacle: {
            // Take damage if not invincible
            if (invincibleTimer_ <= 0) {
                loseLife();
            }
            break;
        }

        default:
            break;
    }
}

void Player::onPartCollected(PartEntity* part) {
    part->collect();
    collectPart(part->getPartType(), part->getCategory());

    if (audio_) {
        audio_->playSound("part_collect");
    }

    addScore(100);
}

void Player::onDoorEntered(DoorEntity* door) {
    // Room transition would be handled by game state
    door->open();

    if (audio_) {
        audio_->playSound("door_open");
    }
}

void Player::updateAnimation() {
    // Determine animation based on state
    int targetAnim = 0;

    switch (state_) {
        case PlayerState::Idle:
            targetAnim = 0;
            break;
        case PlayerState::Walking:
            targetAnim = 1;
            break;
        case PlayerState::Running:
            targetAnim = 2;
            break;
        case PlayerState::Jumping:
            targetAnim = 3;
            break;
        case PlayerState::Falling:
            targetAnim = 4;
            break;
        case PlayerState::Climbing:
            targetAnim = 5;
            break;
        case PlayerState::ClimbingIdle:
            targetAnim = 6;
            break;
        case PlayerState::Ducking:
            targetAnim = 7;
            break;
        default:
            targetAnim = 0;
            break;
    }

    // Update walking/running state based on velocity
    if (onGround_ && state_ != PlayerState::Ducking) {
        if (std::abs(velX_) > runSpeed_ * 0.8f) {
            state_ = PlayerState::Running;
        } else if (std::abs(velX_) > 10.0f) {
            state_ = PlayerState::Walking;
        } else {
            state_ = PlayerState::Idle;
        }
    }

    // Set animation
    if (currentAnimation_ != targetAnim) {
        currentAnimation_ = targetAnim;
        animTimer_ = 0.0f;
    }

    // Update flip based on direction
    if (direction_ == Direction::Left) {
        addFlags(EntityFlags::FlipH);
    } else {
        removeFlags(EntityFlags::FlipH);
    }
}

void Player::render(Renderer* renderer) {
    if (!isVisible()) return;

    // Flash when invincible
    if (invincibleTimer_ > 0) {
        if (static_cast<int>(invincibleTimer_ * 10) % 2 == 0) {
            return;  // Skip rendering every other frame
        }
    }

    // Draw player
    int drawX = static_cast<int>(x_);
    int drawY = static_cast<int>(y_);

    if (sprite_) {
        Entity::render(renderer);
    } else {
        // Placeholder - colored rectangle
        Color color(50, 150, 50);  // Green player

        if (state_ == PlayerState::Hurt) {
            color = Color(255, 100, 100);
        } else if (isClimbing()) {
            color = Color(100, 100, 200);
        }

        renderer->fillRect(Rect(drawX, drawY, width_, height_), color);

        // Draw face direction indicator
        int eyeX = drawX + (direction_ == Direction::Right ? width_ - 8 : 4);
        renderer->fillRect(Rect(eyeX, drawY + 8, 4, 4), Color(255, 255, 255));
    }
}

void Player::setState(PlayerState state) {
    if (state_ != state) {
        prevState_ = state_;
        state_ = state;
    }
}

void Player::setDirection(Direction dir) {
    direction_ = dir;
}

void Player::setRoom(Room* room) {
    Entity::setRoom(room);
}

void Player::enterRoom(Room* room, int x, int y) {
    setRoom(room);
    setPosition(static_cast<float>(x), static_cast<float>(y));
    velX_ = 0;
    velY_ = 0;
    state_ = PlayerState::Idle;
    onGround_ = false;
}

void Player::collectPart(int partType, int category) {
    CollectedPart part;
    part.partType = partType;
    part.category = category;
    part.used = false;
    collectedParts_.push_back(part);
}

bool Player::hasPart(int partType) const {
    for (const auto& part : collectedParts_) {
        if (part.partType == partType && !part.used) {
            return true;
        }
    }
    return false;
}

int Player::getPartCount(int category) const {
    int count = 0;
    for (const auto& part : collectedParts_) {
        if (part.category == category && !part.used) {
            count++;
        }
    }
    return count;
}

int Player::getTotalPartCount() const {
    int count = 0;
    for (const auto& part : collectedParts_) {
        if (!part.used) {
            count++;
        }
    }
    return count;
}

bool Player::canBuildVehicle(int vehicleType) const {
    // TODO: Check if player has all required parts for vehicle
    return getTotalPartCount() >= 8;  // Placeholder
}

void Player::usePart(int partType) {
    for (auto& part : collectedParts_) {
        if (part.partType == partType && !part.used) {
            part.used = true;
            return;
        }
    }
}

void Player::enterPuzzle(int puzzleId) {
    currentPuzzleId_ = puzzleId;
    state_ = PlayerState::Puzzle;
}

void Player::exitPuzzle(bool success) {
    state_ = PlayerState::Idle;

    if (success) {
        addScore(500);
        if (audio_) {
            audio_->playSound("puzzle_complete");
        }
    }

    currentPuzzleId_ = -1;
}

void Player::loseLife() {
    lives_--;
    state_ = PlayerState::Hurt;
    invincibleTimer_ = invincibleDuration_;

    // Knockback
    velY_ = -200.0f;
    velX_ = (direction_ == Direction::Right) ? -100.0f : 100.0f;

    if (audio_) {
        audio_->playSound("player_hurt");
    }

    if (lives_ <= 0) {
        state_ = PlayerState::Dead;
        if (audio_) {
            audio_->playSound("game_over");
        }
    }
}

void Player::gainLife() {
    lives_++;

    if (audio_) {
        audio_->playSound("extra_life");
    }
}

void Player::addScore(int points) {
    int oldScore = score_;
    score_ += points;

    // Check for extra life at score thresholds
    if ((oldScore / 10000) < (score_ / 10000)) {
        gainLife();
    }
}

void Player::interact() {
    if (!room_) return;

    // Check for nearby interactable entities
    float checkX = x_ + width_ / 2 + (direction_ == Direction::Right ? 20 : -20);
    float checkY = y_ + height_ / 2;

    // Check for doors
    auto* door = room_->getDoorAt(checkX, checkY);
    if (door) {
        if (!door->isLocked()) {
            door->open();
        } else if (audio_) {
            audio_->playSound("door_locked");
        }
    }

    // Check for other interactive entities
    Entity* entity = room_->getEntityAt(checkX, checkY);
    if (entity && hasFlag(entity->getFlags(), EntityFlags::Interactive)) {
        // Trigger interaction
        // This would be handled by specific entity types
    }
}

} // namespace opengg
