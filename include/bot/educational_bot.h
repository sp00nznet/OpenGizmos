#pragma once

#include "bot_manager.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace Bot {

// Knowledge base entry for educational content
struct KnowledgeEntry {
    std::string category;       // "math", "reading", "spelling", "vocabulary"
    std::string question;       // Question text or pattern
    std::string answer;         // Correct answer
    int difficulty = 1;         // 1-3
    int timesEncountered = 0;
    int timesCorrect = 0;
};

// Math problem types
enum class MathProblemType {
    Addition,
    Subtraction,
    Multiplication,
    Division,
    WordProblem,
    Sequence,
    Comparison,
    Fraction,
    Money,
    Time,
    Measurement
};

// Word problem types
enum class WordProblemType {
    Spelling,
    Vocabulary,
    Comprehension,
    Analogies,
    Prefixes,
    Suffixes,
    RootWords,
    Context
};

// Generic Educational Bot - handles OutNumbered, Spellbound, Treasure series
class EducationalBot : public GameBot {
public:
    explicit EducationalBot(GameType gameType);
    ~EducationalBot() override;

    // GameBot interface
    void initialize(Game* game) override;
    void shutdown() override;
    void update(float deltaTime) override;

    BotDecision getNextDecision() override;
    void executeDecision(BotDecision decision, InputSystem* input) override;

    void analyzeGameState() override;
    void onRoomChanged(Room* newRoom) override;
    void onPuzzleStarted(int puzzleType) override;
    void onPuzzleEnded(bool success) override;

    GameType getGameType() const override { return gameType_; }
    std::string getStatusText() const override;
    float getCompletionProgress() const override;

    // Educational content solving
    int solveMathProblem(const std::string& problem, MathProblemType type);
    std::string solveWordProblem(const std::string& problem, WordProblemType type);
    int selectMultipleChoice(const std::string& question,
                            const std::vector<std::string>& choices);

private:
    GameType gameType_;

    // Game-specific state
    struct GameProgress {
        int currentLevel = 1;
        int currentArea = 0;
        int collectiblesFound = 0;
        int totalCollectibles = 0;
        int enemiesDefeated = 0;
        float completionPercent = 0.0f;

        // Game-specific
        int cluesFound = 0;          // OutNumbered
        int spellIngredients = 0;    // Spellbound
        int treasuresCollected = 0;  // Treasure series
    };

    // Navigation
    struct NavigationState {
        float playerX = 0, playerY = 0;
        int currentRoom = 0;
        std::vector<int> exploredRooms;
        std::vector<int> pathToTarget;
        int pathIndex = 0;
    };

    // Educational content handling
    void loadKnowledgeBase();
    void learnFromResult(const std::string& question, bool correct);

    // Math solving
    int parseAndSolveMath(const std::string& expression);
    int solveAddition(int a, int b);
    int solveSubtraction(int a, int b);
    int solveMultiplication(int a, int b);
    int solveDivision(int a, int b);
    int solveSequence(const std::vector<int>& sequence);
    bool compareFractions(int num1, int den1, int num2, int den2);

    // Word solving
    bool checkSpelling(const std::string& word);
    std::string findWordMeaning(const std::string& word);
    std::string completeAnalogy(const std::string& pattern);
    std::string findRootWord(const std::string& word);

    // Navigation
    BotDecision decideMovement();
    BotDecision decideExploration();
    void updateNavigation();
    std::vector<int> findPathToGoal();

    // Enemy/hazard handling
    BotDecision handleEnemy();
    bool isEnemyNearby();
    bool shouldEngageEnemy();

    // Game-specific behaviors
    BotDecision handleOutNumbered();     // TV studio, Telly robot
    BotDecision handleSpellbound();      // Haunted house, ghosts
    BotDecision handleTreasureMountain(); // Mountain climbing, elves
    BotDecision handleTreasureMathStorm(); // Ice mountain, Ice Queen
    BotDecision handleTreasureCove();    // Underwater, sea creatures

    // State
    GameProgress progress_;
    NavigationState navigation_;
    std::vector<KnowledgeEntry> knowledgeBase_;
    std::unordered_map<std::string, std::string> wordDictionary_;

    // Current objective
    enum class BotObjective {
        Idle,
        Explore,
        SeekCollectible,
        SolvePuzzle,
        EvadeEnemy,
        EngageEnemy,
        ReturnToBase,
        FinalChallenge
    };
    BotObjective currentObjective_ = BotObjective::Idle;

    // Timers
    float decisionTimer_ = 0.0f;
    float stuckTimer_ = 0.0f;

    // Statistics
    int correctAnswers_ = 0;
    int totalAnswers_ = 0;
    float accuracyRate_ = 0.0f;
};

// Factory function to create the right educational bot
inline std::unique_ptr<EducationalBot> createEducationalBot(GameType type) {
    switch (type) {
        case GameType::OutNumbered:
        case GameType::Spellbound:
        case GameType::TreasureMountain:
        case GameType::TreasureMathStorm:
        case GameType::TreasureCove:
            return std::make_unique<EducationalBot>(type);
        default:
            return nullptr;
    }
}

} // namespace Bot
