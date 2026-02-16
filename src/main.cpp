#include "game_loop.h"
#include "renderer.h"
#include "audio.h"
#include "input.h"
#include "asset_cache.h"
#include "ne_resource.h"
#include "font.h"
#include "game_registry.h"
#include "neptune/neptune_game.h"
#include <SDL.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <cstdlib>
#include <cmath>
#include <algorithm>

using namespace opengg;

// Forward declarations for game states
class TitleState;
class GameSelectionState;
class GameLaunchState;
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
            advanceToGameSelect();
        }
    }

    void render() override {
        Renderer* renderer = game_->getRenderer();
        TextRenderer* text = game_->getTextRenderer();

        // Clear to dark blue gradient-ish
        renderer->clear(Color(10, 10, 40));

        int centerX = Renderer::GAME_WIDTH / 2;
        int centerY = Renderer::GAME_HEIGHT / 2;

        uint8_t alpha = static_cast<uint8_t>(fadeProgress_ * 255);

        // Draw title background
        renderer->fillRect(Rect(centerX - 220, centerY - 80, 440, 160),
                          Color(30, 30, 100, alpha));
        renderer->drawRect(Rect(centerX - 220, centerY - 80, 440, 160),
                          Color(100, 100, 200, alpha));

        // Draw title text
        if (text) {
            text->drawTextAligned(renderer, "OPENGG", centerX - 200, centerY - 60, 400, TextAlign::Center,
                                TextColor(255, 255, 100, alpha));

            text->drawTextAligned(renderer, "TLC Educational Game Launcher",
                                 centerX - 200, centerY - 30, 400, TextAlign::Center,
                                 TextColor(200, 200, 255, alpha));

            text->drawTextAligned(renderer, "Multi-Game Engine",
                                 centerX - 100, centerY + 10, 200, TextAlign::Center,
                                 TextColor(150, 150, 200, alpha));

            // Blinking "Press any key" text
            if (static_cast<int>(timer_ * 2) % 2 == 0) {
                text->drawTextAligned(renderer, "Press ENTER to start",
                                     centerX - 100, centerY + 60, 200, TextAlign::Center,
                                     TextColor(255, 255, 255, alpha));
            }
        }

        // Fade effect
        renderer->fadeIn(fadeProgress_);
    }

    void handleInput() override {
        InputSystem* input = game_->getInput();

        if (input->isActionPressed(GameAction::Action) ||
            input->isActionPressed(GameAction::MenuSelect) ||
            input->isMouseButtonPressed(MouseButton::Left)) {
            advanceToGameSelect();
        }

        if (input->isActionPressed(GameAction::Cancel)) {
            game_->quit();
        }
    }

private:
    void advanceToGameSelect();

    Game* game_;
    float timer_ = 0.0f;
    float fadeProgress_ = 0.0f;
};

// Game Selection state - shows grid of available games
class GameSelectionState : public GameState {
public:
    explicit GameSelectionState(Game* game) : game_(game) {}

    void enter() override {
        SDL_Log("Entering Game Selection...");
        selectedIndex_ = 0;
        scrollOffset_ = 0;

        // Get available games from registry
        GameRegistry* registry = game_->getGameRegistry();
        if (registry) {
            games_ = registry->getAvailableGames();
        }

        if (games_.empty()) {
            SDL_Log("No games found in registry!");
        } else {
            SDL_Log("Found %zu games", games_.size());
        }
    }

    void exit() override {}

    void update(float dt) override {
        animTimer_ += dt;
    }

