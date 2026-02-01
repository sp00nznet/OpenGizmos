#include "entity.h"
#include "renderer.h"
#include <cmath>
#include <algorithm>

namespace opengg {

// Entity base class
Entity::Entity() = default;
Entity::~Entity() = default;

void Entity::init() {
    flags_ = EntityFlags::Active | EntityFlags::Visible;
}

void Entity::update(float dt) {
    // Apply velocity
    x_ += velX_ * dt;
    y_ += velY_ * dt;

    // Update animation
    if (animPlaying_ && animFrameCount_ > 1) {
        animTimer_ += dt;
        if (animTimer_ >= animFrameTime_) {
            animTimer_ -= animFrameTime_;
            animFrame_++;

            if (animFrame_ >= animFrameCount_) {
                if (animLooping_) {
                    animFrame_ = 0;
                } else {
                    animFrame_ = animFrameCount_ - 1;
                    animPlaying_ = false;
                }
            }
        }
    }
}

void Entity::render(Renderer* renderer) {
    if (!isVisible() || !sprite_) return;

    // Calculate sprite source rect based on animation frame
    int srcX = spriteX_ + (animFrame_ * spriteW_);
    int srcY = spriteY_;

    // Draw sprite
    bool flipH = hasFlag(flags_, EntityFlags::FlipH);
    bool flipV = hasFlag(flags_, EntityFlags::FlipV);

    if (flipH || flipV) {
        renderer->drawSpriteFlipped(sprite_,
                                    static_cast<int>(x_), static_cast<int>(y_),
                                    flipH, flipV);
    } else {
        renderer->drawSprite(sprite_,
                            static_cast<int>(x_), static_cast<int>(y_),
                            Rect(srcX, srcY, spriteW_, spriteH_));
    }
}

void Entity::destroy() {
    removeFlags(EntityFlags::Active);
    removeFlags(EntityFlags::Visible);
}

void Entity::setPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void Entity::move(float dx, float dy) {
    x_ += dx;
    y_ += dy;
}

void Entity::setVelocity(float vx, float vy) {
    velX_ = vx;
    velY_ = vy;
}

void Entity::setSize(int w, int h) {
    width_ = w;
    height_ = h;
}

bool Entity::overlaps(const Entity& other) const {
    return getRight() > other.getLeft() &&
           getLeft() < other.getRight() &&
           getBottom() > other.getTop() &&
           getTop() < other.getBottom();
}

bool Entity::overlapsPoint(float px, float py) const {
    return px >= getLeft() && px < getRight() &&
           py >= getTop() && py < getBottom();
}

void Entity::setSprite(SDL_Texture* texture) {
    sprite_ = texture;
}

void Entity::setSpriteRect(int x, int y, int w, int h) {
    spriteX_ = x;
    spriteY_ = y;
    spriteW_ = w;
    spriteH_ = h;
}

void Entity::setAnimation(int animId, int frameCount, float frameTime) {
    currentAnim_ = animId;
    animFrameCount_ = frameCount;
    animFrameTime_ = frameTime;
}

void Entity::playAnimation(int animId, bool loop) {
    if (currentAnim_ != animId) {
        currentAnim_ = animId;
        animFrame_ = 0;
        animTimer_ = 0.0f;
    }
    animLooping_ = loop;
    animPlaying_ = true;
}

void Entity::stopAnimation() {
    animPlaying_ = false;
}

// PartEntity
PartEntity::PartEntity() {
    type_ = EntityType::Part;
    addFlags(EntityFlags::Collectible);
    setSize(24, 24);
}

void PartEntity::init() {
    Entity::init();
    bobTimer_ = static_cast<float>(rand() % 1000) / 1000.0f * 3.14159f * 2.0f;
}

void PartEntity::update(float dt) {
    Entity::update(dt);

    if (!collected_) {
        // Floating bob animation
        bobTimer_ += dt * 3.0f;
        // Visual offset only, actual position unchanged
    }
}

void PartEntity::render(Renderer* renderer) {
    if (!isVisible() || collected_) return;

    // Calculate bob offset
    float bobOffset = std::sin(bobTimer_) * 3.0f;

    // Draw with bob offset
    int drawY = static_cast<int>(y_ + bobOffset);

    if (sprite_) {
        renderer->drawSprite(sprite_, static_cast<int>(x_), drawY,
                            Rect(spriteX_, spriteY_, spriteW_, spriteH_));
    } else {
        // Placeholder - yellow square
        renderer->fillRect(Rect(static_cast<int>(x_), drawY, width_, height_),
                          Color(255, 220, 50));
    }
}

void PartEntity::setPartInfo(int type, int category, int puzzleId) {
    partType_ = type;
    category_ = category;
    puzzleId_ = puzzleId;
}

void PartEntity::collect() {
    collected_ = true;
    removeFlags(EntityFlags::Active);
    removeFlags(EntityFlags::Visible);
}

// DoorEntity
DoorEntity::DoorEntity() {
    type_ = EntityType::Door;
    addFlags(EntityFlags::Interactive);
    setSize(32, 64);
}

void DoorEntity::init() {
    Entity::init();
}

void DoorEntity::update(float dt) {
    Entity::update(dt);

    // Animate door opening/closing
    if (open_ && animProgress_ < 1.0f) {
        animProgress_ = std::min(1.0f, animProgress_ + dt * 4.0f);
    } else if (!open_ && animProgress_ > 0.0f) {
        animProgress_ = std::max(0.0f, animProgress_ - dt * 4.0f);
    }
}

void DoorEntity::render(Renderer* renderer) {
    if (!isVisible()) return;

    // Draw door frame
    renderer->fillRect(Rect(static_cast<int>(x_) - 4, static_cast<int>(y_),
                           width_ + 8, height_), Color(60, 40, 30));

    // Draw door (slides up when opening)
    int doorHeight = static_cast<int>(height_ * (1.0f - animProgress_));
    if (doorHeight > 0) {
        renderer->fillRect(Rect(static_cast<int>(x_), static_cast<int>(y_) + height_ - doorHeight,
                               width_, doorHeight), Color(120, 80, 50));
    }
}

void DoorEntity::setTarget(int roomId, int x, int y) {
    targetRoom_ = roomId;
    targetX_ = x;
    targetY_ = y;
}

void DoorEntity::open() {
    if (!locked_) {
        open_ = true;
    }
}

void DoorEntity::close() {
    open_ = false;
}

// LadderEntity
LadderEntity::LadderEntity() {
    type_ = EntityType::Ladder;
    removeFlags(EntityFlags::Solid);
    setSize(24, 96);
}

void LadderEntity::init() {
    Entity::init();
}

void LadderEntity::render(Renderer* renderer) {
    if (!isVisible()) return;

    int x = static_cast<int>(x_);
    int y = static_cast<int>(y_);

    // Draw ladder rails
    renderer->fillRect(Rect(x, y, 4, height_), Color(100, 70, 40));
    renderer->fillRect(Rect(x + width_ - 4, y, 4, height_), Color(100, 70, 40));

    // Draw rungs
    int rungSpacing = 16;
    for (int ry = 8; ry < height_; ry += rungSpacing) {
        renderer->fillRect(Rect(x + 4, y + ry, width_ - 8, 4), Color(100, 70, 40));
    }
}

bool LadderEntity::canClimbAt(float px, float py) const {
    return px >= x_ && px < x_ + width_ &&
           py >= y_ && py < y_ + height_;
}

// PlatformEntity
PlatformEntity::PlatformEntity() {
    type_ = EntityType::Platform;
    addFlags(EntityFlags::Solid);
    setSize(64, 16);
}

void PlatformEntity::init() {
    Entity::init();
    startX_ = x_;
    startY_ = y_;
    endX_ = x_;
    endY_ = y_;
}

void PlatformEntity::update(float dt) {
    if (!moving_) return;

    // Calculate direction
    float targetX = forward_ ? endX_ : startX_;
    float targetY = forward_ ? endY_ : startY_;

    float dx = targetX - x_;
    float dy = targetY - y_;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < speed_ * dt) {
        // Reached target, reverse
        x_ = targetX;
        y_ = targetY;
        forward_ = !forward_;
    } else {
        // Move toward target
        float factor = speed_ * dt / dist;
        x_ += dx * factor;
        y_ += dy * factor;
    }
}

