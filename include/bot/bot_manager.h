#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

// Forward declarations
namespace opengg {
    class Game;
    class InputSystem;
    class Room;
    class Player;
}

namespace Bot {

// Supported game types for bots
enum class GameType {
    Unknown = 0,
    GizmosAndGadgets,   // SSG
    OperationNeptune,   // ON
    OutNumbered,        // SSO
    Spellbound,         // SSR
    TreasureMountain,   // TM
    TreasureMathStorm,  // TMS
    TreasureCove        // TC
};

// Bot behavior modes
enum class BotMode {
    Disabled,           // Bot inactive
    Observe,            // Watch and analyze, don't act
    Assist,             // Help player with hints
    AutoPlay,           // Full autonomous play
    SpeedRun            // Optimized for fastest completion
};

// Bot decision types
enum class BotDecision {
    None,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    Jump,
    Climb,
    Interact,
    UsePart,
    EnterDoor,
    SolvePuzzle,
    AnswerQuestion,
    Wait
};

// Bot state information
struct BotState {
    bool isEnabled = false;
    BotMode mode = BotMode::Disabled;
    GameType gameType = GameType::Unknown;

    // Performance stats
    int puzzlesSolved = 0;
    int questionsAnswered = 0;
    int partsCollected = 0;
    float playTimeSeconds = 0.0f;
    int deaths = 0;

    // Current objective
    std::string currentObjective;
    float objectiveProgress = 0.0f;
};

// Base class for game-specific bot AI
class GameBot {
public:
    virtual ~GameBot() = default;

    // Core bot interface
    virtual void initialize(opengg::Game* game) = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;

    // Decision making
    virtual BotDecision getNextDecision() = 0;
    virtual void executeDecision(BotDecision decision, opengg::InputSystem* input) = 0;

    // State analysis
    virtual void analyzeGameState() = 0;
    virtual void onRoomChanged(opengg::Room* newRoom) = 0;
    virtual void onPuzzleStarted(int puzzleType) = 0;
    virtual void onPuzzleEnded(bool success) = 0;

    // Info
    virtual GameType getGameType() const = 0;
    virtual std::string getStatusText() const = 0;
    virtual float getCompletionProgress() const = 0;

    // Mode
    void setMode(BotMode mode) { mode_ = mode; }
    BotMode getMode() const { return mode_; }

protected:
    BotMode mode_ = BotMode::Disabled;
    opengg::Game* game_ = nullptr;
    float decisionCooldown_ = 0.0f;
    static constexpr float MIN_DECISION_INTERVAL = 0.1f; // 100ms between decisions
};

// Bot Manager - coordinates all bot functionality
class BotManager {
public:
    static BotManager& getInstance();

    // Lifecycle
    void initialize(opengg::Game* game);
    void shutdown();
    void update(float deltaTime);

    // Bot control
    void setEnabled(bool enabled);
    bool isEnabled() const { return state_.isEnabled; }

    void setMode(BotMode mode);
    BotMode getMode() const { return state_.mode; }

    void setGameType(GameType type);
    GameType getGameType() const { return state_.gameType; }

    // Get current bot
    GameBot* getCurrentBot() { return currentBot_.get(); }
    const BotState& getState() const { return state_; }

    // Input injection - bot decisions become input
    void injectInput(opengg::InputSystem* input);
    void executeDecision(opengg::InputSystem* input) { injectInput(input); }

    // Convenience methods for status
    std::string getStatusText() const;
    float getCompletionProgress() const;

    // Event callbacks
    void onRoomChanged(opengg::Room* newRoom);
    void onPuzzleStarted(int puzzleType);
    void onPuzzleEnded(bool success);
    void onPlayerDied();
    void onPartCollected(int partType);

    // Debug info
    std::string getDebugInfo() const;
    void renderDebugOverlay();

    // Callbacks for UI
    using StatusCallback = std::function<void(const std::string&)>;
    void setStatusCallback(StatusCallback cb) { statusCallback_ = cb; }

private:
    BotManager() = default;
    ~BotManager() = default;
    BotManager(const BotManager&) = delete;
    BotManager& operator=(const BotManager&) = delete;

    void createBotForGameType(GameType type);

    opengg::Game* game_ = nullptr;
    std::unique_ptr<GameBot> currentBot_;
    BotState state_;
    StatusCallback statusCallback_;

    // Decision history for analysis
    std::vector<BotDecision> recentDecisions_;
    static constexpr size_t MAX_DECISION_HISTORY = 100;
};

// Helper to convert game type to string
inline const char* gameTypeToString(GameType type) {
    switch (type) {
        case GameType::GizmosAndGadgets: return "Gizmos & Gadgets";
        case GameType::OperationNeptune: return "Operation Neptune";
        case GameType::OutNumbered: return "OutNumbered!";
        case GameType::Spellbound: return "Spellbound!";
        case GameType::TreasureMountain: return "Treasure Mountain!";
        case GameType::TreasureMathStorm: return "Treasure MathStorm!";
        case GameType::TreasureCove: return "Treasure Cove!";
        default: return "Unknown";
    }
}

inline const char* botModeToString(BotMode mode) {
    switch (mode) {
        case BotMode::Disabled: return "Disabled";
        case BotMode::Observe: return "Observe";
        case BotMode::Assist: return "Assist";
        case BotMode::AutoPlay: return "Auto-Play";
        case BotMode::SpeedRun: return "Speed Run";
        default: return "Unknown";
    }
}

} // namespace Bot
