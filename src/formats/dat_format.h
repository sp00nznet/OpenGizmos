#pragma once

#include <cstdint>

namespace opengg {

#pragma pack(push, 1)

// Room structure (from reverssg decompilation)
struct RoomData {
    uint16_t flags;
    int16_t  structuralEntityCounts[4];
    int16_t  ladderEntityCounts[4];
    int16_t  doorCount;
    int16_t  partCount;
    int16_t  obstacleCount;
    int16_t  triggerCount;
    uint16_t backgroundId;
    uint16_t musicId;
    int16_t  width;
    int16_t  height;
    int16_t  startX;
    int16_t  startY;
};

// Area structure (contains multiple rooms)
struct AreaData {
    int16_t  roomCount;
    uint16_t areaId;
    uint16_t flags;
    char     name[32];
};

// Actor/Entity structure (168 bytes total in original)
struct ActorData {
    int16_t  spriteId;
    int16_t  animationId;
    int32_t  positionX;      // Fixed-point position
    int32_t  positionY;
    int32_t  velocityX;      // Fixed-point velocity
    int32_t  velocityY;
    int16_t  state;
    int16_t  health;
    int16_t  direction;      // 0=left, 1=right
    uint16_t flags;
    int16_t  aiType;
    int16_t  aiState;
    int16_t  aiTimer;
    int16_t  collisionWidth;
    int16_t  collisionHeight;
    int16_t  animFrame;
    int16_t  animTimer;
    uint8_t  reserved[128];  // Padding to 168 bytes
};

// Part (collectible item)
struct PartData {
    int16_t  partType;       // Which vehicle part
    int16_t  category;       // Part category
    int16_t  spriteId;
    int16_t  positionX;
    int16_t  positionY;
    uint16_t flags;
    int16_t  puzzleId;       // Associated puzzle (if any)
};

// Vehicle definition
struct VehicleData {
    int16_t  vehicleType;    // Airplane, Car, Alternative
    int16_t  spriteId;
    int16_t  requiredParts[8]; // Part IDs needed to build
    int16_t  partCount;
    uint16_t flags;
    int16_t  speed;
    int16_t  handling;
};

// Puzzle definition
struct PuzzleData {
    int16_t  puzzleType;     // Balance, Electricity, etc.
    int16_t  difficulty;     // 1-3 (easy/medium/hard buildings)
    int16_t  spriteSetId;
    int16_t  backgroundId;
    uint16_t flags;
    int16_t  timeLimit;      // In seconds, 0 = no limit
    int16_t  targetValue;    // Goal for the puzzle
};

// Save game header
struct SaveHeader {
    char     magic[4];       // "OGSG" - OpenGizmos Save Game
    uint16_t version;
    uint16_t checksum;
    uint32_t timestamp;
    char     playerName[32];
};

// Save game data
struct SaveData {
    SaveHeader header;
    int16_t  currentBuilding; // 0-2
    int16_t  currentLevel;    // 0-4
    int16_t  currentRoom;
    int32_t  playerX;
    int32_t  playerY;
    uint8_t  partsCollected[64]; // Bitfield
    uint8_t  puzzlesSolved[64];  // Bitfield
    int16_t  vehicleProgress[3]; // Parts collected per vehicle
    uint32_t score;
    uint32_t playTime;       // In seconds
};

#pragma pack(pop)

// Puzzle types
enum class PuzzleType : int16_t {
    Balance       = 0,
    Electricity   = 1,
    Energy        = 2,
    Force         = 3,
    Gear          = 4,
    Jigsaw        = 5,
    Magnet        = 6,
    SimpleMachine = 7
};

// Building IDs
enum class Building : int16_t {
    Easy   = 0,  // Green/Easy
    Medium = 1,  // Yellow/Medium
    Hard   = 2   // Red/Hard
};

// Entity types
enum class EntityType : int16_t {
    Player    = 0,
    Morty     = 1,
    Part      = 2,
    Obstacle  = 3,
    Door      = 4,
    Ladder    = 5,
    Platform  = 6,
    Trigger   = 7,
    NPC       = 8
};

// Actor states
enum class ActorState : int16_t {
    Idle      = 0,
    Walking   = 1,
    Climbing  = 2,
    Falling   = 3,
    Jumping   = 4,
    Puzzle    = 5,
    Building  = 6,
    Racing    = 7,
    Dead      = 8
};

} // namespace opengg
