#include "bot/bot_manager.h"
#include "bot/neptune_bot.h"
#include "bot/gizmos_bot.h"
#include "bot/educational_bot.h"
#include "input.h"
#include "game_loop.h"
#include <SDL.h>
#include <sstream>
#include <iomanip>

namespace Bot {

BotManager& BotManager::getInstance() {
    static BotManager instance;
    return instance;
}

void BotManager::initialize(opengg::Game* game) {
    game_ = game;
    state_ = BotState();
    recentDecisions_.clear();

    SDL_Log("BotManager initialized");
}

void BotManager::shutdown() {
    if (currentBot_) {
        currentBot_->shutdown();
        currentBot_.reset();
    }

    game_ = nullptr;
    state_ = BotState();
    recentDecisions_.clear();

    SDL_Log("BotManager shutdown");
}

void BotManager::update(float deltaTime) {
    if (!state_.isEnabled || !currentBot_ || state_.mode == BotMode::Disabled) {
        return;
    }

    state_.playTimeSeconds += deltaTime;

    // Update the current bot
    currentBot_->update(deltaTime);

    // Get and store decision for history
    BotDecision decision = currentBot_->getNextDecision();
    if (decision != BotDecision::None) {
        recentDecisions_.push_back(decision);
        if (recentDecisions_.size() > MAX_DECISION_HISTORY) {
            recentDecisions_.erase(recentDecisions_.begin());
        }
    }

    // Update objective display
    state_.currentObjective = currentBot_->getStatusText();
    state_.objectiveProgress = currentBot_->getCompletionProgress();

    // Notify status callback
    if (statusCallback_) {
        statusCallback_(getDebugInfo());
    }
}

void BotManager::setEnabled(bool enabled) {
    if (state_.isEnabled == enabled) return;

    state_.isEnabled = enabled;

    if (enabled) {
        SDL_Log("Bot enabled - Mode: %s, Game: %s",
                botModeToString(state_.mode),
                gameTypeToString(state_.gameType));

        // Create bot if we have a game type
        if (state_.gameType != GameType::Unknown && !currentBot_) {
            createBotForGameType(state_.gameType);
        }
    } else {
        SDL_Log("Bot disabled");
    }
}

void BotManager::setMode(BotMode mode) {
    state_.mode = mode;

    if (currentBot_) {
        currentBot_->setMode(mode);
    }

    SDL_Log("Bot mode changed to: %s", botModeToString(mode));
}

void BotManager::setGameType(GameType type) {
    if (state_.gameType == type && currentBot_) {
        return;
    }

    state_.gameType = type;

    // Recreate bot for new game type
    if (state_.isEnabled) {
        createBotForGameType(type);
    }

    SDL_Log("Bot game type set to: %s", gameTypeToString(type));
}

void BotManager::createBotForGameType(GameType type) {
    // Shutdown existing bot
    if (currentBot_) {
        currentBot_->shutdown();
        currentBot_.reset();
    }

    // Create appropriate bot
    switch (type) {
        case GameType::OperationNeptune:
            currentBot_ = std::make_unique<NeptuneBot>();
            break;

        case GameType::GizmosAndGadgets:
            currentBot_ = std::make_unique<GizmosBot>();
            break;

        case GameType::OutNumbered:
        case GameType::Spellbound:
        case GameType::TreasureMountain:
        case GameType::TreasureMathStorm:
        case GameType::TreasureCove:
            currentBot_ = createEducationalBot(type);
            break;

        default:
            SDL_Log("Unknown game type, no bot created");
            return;
    }

    if (currentBot_ && game_) {
        currentBot_->initialize(game_);
        currentBot_->setMode(state_.mode);
        SDL_Log("Created bot for: %s", gameTypeToString(type));
    }
}

void BotManager::injectInput(opengg::InputSystem* input) {
    if (!state_.isEnabled || !currentBot_ || state_.mode == BotMode::Disabled) {
        return;
    }

    // Only inject input in AutoPlay or SpeedRun modes
    if (state_.mode != BotMode::AutoPlay && state_.mode != BotMode::SpeedRun) {
        return;
    }

    BotDecision decision = currentBot_->getNextDecision();
    if (decision != BotDecision::None) {
        currentBot_->executeDecision(decision, input);
    }
}

void BotManager::onRoomChanged(opengg::Room* newRoom) {
    if (currentBot_) {
        currentBot_->onRoomChanged(newRoom);
    }
}

void BotManager::onPuzzleStarted(int puzzleType) {
    if (currentBot_) {
        currentBot_->onPuzzleStarted(puzzleType);
    }
}

void BotManager::onPuzzleEnded(bool success) {
    if (currentBot_) {
        currentBot_->onPuzzleEnded(success);
    }

    if (success) {
        state_.puzzlesSolved++;
    }
}

void BotManager::onPlayerDied() {
    state_.deaths++;
}

void BotManager::onPartCollected(int partType) {
    state_.partsCollected++;
}

std::string BotManager::getStatusText() const {
    if (currentBot_) {
        return currentBot_->getStatusText();
    }
    return "Bot not active";
}

float BotManager::getCompletionProgress() const {
    if (currentBot_) {
        return currentBot_->getCompletionProgress();
    }
    return 0.0f;
}

std::string BotManager::getDebugInfo() const {
    std::ostringstream ss;

    ss << "=== Bot Status ===\n";
    ss << "Enabled: " << (state_.isEnabled ? "Yes" : "No") << "\n";
    ss << "Mode: " << botModeToString(state_.mode) << "\n";
    ss << "Game: " << gameTypeToString(state_.gameType) << "\n";
    ss << "\n";

    if (currentBot_) {
        ss << "Objective: " << state_.currentObjective << "\n";
        ss << "Progress: " << std::fixed << std::setprecision(1)
           << (state_.objectiveProgress * 100.0f) << "%\n";
        ss << "\n";
    }

    ss << "=== Statistics ===\n";
    ss << "Play Time: " << std::fixed << std::setprecision(1)
       << state_.playTimeSeconds << "s\n";
    ss << "Puzzles Solved: " << state_.puzzlesSolved << "\n";
    ss << "Parts Collected: " << state_.partsCollected << "\n";
    ss << "Deaths: " << state_.deaths << "\n";

    if (!recentDecisions_.empty()) {
        ss << "\n=== Recent Decisions ===\n";
        int count = std::min((int)recentDecisions_.size(), 10);
        for (int i = recentDecisions_.size() - count; i < (int)recentDecisions_.size(); i++) {
            ss << "  " << static_cast<int>(recentDecisions_[i]) << "\n";
        }
    }

    return ss.str();
}

void BotManager::renderDebugOverlay() {
    // This would render bot info as an overlay
    // Implementation depends on renderer access
}

} // namespace Bot
