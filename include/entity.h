#pragma once

#include "formats/dat_format.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

struct SDL_Texture;

namespace opengg {

class Room;
class Renderer;
class AssetCache;

// Entity flags
enum class EntityFlags : uint16_t {
    None        = 0,
    Active      = 1 << 0,
    Visible     = 1 << 1,
    Solid       = 1 << 2,
    Collectible = 1 << 3,
    Interactive = 1 << 4,
    Animated    = 1 << 5,
    FlipH       = 1 << 6,
    FlipV       = 1 << 7,
};

inline EntityFlags operator|(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline EntityFlags operator&(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline bool hasFlag(EntityFlags flags, EntityFlags test) {
    return (static_cast<uint16_t>(flags) & static_cast<uint16_t>(test)) != 0;
}

// Base entity class
class Entity {
public:
    Entity();
    virtual ~Entity();

    // Lifecycle
    virtual void init();
    virtual void update(float dt);
    virtual void render(Renderer* renderer);
    virtual void destroy();

    // Position (fixed-point internally, float for interface)
    float getX() const { return x_; }
    float getY() const { return y_; }
    void setPosition(float x, float y);
    void move(float dx, float dy);

    // Velocity
    float getVelX() const { return velX_; }
    float getVelY() const { return velY_; }
    void setVelocity(float vx, float vy);

    // Collision box
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    void setSize(int w, int h);

    // Bounding box in world coordinates
    float getLeft() const { return x_; }
    float getRight() const { return x_ + width_; }
    float getTop() const { return y_; }
    float getBottom() const { return y_ + height_; }

    // Collision detection
    bool overlaps(const Entity& other) const;
    bool overlapsPoint(float px, float py) const;

    // Flags
    EntityFlags getFlags() const { return flags_; }
    void setFlags(EntityFlags flags) { flags_ = flags; }
    void addFlags(EntityFlags flags) { flags_ = flags_ | flags; }
    void removeFlags(EntityFlags flags) { flags_ = static_cast<EntityFlags>(static_cast<uint16_t>(flags_) & ~static_cast<uint16_t>(flags)); }
    bool isActive() const { return hasFlag(flags_, EntityFlags::Active); }
    bool isVisible() const { return hasFlag(flags_, EntityFlags::Visible); }
    bool isSolid() const { return hasFlag(flags_, EntityFlags::Solid); }

    // Type
    EntityType getType() const { return type_; }
    void setType(EntityType type) { type_ = type; }

    // Sprite
    void setSprite(SDL_Texture* texture);
    void setSpriteRect(int x, int y, int w, int h);

    // Animation
    void setAnimation(int animId, int frameCount, float frameTime);
    void playAnimation(int animId, bool loop = true);
    void stopAnimation();

    // ID
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    // Room reference
    Room* getRoom() const { return room_; }
    void setRoom(Room* room) { room_ = room; }

protected:
    // Position and movement
    float x_ = 0.0f;
    float y_ = 0.0f;
    float velX_ = 0.0f;
    float velY_ = 0.0f;

    // Collision
    int width_ = 32;
    int height_ = 32;

    // Identity
    int id_ = 0;
    EntityType type_ = EntityType::Player;
    EntityFlags flags_ = EntityFlags::Active | EntityFlags::Visible;

    // Sprite/animation
    SDL_Texture* sprite_ = nullptr;
    int spriteX_ = 0;
    int spriteY_ = 0;
    int spriteW_ = 0;
    int spriteH_ = 0;

    // Animation state
    int currentAnim_ = 0;
    int animFrameCount_ = 1;
    float animFrameTime_ = 0.1f;
    float animTimer_ = 0.0f;
    int animFrame_ = 0;
    bool animLooping_ = true;
    bool animPlaying_ = false;

    // Room reference
    Room* room_ = nullptr;
};

// Part entity (collectible)
class PartEntity : public Entity {
public:
    PartEntity();

    void init() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;

    // Part info
    int getPartType() const { return partType_; }
    int getCategory() const { return category_; }
    int getPuzzleId() const { return puzzleId_; }

    void setPartInfo(int type, int category, int puzzleId = -1);

    // Collection
    bool isCollected() const { return collected_; }
    void collect();

private:
    int partType_ = 0;
    int category_ = 0;
    int puzzleId_ = -1;
    bool collected_ = false;
    float bobTimer_ = 0.0f;  // Floating animation
};

// Door entity
class DoorEntity : public Entity {
public:
    DoorEntity();

    void init() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;

    // Door properties
    int getTargetRoom() const { return targetRoom_; }
    int getTargetX() const { return targetX_; }
    int getTargetY() const { return targetY_; }

    void setTarget(int roomId, int x, int y);

    // State
    bool isOpen() const { return open_; }
    bool isLocked() const { return locked_; }
    void setLocked(bool locked) { locked_ = locked; }
    void open();
    void close();

private:
    int targetRoom_ = 0;
    int targetX_ = 0;
    int targetY_ = 0;
    bool open_ = false;
    bool locked_ = false;
    float animProgress_ = 0.0f;
};

// Ladder entity
class LadderEntity : public Entity {
public:
    LadderEntity();

    void init() override;
    void render(Renderer* renderer) override;

    // Ladder bounds
    float getClimbTop() const { return y_; }
    float getClimbBottom() const { return y_ + height_; }

    bool canClimbAt(float x, float y) const;
};

// Platform entity (moving or static)
class PlatformEntity : public Entity {
public:
    PlatformEntity();

    void init() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;

    // Movement
    void setMovement(float startX, float startY, float endX, float endY, float speed);
    void setMoving(bool moving) { moving_ = moving; }

private:
    float startX_ = 0.0f;
    float startY_ = 0.0f;
    float endX_ = 0.0f;
    float endY_ = 0.0f;
    float speed_ = 50.0f;
    bool moving_ = false;
    bool forward_ = true;
};

// Obstacle entity
class ObstacleEntity : public Entity {
public:
    ObstacleEntity();

    void init() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;

    // Obstacle behavior
    enum class Behavior {
        Static,
        Patrol,
        Chase,
        Falling
    };

    void setBehavior(Behavior behavior) { behavior_ = behavior; }
    Behavior getBehavior() const { return behavior_; }

    void setPatrolPath(float minX, float maxX);

private:
    Behavior behavior_ = Behavior::Static;
    float patrolMinX_ = 0.0f;
    float patrolMaxX_ = 0.0f;
    bool patrolForward_ = true;
};

// Trigger entity (invisible interaction zone)
class TriggerEntity : public Entity {
public:
    using TriggerCallback = std::function<void(Entity* triggerer)>;

    TriggerEntity();

    void init() override;
    void update(float dt) override;

    // Trigger settings
    void setCallback(TriggerCallback callback) { callback_ = std::move(callback); }
    void setOneShot(bool oneShot) { oneShot_ = oneShot; }
    void reset() { triggered_ = false; }

    // Check trigger
    void checkTrigger(Entity* entity);

private:
    TriggerCallback callback_;
    bool oneShot_ = false;
    bool triggered_ = false;
};

} // namespace opengg
