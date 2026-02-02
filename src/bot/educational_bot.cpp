#include "bot/educational_bot.h"
#include "input.h"
#include "game_loop.h"
#include "room.h"
#include <SDL.h>
#include <algorithm>
#include <sstream>
#include <regex>
#include <cctype>

namespace Bot {

EducationalBot::EducationalBot(GameType gameType)
    : gameType_(gameType) {
}

EducationalBot::~EducationalBot() = default;

void EducationalBot::initialize(Game* game) {
    game_ = game;

    progress_ = GameProgress();
    navigation_ = NavigationState();
    knowledgeBase_.clear();
    wordDictionary_.clear();

    // Load educational content knowledge base
    loadKnowledgeBase();

    SDL_Log("Educational Bot initialized for: %s", gameTypeToString(gameType_));
}

void EducationalBot::shutdown() {
    knowledgeBase_.clear();
    wordDictionary_.clear();

    SDL_Log("Educational Bot shutdown");
}

void EducationalBot::update(float deltaTime) {
    if (mode_ == BotMode::Disabled) return;

    decisionCooldown_ -= deltaTime;

    // Update navigation state
    updateNavigation();

    // Check for stuck condition
    // ...

    analyzeGameState();
}

BotDecision EducationalBot::getNextDecision() {
    if (mode_ == BotMode::Disabled || decisionCooldown_ > 0) {
        return BotDecision::None;
    }

    decisionCooldown_ = MIN_DECISION_INTERVAL;

    // Route to game-specific handler
    switch (gameType_) {
        case GameType::OutNumbered:
            return handleOutNumbered();
        case GameType::Spellbound:
            return handleSpellbound();
        case GameType::TreasureMountain:
            return handleTreasureMountain();
        case GameType::TreasureMathStorm:
            return handleTreasureMathStorm();
        case GameType::TreasureCove:
            return handleTreasureCove();
        default:
            return BotDecision::Wait;
    }
}

void EducationalBot::executeDecision(BotDecision decision, InputSystem* input) {
    if (!input) return;

    switch (decision) {
        case BotDecision::MoveLeft:
            SDL_Log("Edu Bot: Move Left");
            break;
        case BotDecision::MoveRight:
            SDL_Log("Edu Bot: Move Right");
            break;
        case BotDecision::MoveUp:
            SDL_Log("Edu Bot: Move Up");
            break;
        case BotDecision::MoveDown:
            SDL_Log("Edu Bot: Move Down");
            break;
        case BotDecision::Jump:
            SDL_Log("Edu Bot: Jump");
            break;
        case BotDecision::Interact:
            SDL_Log("Edu Bot: Interact");
            break;
        case BotDecision::AnswerQuestion:
            SDL_Log("Edu Bot: Answer Question");
            break;
        default:
            break;
    }
}

void EducationalBot::analyzeGameState() {
    // Update progress based on game state
    progress_.completionPercent = getCompletionProgress() * 100.0f;
}

void EducationalBot::onRoomChanged(Room* newRoom) {
    SDL_Log("Educational Bot: Room changed");

    if (newRoom) {
        // navigation_.currentRoom = newRoom->getId();
        // navigation_.exploredRooms.push_back(navigation_.currentRoom);
    }

    navigation_.pathToTarget.clear();
    navigation_.pathIndex = 0;
}

void EducationalBot::onPuzzleStarted(int puzzleType) {
    SDL_Log("Educational Bot: Puzzle started - type %d", puzzleType);
    currentObjective_ = BotObjective::SolvePuzzle;
}

void EducationalBot::onPuzzleEnded(bool success) {
    SDL_Log("Educational Bot: Puzzle ended - %s", success ? "success" : "failure");

    totalAnswers_++;
    if (success) {
        correctAnswers_++;
    }
    accuracyRate_ = (float)correctAnswers_ / (float)totalAnswers_;

    currentObjective_ = BotObjective::Explore;
}

std::string EducationalBot::getStatusText() const {
    std::ostringstream ss;

    ss << gameTypeToString(gameType_) << " - ";

    switch (currentObjective_) {
        case BotObjective::Idle:
            ss << "Idle";
            break;
        case BotObjective::Explore:
            ss << "Exploring Level " << progress_.currentLevel;
            break;
        case BotObjective::SeekCollectible:
            ss << "Seeking treasure";
            break;
        case BotObjective::SolvePuzzle:
            ss << "Solving puzzle";
            break;
        case BotObjective::EvadeEnemy:
            ss << "Evading enemy";
            break;
        case BotObjective::EngageEnemy:
            ss << "Engaging enemy";
            break;
        case BotObjective::FinalChallenge:
            ss << "Final challenge!";
            break;
        default:
            ss << "Working...";
            break;
    }

    ss << " [Accuracy: " << (int)(accuracyRate_ * 100) << "%]";

    return ss.str();
}

float EducationalBot::getCompletionProgress() const {
    // Calculate based on collectibles/treasures found
    if (progress_.totalCollectibles == 0) {
        return (float)progress_.currentLevel / 4.0f; // Assume 4 levels
    }
    return (float)progress_.collectiblesFound / (float)progress_.totalCollectibles;
}

// Educational content solving

void EducationalBot::loadKnowledgeBase() {
    // Load basic math facts
    // Load common vocabulary
    // Load spelling patterns

    // Math knowledge
    knowledgeBase_.push_back({"math", "addition", "sum", 1, 0, 0});
    knowledgeBase_.push_back({"math", "subtraction", "difference", 1, 0, 0});
    knowledgeBase_.push_back({"math", "multiplication", "product", 2, 0, 0});
    knowledgeBase_.push_back({"math", "division", "quotient", 2, 0, 0});

    // Common words for spelling
    wordDictionary_["their"] = "belonging to them";
    wordDictionary_["there"] = "in that place";
    wordDictionary_["they're"] = "they are";
    // ... more words would be added
}

void EducationalBot::learnFromResult(const std::string& question, bool correct) {
    // Find and update knowledge entry
    for (auto& entry : knowledgeBase_) {
        if (entry.question == question) {
            entry.timesEncountered++;
            if (correct) entry.timesCorrect++;
            return;
        }
    }

    // New question, add to knowledge base
    KnowledgeEntry newEntry;
    newEntry.question = question;
    newEntry.timesEncountered = 1;
    newEntry.timesCorrect = correct ? 1 : 0;
    knowledgeBase_.push_back(newEntry);
}

int EducationalBot::solveMathProblem(const std::string& problem, MathProblemType type) {
    switch (type) {
        case MathProblemType::Addition:
        case MathProblemType::Subtraction:
        case MathProblemType::Multiplication:
        case MathProblemType::Division:
            return parseAndSolveMath(problem);

        case MathProblemType::Sequence:
            // Extract numbers and find pattern
            return 0;

        case MathProblemType::WordProblem:
            // Parse word problem and extract math operation
            return parseAndSolveMath(problem);

        default:
            return 0;
    }
}

int EducationalBot::parseAndSolveMath(const std::string& expression) {
    // Simple expression parser
    // Handles: a + b, a - b, a * b, a / b

    int a = 0, b = 0;
    char op = '+';

    std::istringstream ss(expression);
    ss >> a >> op >> b;

    switch (op) {
        case '+': return solveAddition(a, b);
        case '-': return solveSubtraction(a, b);
        case '*':
        case 'x':
        case 'X': return solveMultiplication(a, b);
        case '/': return solveDivision(a, b);
        default: return 0;
    }
}

int EducationalBot::solveAddition(int a, int b) { return a + b; }
int EducationalBot::solveSubtraction(int a, int b) { return a - b; }
int EducationalBot::solveMultiplication(int a, int b) { return a * b; }
int EducationalBot::solveDivision(int a, int b) { return b != 0 ? a / b : 0; }

int EducationalBot::solveSequence(const std::vector<int>& sequence) {
    if (sequence.size() < 2) return 0;

    // Try arithmetic sequence (constant difference)
    int diff = sequence[1] - sequence[0];
    bool isArithmetic = true;
    for (size_t i = 2; i < sequence.size(); i++) {
        if (sequence[i] - sequence[i-1] != diff) {
            isArithmetic = false;
            break;
        }
    }
    if (isArithmetic) {
        return sequence.back() + diff;
    }

    // Try geometric sequence (constant ratio)
    if (sequence[0] != 0) {
        int ratio = sequence[1] / sequence[0];
        bool isGeometric = true;
        for (size_t i = 2; i < sequence.size(); i++) {
            if (sequence[i-1] != 0 && sequence[i] / sequence[i-1] != ratio) {
                isGeometric = false;
                break;
            }
        }
        if (isGeometric) {
            return sequence.back() * ratio;
        }
    }

    return 0; // Unknown pattern
}

bool EducationalBot::compareFractions(int num1, int den1, int num2, int den2) {
    // Cross multiply to compare: num1/den1 vs num2/den2
    return (num1 * den2) > (num2 * den1);
}

std::string EducationalBot::solveWordProblem(const std::string& problem, WordProblemType type) {
    switch (type) {
        case WordProblemType::Spelling:
            return checkSpelling(problem) ? "correct" : "incorrect";

        case WordProblemType::Vocabulary:
            return findWordMeaning(problem);

        case WordProblemType::Analogies:
            return completeAnalogy(problem);

        case WordProblemType::RootWords:
            return findRootWord(problem);

        default:
            return "";
    }
}

bool EducationalBot::checkSpelling(const std::string& word) {
    // Check against dictionary
    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return wordDictionary_.find(lower) != wordDictionary_.end();
}

std::string EducationalBot::findWordMeaning(const std::string& word) {
    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto it = wordDictionary_.find(lower);
    if (it != wordDictionary_.end()) {
        return it->second;
    }
    return "unknown";
}

std::string EducationalBot::completeAnalogy(const std::string& pattern) {
    // Pattern like "hot is to cold as big is to ___"
    // Would need a relationship database
    return "small"; // Placeholder
}

std::string EducationalBot::findRootWord(const std::string& word) {
    // Strip common prefixes and suffixes
    std::string root = word;

    // Common suffixes
    std::vector<std::string> suffixes = {"ing", "ed", "er", "est", "ly", "ness", "ment", "tion"};
    for (const auto& suffix : suffixes) {
        if (root.length() > suffix.length() &&
            root.substr(root.length() - suffix.length()) == suffix) {
            root = root.substr(0, root.length() - suffix.length());
            break;
        }
    }

    // Common prefixes
    std::vector<std::string> prefixes = {"un", "re", "pre", "dis", "mis"};
    for (const auto& prefix : prefixes) {
        if (root.length() > prefix.length() &&
            root.substr(0, prefix.length()) == prefix) {
            root = root.substr(prefix.length());
            break;
        }
    }

    return root;
}

int EducationalBot::selectMultipleChoice(const std::string& question,
                                         const std::vector<std::string>& choices) {
    // Try to match question to known answers
    for (const auto& entry : knowledgeBase_) {
        if (question.find(entry.question) != std::string::npos) {
            // Found a match, look for answer in choices
            for (size_t i = 0; i < choices.size(); i++) {
                if (choices[i].find(entry.answer) != std::string::npos) {
                    return static_cast<int>(i);
                }
            }
        }
    }

    // No known answer, make educated guess
    // Often the longest answer or middle choice is correct
    int bestChoice = 0;
    size_t maxLen = 0;
    for (size_t i = 0; i < choices.size(); i++) {
        if (choices[i].length() > maxLen) {
            maxLen = choices[i].length();
            bestChoice = static_cast<int>(i);
        }
    }

    return bestChoice;
}

// Navigation

void EducationalBot::updateNavigation() {
    // Update player position from game state
    // Update path if needed
}

BotDecision EducationalBot::decideMovement() {
    if (navigation_.pathToTarget.empty()) {
        return BotDecision::Wait;
    }

    // Follow path
    // ...

    return BotDecision::MoveRight; // Default
}

BotDecision EducationalBot::decideExploration() {
    // Explore unexplored areas
    return BotDecision::MoveRight;
}

std::vector<int> EducationalBot::findPathToGoal() {
    // Pathfinding to current goal
    return {};
}

// Enemy handling

bool EducationalBot::isEnemyNearby() {
    // Check for enemies in current room
    return false;
}

bool EducationalBot::shouldEngageEnemy() {
    // Determine if we should fight or flee
    // Educational games usually require answering questions
    return true;
}

BotDecision EducationalBot::handleEnemy() {
    if (shouldEngageEnemy()) {
        currentObjective_ = BotObjective::EngageEnemy;
        return BotDecision::Interact;
    } else {
        currentObjective_ = BotObjective::EvadeEnemy;
        return BotDecision::MoveLeft; // Run away
    }
}

// Game-specific handlers

BotDecision EducationalBot::handleOutNumbered() {
    // OutNumbered: TV studio, math puzzles, Telly robot

    // Check for Telly (enemy)
    if (isEnemyNearby()) {
        return handleEnemy();
    }

    // Seek clues
    if (progress_.cluesFound < 10) { // Assume 10 clues needed
        currentObjective_ = BotObjective::SeekCollectible;
        return decideExploration();
    }

    // All clues found, go to final confrontation
    currentObjective_ = BotObjective::FinalChallenge;
    return decideMovement();
}

BotDecision EducationalBot::handleSpellbound() {
    // Spellbound: Haunted house, word/spelling puzzles, ghosts

    // Check for ghosts (enemies)
    if (isEnemyNearby()) {
        return handleEnemy();
    }

    // Collect spell ingredients
    if (progress_.spellIngredients < 5) { // Assume 5 ingredients
        currentObjective_ = BotObjective::SeekCollectible;
        return decideExploration();
    }

    // Cast rescue spell
    currentObjective_ = BotObjective::FinalChallenge;
    return BotDecision::Interact;
}

BotDecision EducationalBot::handleTreasureMountain() {
    // Treasure Mountain: Climb mountain, catch elves, collect treasures

    // Seek elves for riddles
    if (isEnemyNearby()) {
        // Elves give riddles, not threats
        return BotDecision::Interact;
    }

    // Climb to next level
    if (progress_.currentLevel < 4) {
        return BotDecision::MoveUp;
    }

    // At summit, collect crown jewels
    currentObjective_ = BotObjective::FinalChallenge;
    return BotDecision::Interact;
}

BotDecision EducationalBot::handleTreasureMathStorm() {
    // Treasure MathStorm: Snow mountain, math puzzles, Ice Queen

    // Similar to Treasure Mountain but with more math focus

    // Check resources (equipment)
    // Navigate mountain zones
    // Solve math challenges

    if (progress_.currentLevel < 4) {
        return decideExploration();
    }

    currentObjective_ = BotObjective::FinalChallenge;
    return BotDecision::Interact;
}

BotDecision EducationalBot::handleTreasureCove() {
    // Treasure Cove: Underwater, reading puzzles, find pearl

    // Similar structure to other Treasure games
    // Focus on reading comprehension

    if (progress_.treasuresCollected < progress_.totalCollectibles) {
        currentObjective_ = BotObjective::SeekCollectible;
        return decideExploration();
    }

    currentObjective_ = BotObjective::FinalChallenge;
    return BotDecision::Interact;
}

} // namespace Bot