void PlatformEntity::render(Renderer* renderer) {
    if (!isVisible()) return;

    renderer->fillRect(Rect(static_cast<int>(x_), static_cast<int>(y_),
                           width_, height_), Color(80, 80, 80));
}

void PlatformEntity::setMovement(float sx, float sy, float ex, float ey, float spd) {
    startX_ = sx;
    startY_ = sy;
    endX_ = ex;
    endY_ = ey;
    speed_ = spd;
}

// ObstacleEntity
ObstacleEntity::ObstacleEntity() {
    type_ = EntityType::Obstacle;
    addFlags(EntityFlags::Solid);
    setSize(32, 32);
}

void ObstacleEntity::init() {
    Entity::init();
}

void ObstacleEntity::update(float dt) {
    Entity::update(dt);

    switch (behavior_) {
        case Behavior::Patrol: {
            float speed = 60.0f;
            if (patrolForward_) {
                x_ += speed * dt;
                if (x_ >= patrolMaxX_) {
                    x_ = patrolMaxX_;
                    patrolForward_ = false;
                    addFlags(EntityFlags::FlipH);
                }
            } else {
                x_ -= speed * dt;
                if (x_ <= patrolMinX_) {
                    x_ = patrolMinX_;
                    patrolForward_ = true;
                    removeFlags(EntityFlags::FlipH);
                }
            }
            break;
        }

        case Behavior::Falling:
            velY_ += 400.0f * dt;  // Gravity
            velY_ = std::min(velY_, 300.0f);  // Terminal velocity
            break;

        default:
            break;
    }
}

void ObstacleEntity::render(Renderer* renderer) {
    if (!isVisible()) return;

    if (sprite_) {
        Entity::render(renderer);
    } else {
        // Placeholder - red square
        renderer->fillRect(Rect(static_cast<int>(x_), static_cast<int>(y_),
                               width_, height_), Color(200, 50, 50));
    }
}

void ObstacleEntity::setPatrolPath(float minX, float maxX) {
    patrolMinX_ = minX;
    patrolMaxX_ = maxX;
    behavior_ = Behavior::Patrol;
}

// TriggerEntity
TriggerEntity::TriggerEntity() {
    type_ = EntityType::Trigger;
    removeFlags(EntityFlags::Visible);
    removeFlags(EntityFlags::Solid);
}

void TriggerEntity::init() {
    Entity::init();
    removeFlags(EntityFlags::Visible);
}

void TriggerEntity::update(float dt) {
    // Triggers don't move or animate
}

void TriggerEntity::checkTrigger(Entity* entity) {
    if (!isActive() || !entity) return;
    if (oneShot_ && triggered_) return;

    if (overlaps(*entity)) {
        triggered_ = true;
        if (callback_) {
            callback_(entity);
        }
    }
}

} // namespace opengg