    void render() override {
        Renderer* renderer = game_->getRenderer();
        TextRenderer* text = game_->getTextRenderer();

        renderer->clear(Color(15, 20, 35));

        if (!text) return;

        // Header bar
        renderer->fillRect(Rect(0, 0, 640, 40), Color(30, 40, 70));
        text->drawTextAligned(renderer, "SELECT A GAME", 0, 12, 640, TextAlign::Center,
                            TextColor(255, 255, 100));

        if (games_.empty()) {
            text->drawTextAligned(renderer, "No extracted games found.", 0, 200, 640, TextAlign::Center,
                                TextColor(200, 100, 100));
            text->drawTextAligned(renderer, "Place extracted game directories in 'extracted/'", 0, 230, 640, TextAlign::Center,
                                TextColor(150, 150, 180));
            return;
        }

        // Game grid: 2 columns, 4 rows visible
        int cols = 2;
        int cardW = 290;
        int cardH = 90;
        int padX = 15;
        int padY = 10;
        int startX = (640 - (cols * cardW + (cols - 1) * padX)) / 2;
        int startY = 50;
        int maxVisibleRows = 4;

        for (size_t i = 0; i < games_.size(); ++i) {
            int row = static_cast<int>(i) / cols - scrollOffset_;
            int col = static_cast<int>(i) % cols;

            if (row < 0 || row >= maxVisibleRows) continue;

            int x = startX + col * (cardW + padX);
            int y = startY + row * (cardH + padY);

            bool selected = (static_cast<int>(i) == selectedIndex_);

            // Card background
            Color bgColor = selected ? Color(50, 60, 100) : Color(25, 30, 50);
            Color borderColor = selected ? Color(100, 150, 255) : Color(50, 60, 80);

            renderer->fillRect(Rect(x, y, cardW, cardH), bgColor);
            renderer->drawRect(Rect(x, y, cardW, cardH), borderColor);

            // Selection indicator
            if (selected) {
                renderer->fillRect(Rect(x + 2, y + 2, 4, cardH - 4), Color(100, 200, 255));
            }

            // Game name
            TextColor nameColor = selected ? TextColor(255, 255, 255) : TextColor(180, 180, 200);
            text->drawText(renderer, games_[i].name.c_str(), x + 12, y + 8, nameColor);

            // Game ID and company
            char infoLine[128];
            snprintf(infoLine, sizeof(infoLine), "[%s] %s",
                     games_[i].id.c_str(), games_[i].company.c_str());
            text->drawText(renderer, infoLine, x + 12, y + 28,
                          TextColor(100, 120, 160));

            // Asset counts
            char assetLine[128];
            snprintf(assetLine, sizeof(assetLine), "Sprites: %d  WAV: %d  MIDI: %d",
                     games_[i].spriteCount, games_[i].wavCount, games_[i].midiCount);
            text->drawText(renderer, assetLine, x + 12, y + 48,
                          TextColor(80, 100, 130));

            // Puzzle/video counts
            char extraLine[128];
            snprintf(extraLine, sizeof(extraLine), "Puzzles: %d  Videos: %d",
                     games_[i].puzzleCount, games_[i].videoCount);
            text->drawText(renderer, extraLine, x + 12, y + 64,
                          TextColor(70, 90, 120));
        }

        // Bottom info bar
        renderer->fillRect(Rect(0, 440, 640, 40), Color(25, 30, 50));

        // Selected game info
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(games_.size())) {
            char selInfo[128];
            snprintf(selInfo, sizeof(selInfo), "%s  (%d/%zu)",
                     games_[selectedIndex_].name.c_str(),
                     selectedIndex_ + 1, games_.size());
            text->drawTextAligned(renderer, selInfo, 0, 448, 640, TextAlign::Center,
                                 TextColor(200, 200, 255));
        }

        text->drawTextAligned(renderer, "Arrow Keys: Navigate   ENTER: Select   ESC: Quit",
                             0, 464, 640, TextAlign::Center, TextColor(100, 110, 140));
    }

    void handleInput() override {
        InputSystem* input = game_->getInput();
        Renderer* renderer = game_->getRenderer();

        if (input->isActionPressed(GameAction::Cancel)) {
            game_->quit();
            return;
        }

        if (games_.empty()) return;

        int cols = 2;
        int totalRows = (static_cast<int>(games_.size()) + cols - 1) / cols;

        // Keyboard navigation
        if (input->isActionPressed(GameAction::MenuUp) ||
            input->isActionPressed(GameAction::MoveUp)) {
            if (selectedIndex_ >= cols) {
                selectedIndex_ -= cols;
                ensureVisible();
            }
        }
        if (input->isActionPressed(GameAction::MenuDown) ||
            input->isActionPressed(GameAction::MoveDown)) {
            if (selectedIndex_ + cols < static_cast<int>(games_.size())) {
                selectedIndex_ += cols;
                ensureVisible();
            }
        }
        if (input->isActionPressed(GameAction::MoveLeft) ||
            input->isActionPressed(GameAction::MenuLeft)) {
            if (selectedIndex_ > 0) {
                selectedIndex_--;
                ensureVisible();
            }
        }
        if (input->isActionPressed(GameAction::MoveRight) ||
            input->isActionPressed(GameAction::MenuRight)) {
            if (selectedIndex_ < static_cast<int>(games_.size()) - 1) {
                selectedIndex_++;
                ensureVisible();
            }
        }

        // Enter to launch
        if (input->isActionPressed(GameAction::Action) ||
            input->isActionPressed(GameAction::MenuSelect)) {
            launchGame(selectedIndex_);
        }

        // Mouse support
        int screenX = input->getMouseX();
        int screenY = input->getMouseY();
        int mouseX, mouseY;
        renderer->screenToGame(screenX, screenY, mouseX, mouseY);

        if (input->isMouseButtonPressed(MouseButton::Left)) {
            // Check which card was clicked
            int cardW = 290, cardH = 90, padX = 15, padY = 10;
            int startX = (640 - (cols * cardW + (cols - 1) * padX)) / 2;
            int startY = 50;

            for (size_t i = 0; i < games_.size(); ++i) {
                int row = static_cast<int>(i) / cols - scrollOffset_;
                int col = static_cast<int>(i) % cols;

                if (row < 0 || row >= 4) continue;

                int x = startX + col * (cardW + padX);
                int y = startY + row * (cardH + padY);

                if (mouseX >= x && mouseX < x + cardW &&
                    mouseY >= y && mouseY < y + cardH) {
                    if (static_cast<int>(i) == selectedIndex_) {
                        // Double-click launches
                        launchGame(selectedIndex_);
                    } else {
                        selectedIndex_ = static_cast<int>(i);
                    }
                    break;
                }
            }
        }

        // Mouse wheel scrolling
        int wheel = input->getMouseWheelDelta();
        if (wheel > 0 && scrollOffset_ > 0) {
            scrollOffset_--;
        } else if (wheel < 0 && scrollOffset_ < totalRows - 4) {
            scrollOffset_++;
        }
    }

