#include "room.h"
#include "player.h"
#include "renderer.h"
#include "asset_cache.h"
#include <algorithm>

namespace opengg {

// Room implementation
Room::Room() = default;
Room::~Room() = default;

bool Room::load(int roomId, AssetCache* assetCache) {
    id_ = roomId;

    // TODO: Load room data from original game files
    // For now, create a basic test room

    createEmpty(640, 480);

    return true;
}

void Room::createEmpty(int width, int height) {
    width_ = width;
    height_ = height;

    // Initialize tile map
    tileWidth_ = 32;
    tileHeight_ = 32;
    tilesX_ = width / tileWidth_;
    tilesY_ = height / tileHeight_;
    tiles_.resize(tilesX_ * tilesY_, 0);

    // Create floor (solid tiles at bottom)
    for (int x = 0; x < tilesX_; ++x) {
        setTile(x, tilesY_ - 1, 1);  // Solid floor
        setTile(x, tilesY_ - 2, 1);  // Second row of floor
    }

    // Add some platforms
    for (int x = 3; x < 6; ++x) {
        setTile(x, tilesY_ - 5, 1);
    }
    for (int x = 8; x < 12; ++x) {
        setTile(x, tilesY_ - 7, 1);
    }
    for (int x = 14; x < 17; ++x) {
        setTile(x, tilesY_ - 5, 1);
    }

    // Default start position
    startX_ = 100;
    startY_ = height - 100;
}

void Room::enter() {
    if (enterCallback_) {
        enterCallback_(this);
    }
}

void Room::exit() {
    if (exitCallback_) {
        exitCallback_(this);
    }
}

void Room::update(float dt, Player* player) {
    updateEntities(dt);

    // Check triggers against player
    if (player) {
        checkTriggers(player);
    }
}

void Room::updateEntities(float dt) {
    for (auto& entity : entities_) {
        if (entity->isActive()) {
            entity->update(dt);
        }
    }

    // Remove destroyed entities
    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(),
            [](const std::unique_ptr<Entity>& e) { return !e->isActive(); }),
        entities_.end()
    );
}

void Room::render(Renderer* renderer) {
    // Draw background
    if (background_) {
        renderer->drawSprite(background_, 0, 0);
    } else {
        // Default background color
        renderer->clear(Color(100, 150, 200));
    }

    // Draw tile map (floor/platforms)
    for (int y = 0; y < tilesY_; ++y) {
        for (int x = 0; x < tilesX_; ++x) {
            int tile = getTile(x, y);
            if (tile > 0) {
                // Solid tile
                Color color(80, 60, 40);  // Brown floor
                renderer->fillRect(Rect(x * tileWidth_, y * tileHeight_,
                                       tileWidth_, tileHeight_), color);
            }
        }
    }

    // Draw entities by layer
    for (int layer = 0; layer < static_cast<int>(RoomLayer::Count); ++layer) {
        renderLayer(renderer, static_cast<RoomLayer>(layer));
    }
}

void Room::renderLayer(Renderer* renderer, RoomLayer layer) {
    for (auto& entity : entities_) {
        if (!entity->isVisible()) continue;

        // Determine entity layer
        RoomLayer entityLayer = RoomLayer::Entities;
        switch (entity->getType()) {
            case EntityType::Ladder:
                entityLayer = RoomLayer::BackDecor;
                break;
            case EntityType::Door:
                entityLayer = RoomLayer::BackDecor;
                break;
            case EntityType::Platform:
                entityLayer = RoomLayer::Platforms;
                break;
            default:
                entityLayer = RoomLayer::Entities;
                break;
        }

        if (entityLayer == layer) {
            entity->render(renderer);
        }
    }
}

void Room::setBackground(SDL_Texture* texture) {
    background_ = texture;
}

void Room::addEntity(std::unique_ptr<Entity> entity) {
    entity->setRoom(this);
    entities_.push_back(std::move(entity));
}

void Room::removeEntity(Entity* entity) {
    if (entity) {
        entity->destroy();
    }
}

Entity* Room::findEntity(int id) {
    for (auto& entity : entities_) {
        if (entity->getId() == id) {
            return entity.get();
        }
    }
    return nullptr;
}

std::vector<Entity*> Room::getEntities() const {
    std::vector<Entity*> result;
    for (const auto& entity : entities_) {
        result.push_back(entity.get());
    }
    return result;
}

std::vector<Entity*> Room::getEntitiesOfType(EntityType type) const {
    std::vector<Entity*> result;
    for (const auto& entity : entities_) {
        if (entity->getType() == type) {
            result.push_back(entity.get());
        }
    }
    return result;
}

bool Room::isSolidAt(float x, float y) const {
    // Check tile map
    int tx = static_cast<int>(x) / tileWidth_;
    int ty = static_cast<int>(y) / tileHeight_;

    if (tx >= 0 && tx < tilesX_ && ty >= 0 && ty < tilesY_) {
        int tile = getTile(tx, ty);
        if (isTileSolid(tile)) {
            return true;
        }
    }

    // Check solid entities
    for (const auto& entity : entities_) {
        if (entity->isSolid() && entity->overlapsPoint(x, y)) {
            return true;
        }
    }

    return false;
}

