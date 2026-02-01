#pragma once

#include "entity.h"
#include "formats/dat_format.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

struct SDL_Texture;

namespace opengg {

class Renderer;
class AssetCache;
class Player;

// Room layer for rendering order
enum class RoomLayer {
    Background,
    BackDecor,
    Platforms,
    Entities,
    FrontDecor,
    Foreground,
    Count
};

// Room class - represents a single room/screen in the game
class Room {
public:
    Room();
    ~Room();

    // Load room from game data
    bool load(int roomId, AssetCache* assetCache);

    // Create empty room for testing
    void createEmpty(int width, int height);

    // Lifecycle
    void enter();
    void exit();
    void update(float dt, Player* player);
    void render(Renderer* renderer);

    // Room properties
    int getId() const { return id_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    int getStartX() const { return startX_; }
    int getStartY() const { return startY_; }

    // Background
    void setBackground(SDL_Texture* texture);
    SDL_Texture* getBackground() const { return background_; }

    // Entity management
    void addEntity(std::unique_ptr<Entity> entity);
    void removeEntity(Entity* entity);
    Entity* findEntity(int id);
    std::vector<Entity*> getEntities() const;
    std::vector<Entity*> getEntitiesOfType(EntityType type) const;

    // Collision detection
    bool isSolidAt(float x, float y) const;
    bool isLadderAt(float x, float y) const;
    Entity* getEntityAt(float x, float y) const;
    std::vector<Entity*> getCollidingEntities(const Entity& entity) const;

    // Tile-based collision (for solid floor/walls)
    void setTile(int x, int y, int tileId);
    int getTile(int x, int y) const;
    bool isTileSolid(int tileId) const;

    // Doors
    DoorEntity* getDoorAt(float x, float y) const;
    std::vector<DoorEntity*> getDoors() const;

    // Parts
    std::vector<PartEntity*> getParts() const;
    int getPartCount() const;
    int getCollectedPartCount() const;

    // Triggers
    void checkTriggers(Entity* entity);

    // Music
    int getMusicId() const { return musicId_; }
    void setMusicId(int id) { musicId_ = id; }

    // Callbacks
    using RoomCallback = std::function<void(Room*)>;
    void setEnterCallback(RoomCallback cb) { enterCallback_ = std::move(cb); }
    void setExitCallback(RoomCallback cb) { exitCallback_ = std::move(cb); }

private:
    void updateEntities(float dt);
    void renderLayer(Renderer* renderer, RoomLayer layer);
    void checkEntityCollisions(Entity* entity);

    int id_ = 0;
    int width_ = 640;
    int height_ = 480;
    int startX_ = 100;
    int startY_ = 400;
    int musicId_ = 0;

    // Graphics
    SDL_Texture* background_ = nullptr;

    // Tile map (for collision)
    std::vector<int> tiles_;
    int tileWidth_ = 32;
    int tileHeight_ = 32;
    int tilesX_ = 0;
    int tilesY_ = 0;

    // Entities by layer
    std::vector<std::unique_ptr<Entity>> entities_;

    // Callbacks
    RoomCallback enterCallback_;
    RoomCallback exitCallback_;
};

// Area class - contains multiple rooms (one floor of a building)
class Area {
public:
    Area();
    ~Area();

    // Load area from game data
    bool load(int areaId, AssetCache* assetCache);

    // Properties
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    int getRoomCount() const { return static_cast<int>(rooms_.size()); }

    // Room access
    Room* getRoom(int index);
    Room* getCurrentRoom() { return currentRoom_; }
    void setCurrentRoom(int index);

    // Navigation
    void goToRoom(int roomId);
    void goToRoom(int roomId, int startX, int startY);

private:
    int id_ = 0;
    std::string name_;
    std::vector<std::unique_ptr<Room>> rooms_;
    Room* currentRoom_ = nullptr;
};

// Building class - contains multiple areas (floors)
class Building {
public:
    Building();
    ~Building();

    // Load building from game data
    bool load(int buildingId, AssetCache* assetCache);

    // Properties
    int getId() const { return id_; }
    opengg::Building getDifficulty() const { return difficulty_; }
    int getAreaCount() const { return static_cast<int>(areas_.size()); }

    // Area access
    Area* getArea(int floor);
    Area* getCurrentArea() { return currentArea_; }
    void setCurrentArea(int floor);

    // Navigation
    void goToFloor(int floor);
    void goToFloor(int floor, int roomId);

private:
    int id_ = 0;
    opengg::Building difficulty_ = opengg::Building::Easy;
    std::vector<std::unique_ptr<Area>> areas_;
    Area* currentArea_ = nullptr;
};

} // namespace opengg