private:
    void ensureVisible() {
        int cols = 2;
        int row = selectedIndex_ / cols;
        if (row < scrollOffset_) scrollOffset_ = row;
        if (row >= scrollOffset_ + 4) scrollOffset_ = row - 3;
    }

    void launchGame(int index);

    Game* game_;
    std::vector<GameInfo> games_;
    int selectedIndex_ = 0;
    int scrollOffset_ = 0;
    float animTimer_ = 0.0f;
};

// Game Launch state - per-game landing screen
class GameLaunchState : public GameState {
public:
    GameLaunchState(Game* game, const std::string& gameId)
        : game_(game), gameId_(gameId) {}

    void enter() override {
        SDL_Log("GameLaunch: Entering for game '%s'", gameId_.c_str());

        GameRegistry* registry = game_->getGameRegistry();
        if (registry) {
            const GameInfo* info = registry->getGameInfo(gameId_);
            if (info) {
                gameInfo_ = *info;
            }
        }

        selectedOption_ = 0;
    }

    void exit() override {}

    void update(float dt) override {
        animTimer_ += dt;
    }

    void render() override {
        Renderer* renderer = game_->getRenderer();
        TextRenderer* text = game_->getTextRenderer();

        renderer->clear(Color(15, 20, 40));

        if (!text) return;

        // Title bar
        renderer->fillRect(Rect(0, 0, 640, 50), Color(30, 40, 70));
        text->drawTextAligned(renderer, gameInfo_.name.c_str(), 0, 8, 640, TextAlign::Center,
                            TextColor(255, 255, 100));

        char subtitle[128];
        snprintf(subtitle, sizeof(subtitle), "[%s] by %s",
                 gameInfo_.id.c_str(), gameInfo_.company.c_str());
        text->drawTextAligned(renderer, subtitle, 0, 30, 640, TextAlign::Center,
                            TextColor(150, 160, 200));

        // Game info panel
        renderer->fillRect(Rect(30, 60, 580, 120), Color(20, 25, 45));
        renderer->drawRect(Rect(30, 60, 580, 120), Color(50, 60, 90));

        text->drawText(renderer, "Game Assets:", 45, 70, TextColor(180, 180, 220));

        char assetInfo[256];
        snprintf(assetInfo, sizeof(assetInfo),
                 "Sprites: %d    WAV Files: %d    MIDI Files: %d",
                 gameInfo_.spriteCount, gameInfo_.wavCount, gameInfo_.midiCount);
        text->drawText(renderer, assetInfo, 45, 90, TextColor(120, 140, 180));

        snprintf(assetInfo, sizeof(assetInfo),
                 "Puzzle Resources: %d    Video Files: %d",
                 gameInfo_.puzzleCount, gameInfo_.videoCount);
        text->drawText(renderer, assetInfo, 45, 110, TextColor(120, 140, 180));

        char pathInfo[256];
        snprintf(pathInfo, sizeof(pathInfo), "Path: %s", gameInfo_.extractedPath.c_str());
        text->drawText(renderer, pathInfo, 45, 140, TextColor(80, 100, 130));

        // Menu options
        const char* options[] = {
            "Browse Assets",
            "Play Game",
            "Back to Game Selection"
        };
        int numOptions = 3;

        // Can only "Play" Neptune for now
        bool canPlay = (gameId_ == "on");

        int menuY = 220;
        for (int i = 0; i < numOptions; ++i) {
            bool selected = (i == selectedOption_);
            bool enabled = (i != 1) || canPlay; // "Play" only enabled for Neptune

            int x = 200;
            int y = menuY + i * 50;

            Color bgColor = selected ? Color(50, 60, 100) : Color(25, 30, 50);
            Color borderColor = selected ? Color(100, 150, 255) : Color(50, 60, 80);

            renderer->fillRect(Rect(x, y, 240, 35), bgColor);
            renderer->drawRect(Rect(x, y, 240, 35), borderColor);

            TextColor textColor;
            if (!enabled) {
                textColor = TextColor(80, 80, 100);
            } else if (selected) {
                textColor = TextColor(255, 255, 255);
            } else {
                textColor = TextColor(160, 170, 200);
            }

            text->drawTextAligned(renderer, options[i], x, y + 10, 240, TextAlign::Center, textColor);

            if (i == 1 && !canPlay) {
                text->drawText(renderer, "(Coming soon)", x + 250, y + 10, TextColor(100, 100, 120));
            }
        }

        // Bottom bar
        renderer->fillRect(Rect(0, 445, 640, 35), Color(25, 30, 50));
        text->drawTextAligned(renderer, "UP/DOWN: Navigate   ENTER: Select   ESC: Back",
                             0, 458, 640, TextAlign::Center, TextColor(100, 110, 140));
    }

