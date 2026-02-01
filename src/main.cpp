#include "game_loop.h"
#include "renderer.h"
#include "audio.h"
#include "input.h"
#include "asset_cache.h"
#include <SDL.h>
#include <iostream>
#include <string>
#include <memory>

using namespace opengg;

// Forward declarations for game states
class TitleState;
class MainMenuState;
class GameplayState;

// Title screen state
class TitleState : public GameState {
public:
    explicit TitleState(Game* game) : game_(game) {}

    void enter() override {
        timer_ = 0.0f;
        fadeProgress_ = 0.0f;
    }

    void exit() override {}

    void update(float dt) override {
        timer_ += dt;

        // Fade in
        if (fadeProgress_ < 1.0f) {
            fadeProgress_ = std::min(1.0f, fadeProgress_ + dt * 2.0f);
        }

        // Auto-advance after a few seconds or on any input
        if (timer_ > 5.0f) {
            advanceToMenu();
        }
    }

    void render() override {
        Renderer* renderer = game_->getRenderer();

        // Clear to black
        renderer->clear(Color(0, 0, 0));

        // Draw title text (placeholder until we load actual graphics)
        int centerX = Renderer::GAME_WIDTH / 2;
        int centerY = Renderer::GAME_HEIGHT / 2;

        // Draw placeholder title
        renderer->fillRect(Rect(centerX - 200, centerY - 50, 400, 100),
                          Color(50, 50, 150, static_cast<uint8_t>(fadeProgress_ * 255)));

        // Fade effect
        renderer->fadeIn(fadeProgress_);
    }

    void handleInput() override {
        InputSystem* input = game_->getInput();

        if (input->isActionPressed(GameAction::Action) ||
            input->isActionPressed(GameAction::MenuSelect) ||
            input->isMouseButtonPressed(MouseButton::Left)) {
            advanceToMenu();
        }

        if (input->isActionPressed(GameAction::Cancel)) {
            game_->quit();
        }
    }

private:
    void advanceToMenu();

    Game* game_;
    float timer_ = 0.0f;
    float fadeProgress_ = 0.0f;
};

// Main menu state
class MainMenuState : public GameState {
public:
    explicit MainMenuState(Game* game) : game_(game) {}

    void enter() override {
        selectedItem_ = 0;
    }

    void exit() override {}

    void update(float dt) override {
        // Animate menu items
        animTimer_ += dt;
    }

    void render() override {
        Renderer* renderer = game_->getRenderer();

        // Dark blue background
        renderer->clear(Color(20, 20, 60));

        // Menu title
        int centerX = Renderer::GAME_WIDTH / 2;
        renderer->fillRect(Rect(centerX - 150, 50, 300, 40), Color(100, 100, 200));

        // Menu items
        const char* menuItems[] = {
            "New Game",
            "Continue",
            "Options",
            "Quit"
        };
        const int itemCount = 4;

        for (int i = 0; i < itemCount; ++i) {
            int y = 150 + i * 50;
            Color color = (i == selectedItem_) ? Color(255, 255, 100) : Color(150, 150, 150);

            // Highlight selected item
            if (i == selectedItem_) {
                float pulse = std::sin(animTimer_ * 5.0f) * 0.2f + 0.8f;
                color.a = static_cast<uint8_t>(pulse * 255);
                renderer->fillRect(Rect(centerX - 100, y - 5, 200, 30), Color(80, 80, 150));
            }

            renderer->fillRect(Rect(centerX - 80, y, 160, 20), color);
        }

        // Instructions
        renderer->fillRect(Rect(centerX - 150, 400, 300, 20), Color(100, 100, 100));
    }

    void handleInput() override {
        InputSystem* input = game_->getInput();

        if (input->isActionPressed(GameAction::MenuUp)) {
            selectedItem_ = (selectedItem_ - 1 + 4) % 4;
            game_->getAudio()->playSound("menu_move");
        }

        if (input->isActionPressed(GameAction::MenuDown)) {
            selectedItem_ = (selectedItem_ + 1) % 4;
            game_->getAudio()->playSound("menu_move");
        }

        if (input->isActionPressed(GameAction::MenuSelect) ||
            input->isActionPressed(GameAction::Action)) {
            selectItem();
        }

        if (input->isActionPressed(GameAction::Cancel) ||
            input->isActionPressed(GameAction::MenuBack)) {
            if (selectedItem_ != 3) {
                selectedItem_ = 3;  // Select Quit
            } else {
                game_->quit();
            }
        }
    }

private:
    void selectItem();

    Game* game_;
    int selectedItem_ = 0;
    float animTimer_ = 0.0f;
};

// Gameplay state (placeholder)
class GameplayState : public GameState {
public:
    explicit GameplayState(Game* game) : game_(game) {}

    void enter() override {
        SDL_Log("Entering gameplay...");
    }

    void exit() override {
        SDL_Log("Exiting gameplay...");
    }