bool Room::isLadderAt(float x, float y) const {
    for (const auto& entity : entities_) {
        if (entity->getType() == EntityType::Ladder) {
            auto* ladder = static_cast<LadderEntity*>(entity.get());
            if (ladder->canClimbAt(x, y)) {
                return true;
            }
        }
    }
    return false;
}

Entity* Room::getEntityAt(float x, float y) const {
    for (const auto& entity : entities_) {
        if (entity->isActive() && entity->overlapsPoint(x, y)) {
            return entity.get();
        }
    }
    return nullptr;
}

std::vector<Entity*> Room::getCollidingEntities(const Entity& entity) const {
    std::vector<Entity*> result;
    for (const auto& other : entities_) {
        if (other.get() != &entity && other->isActive() && entity.overlaps(*other)) {
            result.push_back(other.get());
        }
    }
    return result;
}

void Room::setTile(int x, int y, int tileId) {
    if (x >= 0 && x < tilesX_ && y >= 0 && y < tilesY_) {
        tiles_[y * tilesX_ + x] = tileId;
    }
}

int Room::getTile(int x, int y) const {
    if (x >= 0 && x < tilesX_ && y >= 0 && y < tilesY_) {
        return tiles_[y * tilesX_ + x];
    }
    return 0;
}

bool Room::isTileSolid(int tileId) const {
    return tileId > 0;
}

DoorEntity* Room::getDoorAt(float x, float y) const {
    for (const auto& entity : entities_) {
        if (entity->getType() == EntityType::Door && entity->overlapsPoint(x, y)) {
            return static_cast<DoorEntity*>(entity.get());
        }
    }
    return nullptr;
}

std::vector<DoorEntity*> Room::getDoors() const {
    std::vector<DoorEntity*> result;
    for (const auto& entity : entities_) {
        if (entity->getType() == EntityType::Door) {
            result.push_back(static_cast<DoorEntity*>(entity.get()));
        }
    }
    return result;
}

std::vector<PartEntity*> Room::getParts() const {
    std::vector<PartEntity*> result;
    for (const auto& entity : entities_) {
        if (entity->getType() == EntityType::Part) {
            result.push_back(static_cast<PartEntity*>(entity.get()));
        }
    }
    return result;
}

int Room::getPartCount() const {
    return static_cast<int>(getParts().size());
}

int Room::getCollectedPartCount() const {
    int count = 0;
    for (const auto& entity : entities_) {
        if (entity->getType() == EntityType::Part) {
            auto* part = static_cast<PartEntity*>(entity.get());
            if (part->isCollected()) {
                count++;
            }
        }
    }
    return count;
}

void Room::checkTriggers(Entity* entity) {
    for (auto& trigger : entities_) {
        if (trigger->getType() == EntityType::Trigger) {
            static_cast<TriggerEntity*>(trigger.get())->checkTrigger(entity);
        }
    }
}

// Area implementation
Area::Area() = default;
Area::~Area() = default;

bool Area::load(int areaId, AssetCache* assetCache) {
    id_ = areaId;

    // TODO: Load area data from game files
    // For now, create test rooms

    for (int i = 0; i < 5; ++i) {
        auto room = std::make_unique<Room>();
        room->createEmpty(640, 480);
        rooms_.push_back(std::move(room));
    }

    if (!rooms_.empty()) {
        currentRoom_ = rooms_[0].get();
    }

    return true;
}

Room* Area::getRoom(int index) {
    if (index >= 0 && index < static_cast<int>(rooms_.size())) {
        return rooms_[index].get();
    }
    return nullptr;
}

void Area::setCurrentRoom(int index) {
    if (currentRoom_) {
        currentRoom_->exit();
    }

    currentRoom_ = getRoom(index);

    if (currentRoom_) {
        currentRoom_->enter();
    }
}

void Area::goToRoom(int roomId) {
    for (size_t i = 0; i < rooms_.size(); ++i) {
        if (rooms_[i]->getId() == roomId) {
            setCurrentRoom(static_cast<int>(i));
            return;
        }
    }
}

void Area::goToRoom(int roomId, int startX, int startY) {
    goToRoom(roomId);
    // Position would be set on player, not stored here
}

// Building implementation
Building::Building() = default;
Building::~Building() = default;

bool Building::load(int buildingId, AssetCache* assetCache) {
    id_ = buildingId;
    difficulty_ = static_cast<opengg::Building>(buildingId);

    // TODO: Load building data from game files
    // Each building has 5 floors (areas)

    for (int floor = 0; floor < 5; ++floor) {
        auto area = std::make_unique<Area>();
        area->load(floor, assetCache);
        areas_.push_back(std::move(area));
    }

    if (!areas_.empty()) {
        currentArea_ = areas_[0].get();
    }

    return true;
}

Area* Building::getArea(int floor) {
    if (floor >= 0 && floor < static_cast<int>(areas_.size())) {
        return areas_[floor].get();
    }
    return nullptr;
}

void Building::setCurrentArea(int floor) {
    currentArea_ = getArea(floor);
}

void Building::goToFloor(int floor) {
    setCurrentArea(floor);
}

void Building::goToFloor(int floor, int roomId) {
    setCurrentArea(floor);
    if (currentArea_) {
        currentArea_->goToRoom(roomId);
    }
}

} // namespace opengg