    void handleInput() override {
        InputSystem* input = game_->getInput();

        if (input->isActionPressed(GameAction::Cancel)) {
            game_->popState();
            return;
        }

        int numOptions = 3;

        if (input->isActionPressed(GameAction::MenuUp) ||
            input->isActionPressed(GameAction::MoveUp)) {
            if (selectedOption_ > 0) selectedOption_--;
        }
        if (input->isActionPressed(GameAction::MenuDown) ||
            input->isActionPressed(GameAction::MoveDown)) {
            if (selectedOption_ < numOptions - 1) selectedOption_++;
        }

        if (input->isActionPressed(GameAction::Action) ||
            input->isActionPressed(GameAction::MenuSelect)) {
            executeOption(selectedOption_);
        }

        // Mouse click support
        Renderer* renderer = game_->getRenderer();
        int screenX = input->getMouseX();
        int screenY = input->getMouseY();
        int mouseX, mouseY;
        renderer->screenToGame(screenX, screenY, mouseX, mouseY);

        if (input->isMouseButtonPressed(MouseButton::Left)) {
            int menuY = 220;
            for (int i = 0; i < numOptions; ++i) {
                int x = 200, y = menuY + i * 50;
                if (mouseX >= x && mouseX < x + 240 &&
                    mouseY >= y && mouseY < y + 35) {
                    selectedOption_ = i;
                    executeOption(i);
                    break;
                }
            }
        }
    }

private:
    void executeOption(int option);

    Game* game_;
    std::string gameId_;
    GameInfo gameInfo_;
    int selectedOption_ = 0;
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
        TextRenderer* text = game_->getTextRenderer();

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

        // Draw HUD background
        renderer->fillRect(Rect(5, 5, 120, 50), Color(30, 30, 60, 200));
        renderer->drawRect(Rect(5, 5, 120, 50), Color(80, 80, 150));

        renderer->fillRect(Rect(515, 5, 120, 50), Color(30, 30, 60, 200));
        renderer->drawRect(Rect(515, 5, 120, 50), Color(80, 80, 150));

        if (text) {
            // Left HUD - Parts collected
            text->drawText(renderer, "PARTS: 0/15", 15, 12, TextColor(255, 255, 100));
            text->drawText(renderer, "AREA: Workshop", 15, 28, TextColor(200, 200, 255));

            // Right HUD - Score and FPS
            char scoreText[32];
            snprintf(scoreText, sizeof(scoreText), "SCORE: %d", score_);
            text->drawText(renderer, scoreText, 525, 12, TextColor(255, 255, 100));

            char fpsText[32];
            snprintf(fpsText, sizeof(fpsText), "FPS: %.0f", game_->getFPS());
            text->drawText(renderer, fpsText, 525, 28, TextColor(150, 200, 150));
        }

        // Instructions at bottom
        renderer->fillRect(Rect(0, 460, 640, 20), Color(30, 30, 60, 200));
        if (text) {
            text->drawTextAligned(renderer, "Arrow Keys: Move   ESC: Menu   F12: Screenshot",
                                 0, 465, 640, TextAlign::Center,
                                 TextColor(150, 150, 200));
        }
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
    int score_ = 0;
};

// Asset Viewer state - for browsing game assets (per-game aware)
class AssetViewerState : public GameState {
public:
    AssetViewerState(Game* game, const std::string& gameId = "")
        : game_(game), gameId_(gameId) {}

    ~AssetViewerState() {
        clearPreview();
    }