    void update(float dt) override {
        // Update player
        if (game_->getInput()->isActionDown(GameAction::MoveLeft)) {
            playerX_ -= 200.0f * dt;
        }
        if (game_->getInput()->isActionDown(GameAction::MoveRight)) {
            playerX_ += 200.0f * dt;
        }
        if (game_->getInput()->isActionDown(GameAction::MoveUp)) {
            playerY_ -= 200.0f * dt;
        }
        if (game_->getInput()->isActionDown(GameAction::MoveDown)) {
            playerY_ += 200.0f * dt;
        }

        // Clamp to screen
        playerX_ = std::max(16.0f, std::min(playerX_, 640.0f - 16.0f));
        playerY_ = std::max(16.0f, std::min(playerY_, 480.0f - 16.0f));
    }

    void render() override {
        Renderer* renderer = game_->getRenderer();

        // Draw background (placeholder)
        renderer->clear(Color(100, 150, 100));

        // Draw some placeholder "room" elements
        renderer->fillRect(Rect(0, 400, 640, 80), Color(80, 60, 40));  // Floor
        renderer->fillRect(Rect(100, 300, 20, 100), Color(60, 40, 30)); // Platform
        renderer->fillRect(Rect(300, 250, 20, 150), Color(60, 40, 30)); // Platform
        renderer->fillRect(Rect(500, 350, 20, 50), Color(60, 40, 30));  // Platform

        // Draw player (placeholder - red square)
        int px = static_cast<int>(playerX_) - 16;
        int py = static_cast<int>(playerY_) - 16;
        renderer->fillRect(Rect(px, py, 32, 32), Color(200, 50, 50));

        // Draw HUD
        renderer->fillRect(Rect(10, 10, 100, 20), Color(50, 50, 50, 180));
        renderer->fillRect(Rect(550, 10, 80, 20), Color(50, 50, 50, 180));

        // Show FPS in debug mode
        #ifdef _DEBUG
        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", game_->getFPS());
        // Would draw text here if we had font loaded
        #endif
    }

    void handleInput() override {
        InputSystem* input = game_->getInput();

        if (input->isActionPressed(GameAction::Pause) ||
            input->isActionPressed(GameAction::Cancel)) {
            // Return to menu
            game_->popState();
        }

        if (input->isActionPressed(GameAction::Screenshot)) {
            game_->getRenderer()->saveScreenshot("screenshot.bmp");
            SDL_Log("Screenshot saved!");
        }
    }

private:
    Game* game_;
    float playerX_ = 320.0f;
    float playerY_ = 350.0f;
};

// State transition implementations
void TitleState::advanceToMenu() {
    game_->changeState(std::make_unique<MainMenuState>(game_));
}

void MainMenuState::selectItem() {
    game_->getAudio()->playSound("menu_select");

    switch (selectedItem_) {
        case 0:  // New Game
            game_->changeState(std::make_unique<GameplayState>(game_));
            break;
        case 1:  // Continue
            // TODO: Load save game
            game_->changeState(std::make_unique<GameplayState>(game_));
            break;
        case 2:  // Options
            // TODO: Options menu
            break;
        case 3:  // Quit
            game_->quit();
            break;
    }
}

// Main function
int main(int argc, char* argv[]) {
    // Parse command line arguments
    GameConfig config;
    config.windowTitle = "OpenGizmos - Super Solvers: Gizmos & Gadgets";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-f" || arg == "--fullscreen") {
            config.fullscreen = true;
        } else if (arg == "-w" || arg == "--windowed") {
            config.fullscreen = false;
        } else if ((arg == "-p" || arg == "--path") && i + 1 < argc) {
            config.gamePath = argv[++i];
        } else if ((arg == "-s" || arg == "--scale") && i + 1 < argc) {
            int scale = std::atoi(argv[++i]);
            if (scale >= 1 && scale <= 8) {
                config.windowWidth = Renderer::GAME_WIDTH * scale;
                config.windowHeight = Renderer::GAME_HEIGHT * scale;
            }
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "OpenGizmos - Super Solvers: Gizmos & Gadgets Reimplementation\n\n";
            std::cout << "Usage: opengg [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -f, --fullscreen    Start in fullscreen mode\n";
            std::cout << "  -w, --windowed      Start in windowed mode\n";
            std::cout << "  -p, --path <dir>    Path to original game installation\n";
            std::cout << "  -s, --scale <n>     Window scale factor (1-8)\n";
            std::cout << "  -h, --help          Show this help message\n";
            return 0;
        }
    }

    // Create and initialize game
    Game game;

    if (!game.initialize(config)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
            "Failed to initialize game. Check that you have a valid copy of\n"
            "Super Solvers: Gizmos & Gadgets installed.", nullptr);
        return 1;
    }

    // Start with title screen
    game.pushState(std::make_unique<TitleState>(&game));

    // Run main loop
    game.run();

    // Cleanup
    game.shutdown();

    return 0;
}