    void enter() override {
        SDL_Log("Entering Asset Viewer for game '%s'...", gameId_.c_str());
        loadAssetList();
    }

    void exit() override {
        SDL_Log("Exiting Asset Viewer...");
        clearPreview();
    }

    void update(float dt) override {
        if (needsPreviewUpdate_) {
            loadPreview();
            needsPreviewUpdate_ = false;
        }
    }

    void clearPreview() {
        if (previewTexture_) {
            SDL_DestroyTexture(previewTexture_);
            previewTexture_ = nullptr;
        }
        previewWidth_ = 0;
        previewHeight_ = 0;
    }

    void loadPreview() {
        clearPreview();

        if (selectedItem_ < 0 || selectedItem_ >= static_cast<int>(assetItems_.size())) {
            return;
        }

        AssetCache* cache = game_->getAssetCache();
        if (!cache) return;

        // For extracted game browsing, try loading extracted assets
        if (!gameId_.empty() && !categories_.empty()) {
            const std::string& category = categories_[selectedCategory_];
            const std::string& filename = assetItems_[selectedItem_].first;

            if (category == "sprites") {
                int w, h;
                previewTexture_ = cache->loadExtractedTexture(gameId_, filename, &w, &h);
                if (previewTexture_) {
                    previewWidth_ = w;
                    previewHeight_ = h;
                }
            }
            // Sound/MIDI can't be previewed as textures but we can show info
            return;
        }

        // Legacy NE resource browsing (for Gizmos when no gameId)
        if (selectedItem_ < static_cast<int>(resourceList_.size())) {
            const auto& res = resourceList_[selectedItem_];
            std::string filename = categories_[selectedCategory_];
            auto data = cache->getRawResource(filename, res.typeId, res.id);

            if (data.empty()) return;

            rawDataSize_ = data.size();

            if (res.typeId == 0x8002) { // RT_BITMAP
                previewTexture_ = cache->createTextureFromBitmap(data, &previewWidth_, &previewHeight_);
            } else {
                previewTexture_ = visualizeRawData(data, &previewWidth_, &previewHeight_);
            }

            // Store hex dump for display
            previewHexDump_.clear();
            size_t dumpSize = std::min(data.size(), size_t(256));
            for (size_t i = 0; i < dumpSize; i += 16) {
                char line[80];
                char* p = line;
                p += sprintf(p, "%04zX: ", i);
                for (size_t j = 0; j < 16 && i + j < dumpSize; ++j) {
                    p += sprintf(p, "%02X ", data[i + j]);
                }
                previewHexDump_.push_back(line);
            }
        }
    }

    SDL_Texture* visualizeRawData(const std::vector<uint8_t>& data, int* outW, int* outH) {
        if (data.empty()) return nullptr;

        int gridW = 32;
        int gridH = (static_cast<int>(data.size()) + gridW - 1) / gridW;
        gridH = std::min(gridH, 128);

        *outW = gridW * 4;
        *outH = gridH * 4;

        SDL_Surface* surface = SDL_CreateRGBSurface(0, *outW, *outH, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        if (!surface) return nullptr;

        uint32_t* pixels = static_cast<uint32_t*>(surface->pixels);

        for (int i = 0; i < (*outW) * (*outH); ++i) {
            pixels[i] = 0xFF202020;
        }

        for (size_t i = 0; i < data.size() && static_cast<int>(i / gridW) < gridH; ++i) {
            uint8_t value = data[i];
            int bx = (i % gridW) * 4;
            int by = (i / gridW) * 4;

            uint32_t color;
            if (value == 0x00) {
                color = 0xFF000000;
            } else if (value == 0xFF) {
                color = 0xFFFFFFFF;
            } else {
                uint8_t r = (value < 128) ? (255 - value * 2) : 0;
                uint8_t g = (value < 128) ? (value * 2) : (255 - (value - 128) * 2);
                uint8_t b = (value >= 128) ? ((value - 128) * 2) : 0;
                color = 0xFF000000 | (r << 16) | (g << 8) | b;
            }

            for (int dy = 0; dy < 4; ++dy) {
                for (int dx = 0; dx < 4; ++dx) {
                    int px = bx + dx;
                    int py = by + dy;
                    if (px < *outW && py < *outH) {
                        pixels[py * (*outW) + px] = color;
                    }
                }
            }
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(
            game_->getRenderer()->getSDLRenderer(), surface);
        SDL_FreeSurface(surface);
        return texture;
    }

    void render() override {
        Renderer* renderer = game_->getRenderer();
        TextRenderer* text = game_->getTextRenderer();

        renderer->clear(Color(30, 30, 35));

        // Header
        renderer->fillRect(Rect(0, 0, 640, 30), Color(50, 50, 60));
        if (text) {
            if (!gameId_.empty()) {
                GameRegistry* registry = game_->getGameRegistry();
                const GameInfo* info = registry ? registry->getGameInfo(gameId_) : nullptr;
                char title[128];
                snprintf(title, sizeof(title), "ASSET VIEWER - %s",
                         info ? info->name.c_str() : gameId_.c_str());
                text->drawTextAligned(renderer, title, 0, 8, 640, TextAlign::Center,
                                    TextColor(255, 255, 100));
            } else {
                text->drawTextAligned(renderer, "ASSET VIEWER", 0, 8, 640, TextAlign::Center,
                                    TextColor(255, 255, 100));
            }
        }

        // Left panel - asset list
        renderer->fillRect(Rect(5, 35, 200, 405), Color(25, 25, 30));
        renderer->drawRect(Rect(5, 35, 200, 405), Color(60, 60, 70));

        if (text) {
            text->drawText(renderer, "Categories:", 10, 40, TextColor(150, 150, 200));

            for (size_t i = 0; i < categories_.size(); ++i) {
                int y = 60 + static_cast<int>(i) * 16;
                if (y > 170) break;

                TextColor color = (static_cast<int>(i) == selectedCategory_) ?
                    TextColor(255, 255, 100) : TextColor(120, 120, 150);

                if (static_cast<int>(i) == selectedCategory_) {
                    renderer->fillRect(Rect(8, y - 2, 194, 14), Color(50, 50, 70));
                    text->drawText(renderer, ">", 12, y, color);
                }
                text->drawText(renderer, categories_[i].c_str(), 25, y, color);
            }

            renderer->drawRect(Rect(10, 175, 190, 1), Color(60, 60, 70));

            char countText[64];
            snprintf(countText, sizeof(countText), "Assets: %zu", assetItems_.size());
            text->drawText(renderer, countText, 10, 182, TextColor(150, 150, 200));

            int itemY = 200;
            for (size_t i = scrollOffset_; i < assetItems_.size() && itemY < 430; ++i) {
                TextColor color = (static_cast<int>(i) == selectedItem_) ?
                    TextColor(100, 255, 100) : TextColor(100, 100, 130);

                if (static_cast<int>(i) == selectedItem_) {
                    renderer->fillRect(Rect(8, itemY - 2, 194, 14), Color(40, 60, 40));
                }

                std::string name = assetItems_[i].first;
                if (name.length() > 22) {
                    name = name.substr(0, 19) + "...";
                }
                text->drawText(renderer, name.c_str(), 12, itemY, color);
                itemY += 16;
            }

            if (assetItems_.size() > static_cast<size_t>(maxVisibleItems_)) {
                char scrollInfo[32];
                snprintf(scrollInfo, sizeof(scrollInfo), "[%zu-%zu of %zu]",
                        scrollOffset_ + 1,
                        std::min(scrollOffset_ + maxVisibleItems_, assetItems_.size()),
                        assetItems_.size());
                text->drawText(renderer, scrollInfo, 10, 432, TextColor(80, 80, 100));
            }
        }

        // Right panel - asset preview
        renderer->fillRect(Rect(210, 35, 425, 405), Color(20, 20, 25));
        renderer->drawRect(Rect(210, 35, 425, 405), Color(60, 60, 70));

        if (text) {
            text->drawText(renderer, "Details:", 215, 40, TextColor(150, 150, 200));

            if (selectedItem_ >= 0 && selectedItem_ < static_cast<int>(assetItems_.size())) {
                const auto& item = assetItems_[selectedItem_];
                text->drawText(renderer, item.first.c_str(), 215, 60, TextColor(200, 200, 255));

                int infoY = 85;
                std::string info = item.second;
                while (!info.empty() && infoY < 140) {
                    size_t lineLen = std::min(info.length(), size_t(45));
                    text->drawText(renderer, info.substr(0, lineLen).c_str(), 215, infoY,
                                  TextColor(120, 150, 120));
                    info = info.substr(lineLen);
                    infoY += 16;
                }
            }

            renderer->fillRect(Rect(220, 140, 400, 280), Color(15, 15, 20));
            renderer->drawRect(Rect(220, 140, 400, 280), Color(50, 50, 60));

            if (previewTexture_) {
                int maxW = 390, maxH = 270;
                int drawW = previewWidth_;
                int drawH = previewHeight_;

                if (drawW > maxW || drawH > maxH) {
                    float scaleW = static_cast<float>(maxW) / drawW;
                    float scaleH = static_cast<float>(maxH) / drawH;
                    float scale = std::min(scaleW, scaleH);
                    drawW = static_cast<int>(drawW * scale);
                    drawH = static_cast<int>(drawH * scale);
                }

                int drawX = 220 + (400 - drawW) / 2;
                int drawY = 140 + (280 - drawH) / 2;

                renderer->drawSprite(previewTexture_, Rect(drawX, drawY, drawW, drawH));

                char dimText[64];
                snprintf(dimText, sizeof(dimText), "%dx%d", previewWidth_, previewHeight_);
                text->drawText(renderer, dimText, 225, 400, TextColor(80, 100, 80));
            } else if (!previewHexDump_.empty()) {
                text->drawText(renderer, "Raw Data (hex):", 225, 145, TextColor(100, 100, 150));
                int hexY = 162;
                for (size_t i = 0; i < previewHexDump_.size() && hexY < 410; ++i) {
                    text->drawText(renderer, previewHexDump_[i].c_str(), 225, hexY, TextColor(80, 120, 80));
                    hexY += 12;
                }
                char sizeText[64];
                snprintf(sizeText, sizeof(sizeText), "Size: %zu bytes", rawDataSize_);
                text->drawText(renderer, sizeText, 225, 415, TextColor(80, 100, 80));
            } else {
                text->drawTextAligned(renderer, "[Select a resource to preview]", 220, 270, 400, TextAlign::Center,
                                     TextColor(60, 60, 80));
            }
        }

        // Bottom bar
        renderer->fillRect(Rect(0, 445, 640, 35), Color(40, 40, 50));
        if (text) {
            text->drawTextAligned(renderer, "UP/DOWN: Select   LEFT/RIGHT: Category   ESC: Back",
                                 0, 455, 640, TextAlign::Center, TextColor(100, 100, 130));
        }
    }

    void handleInput() override {
        InputSystem* input = game_->getInput();
        Renderer* renderer = game_->getRenderer();

        if (input->isActionPressed(GameAction::Cancel)) {
            game_->popState();
            return;
        }

        if (input->isActionPressed(GameAction::MenuUp)) {
            if (selectedItem_ > 0) {
                selectedItem_--;
                needsPreviewUpdate_ = true;
            }
            if (selectedItem_ < static_cast<int>(scrollOffset_)) scrollOffset_ = selectedItem_;
        }

        if (input->isActionPressed(GameAction::MenuDown)) {
            if (selectedItem_ < static_cast<int>(assetItems_.size()) - 1) {
                selectedItem_++;
                needsPreviewUpdate_ = true;
            }
            if (selectedItem_ >= static_cast<int>(scrollOffset_) + maxVisibleItems_) scrollOffset_++;
        }

        if (input->isActionPressed(GameAction::MoveLeft)) {
            if (selectedCategory_ > 0) {
                selectedCategory_--;
                loadAssetList();
            }
        }

        if (input->isActionPressed(GameAction::MoveRight)) {
            if (selectedCategory_ < static_cast<int>(categories_.size()) - 1) {
                selectedCategory_++;
                loadAssetList();
            }
        }

        // Mouse support
        int screenX = input->getMouseX();
        int screenY = input->getMouseY();
        int mouseX, mouseY;
        renderer->screenToGame(screenX, screenY, mouseX, mouseY);

        int wheelDelta = input->getMouseWheelDelta();
        if (wheelDelta != 0 && mouseX >= 5 && mouseX <= 205) {
            if (mouseY >= 40 && mouseY <= 175) {
                if (wheelDelta > 0 && selectedCategory_ > 0) {
                    selectedCategory_--;
                    loadAssetList();
                } else if (wheelDelta < 0 && selectedCategory_ < static_cast<int>(categories_.size()) - 1) {
                    selectedCategory_++;
                    loadAssetList();
                }
            } else if (mouseY >= 180 && mouseY <= 440) {
                if (wheelDelta > 0 && scrollOffset_ > 0) {
                    scrollOffset_--;
                } else if (wheelDelta < 0 && scrollOffset_ + maxVisibleItems_ < assetItems_.size()) {
                    scrollOffset_++;
                }
            }
        }

        if (input->isMouseButtonPressed(MouseButton::Left)) {
            if (mouseX >= 8 && mouseX <= 202 && mouseY >= 60 && mouseY <= 170) {
                int clickedCategory = (mouseY - 60) / 16;
                if (clickedCategory >= 0 && clickedCategory < static_cast<int>(categories_.size())) {
                    if (clickedCategory != selectedCategory_) {
                        selectedCategory_ = clickedCategory;
                        loadAssetList();
                    }
                }
            }

            if (mouseX >= 8 && mouseX <= 202 && mouseY >= 200 && mouseY <= 430) {
                int clickedOffset = (mouseY - 200) / 16;
                int clickedItem = static_cast<int>(scrollOffset_) + clickedOffset;
                if (clickedItem >= 0 && clickedItem < static_cast<int>(assetItems_.size())) {
                    if (clickedItem != selectedItem_) {
                        selectedItem_ = clickedItem;
                        needsPreviewUpdate_ = true;
                    }
                }
            }
        }
    }

private:
    void loadAssetList() {
        assetItems_.clear();
        resourceList_.clear();
        clearPreview();
        selectedItem_ = 0;
        scrollOffset_ = 0;

        AssetCache* cache = game_->getAssetCache();
        if (!cache) {
            assetItems_.emplace_back("No asset cache", "Asset cache not initialized");
            return;
        }

        // If we have a gameId, use extracted asset browsing
        if (!gameId_.empty()) {
            if (categories_.empty()) {
                categories_ = {"sprites", "wav", "midi", "puzzles", "rooms", "video"};
            }

            if (selectedCategory_ >= 0 && selectedCategory_ < static_cast<int>(categories_.size())) {
                auto files = cache->listExtractedAssets(gameId_, categories_[selectedCategory_]);

                for (const auto& file : files) {
                    assetItems_.emplace_back(file, categories_[selectedCategory_] + " asset");
                }

                if (assetItems_.empty()) {
                    assetItems_.emplace_back("(No assets found)", "Directory may be empty");
                }
            }
        } else {
            // Legacy NE resource browsing
            if (categories_.empty()) {
                categories_ = {
                    "GIZMO.DAT", "GIZMO256.DAT", "PUZZLE.DAT",
                    "FONT.DAT", "ACTSPCH.DAT", "GMESPCH.DAT"
                };
            }

            if (selectedCategory_ >= 0 && selectedCategory_ < static_cast<int>(categories_.size())) {
                std::string filename = categories_[selectedCategory_];
                resourceList_ = cache->getNEResourceList(filename);

                for (const auto& res : resourceList_) {
                    char info[128];
                    snprintf(info, sizeof(info), "Type: %s  ID: %u  Size: %u bytes",
                             res.typeName.c_str(), res.id, res.size);
                    assetItems_.emplace_back(
                        res.name.empty() ? ("Resource " + std::to_string(res.id)) : res.name,
                        info);
                }

                if (assetItems_.empty()) {
                    assetItems_.emplace_back("(No resources found)", "File may not exist or is not a valid NE file");
                }
            }
        }

        needsPreviewUpdate_ = true;
    }

    Game* game_;
    std::string gameId_;
    std::vector<std::string> categories_;
    int selectedCategory_ = 0;
    int selectedItem_ = 0;
    size_t scrollOffset_ = 0;
    std::vector<std::pair<std::string, std::string>> assetItems_;
    std::vector<Resource> resourceList_;
    static const int maxVisibleItems_ = 14;

    SDL_Texture* previewTexture_ = nullptr;
    int previewWidth_ = 0;
    int previewHeight_ = 0;
    bool needsPreviewUpdate_ = false;
    size_t rawDataSize_ = 0;
    std::vector<std::string> previewHexDump_;
};

// State transition implementations
void TitleState::advanceToGameSelect() {
    game_->changeState(std::make_unique<GameSelectionState>(game_));
}

void GameSelectionState::launchGame(int index) {
    if (index < 0 || index >= static_cast<int>(games_.size())) return;

    const std::string& gameId = games_[index].id;
    SDL_Log("GameSelection: Launching game '%s'", gameId.c_str());

    game_->pushState(std::make_unique<GameLaunchState>(game_, gameId));
}

void GameLaunchState::executeOption(int option) {
    switch (option) {
        case 0: // Browse Assets
            game_->pushState(std::make_unique<AssetViewerState>(game_, gameId_));
            break;

        case 1: // Play Game
            if (gameId_ == "on") {
                // Launch Neptune
                game_->pushState(std::make_unique<NeptuneGameState>(game_));
            }
            break;

        case 2: // Back
            game_->popState();
            break;
    }
}

// Main function
int main(int argc, char* argv[]) {
    SDL_SetMainReady();

    // Parse command line arguments
    GameConfig config;
    config.windowTitle = "OpenGG - TLC Educational Games";

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
            std::cout << "OpenGG - TLC Educational Game Launcher\n\n";
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
            "Failed to initialize game engine.\n"
            "Check that extracted game assets are available.", nullptr);
        return 1;
    }

    // Set up menu callbacks
    game.setNewGameCallback([&game]() {
        game.changeState(std::make_unique<GameSelectionState>(&game));
    });

    game.setAssetViewerCallback([&game]() {
        game.pushState(std::make_unique<AssetViewerState>(&game));
    });

    // Start with title screen
    game.pushState(std::make_unique<TitleState>(&game));

    // Run main loop
    game.run();

    // Cleanup
    game.shutdown();

    return 0;
}
