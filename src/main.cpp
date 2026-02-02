#include "game_loop.h"
#include "renderer.h"
#include "audio.h"
#include "input.h"
#include "asset_cache.h"
#include "ne_resource.h"
#include "font.h"
#include <SDL.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <cstdlib>
#include <cmath>

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
            // Use aligned text for proper centering
            text->drawTextAligned(renderer, "OPENGIZMOS", centerX - 200, centerY - 60, 400, TextAlign::Center,
                                TextColor(255, 255, 100, alpha));

            text->drawTextAligned(renderer, "Super Solvers: Gizmos & Gadgets",
                                 centerX - 150, centerY - 30, 300, TextAlign::Center,
                                 TextColor(200, 200, 255, alpha));

            text->drawTextAligned(renderer, "Reimplementation",
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

// Main menu state - static placeholder (will show game art once assets loaded)
class MainMenuState : public GameState {
public:
    explicit MainMenuState(Game* game) : game_(game) {}

    void enter() override {}
    void exit() override {}

    void update(float dt) override {
        animTimer_ += dt;
    }

    void render() override {
        Renderer* renderer = game_->getRenderer();
        TextRenderer* text = game_->getTextRenderer();

        // Gradient-style background placeholder
        renderer->clear(Color(25, 35, 60));

        int centerX = Renderer::GAME_WIDTH / 2;
        int centerY = Renderer::GAME_HEIGHT / 2;

        // Draw placeholder for where game art will go
        renderer->fillRect(Rect(20, 20, 600, 400), Color(15, 25, 45));
        renderer->drawRect(Rect(20, 20, 600, 400), Color(60, 80, 120));

        if (text) {
            // Placeholder text
            text->drawTextAligned(renderer, "GIZMOS & GADGETS", 20, centerY - 40, 600, TextAlign::Center,
                                TextColor(100, 140, 200));
            text->drawTextAligned(renderer, "[Game artwork will appear here]", 20, centerY, 600, TextAlign::Center,
                                TextColor(70, 90, 130));
        }

        // Bottom info bar
        renderer->fillRect(Rect(0, 430, 640, 50), Color(20, 30, 50));
        if (text) {
            text->drawTextAligned(renderer, "Use File menu to start a New Game", 0, 445, 640, TextAlign::Center,
                                 TextColor(120, 150, 200));
            text->drawText(renderer, "v0.1.0", 580, 460, TextColor(60, 80, 110));
        }
    }

    void handleInput() override {
        InputSystem* input = game_->getInput();

        if (input->isActionPressed(GameAction::Cancel)) {
            game_->quit();
        }
    }

private:
    Game* game_;
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

// Asset Viewer state - for browsing game assets
class AssetViewerState : public GameState {
public:
    explicit AssetViewerState(Game* game) : game_(game) {}

    ~AssetViewerState() {
        clearPreview();
    }

    void enter() override {
        SDL_Log("Entering Asset Viewer...");
        loadAssetList();
    }

    void exit() override {
        SDL_Log("Exiting Asset Viewer...");
        clearPreview();
    }

    void update(float dt) override {
        // Load preview if selection changed
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

        if (selectedItem_ < 0 || selectedItem_ >= static_cast<int>(resourceList_.size())) {
            return;
        }

        AssetCache* cache = game_->getAssetCache();
        if (!cache) return;

        const auto& res = resourceList_[selectedItem_];
        std::string filename = categories_[selectedCategory_];
        auto data = cache->getRawResource(filename, res.typeId, res.id);

        if (data.empty()) return;

        // Store raw data info
        rawDataSize_ = data.size();

        // Try to interpret as standard bitmap
        if (res.typeId == 0x8002) { // RT_BITMAP
            previewTexture_ = cache->createTextureFromBitmap(data, &previewWidth_, &previewHeight_);
        }
        // Try to interpret custom types as sprite data
        else if (res.typeId == 0x7F01 || res.typeId == 0x7F02 || res.typeId == 0x7F03 ||
                 res.typeId == 0x800F) { // Custom sprite types
            previewTexture_ = tryDecodeAsSprite(data, &previewWidth_, &previewHeight_);

            // If sprite decode failed, show raw data visualization
            if (!previewTexture_) {
                previewTexture_ = visualizeRawData(data, &previewWidth_, &previewHeight_);
            }
        }
        // For any other type, visualize raw data
        else {
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

    SDL_Texture* tryDecodeAsSprite(const std::vector<uint8_t>& data, int* outW, int* outH) {
        if (data.size() < 12) return nullptr;

        // Parse sprite header: width(2), height(2), hotspotX(2), hotspotY(2), flags(2), reserved(2)
        uint16_t width = data[0] | (data[1] << 8);
        uint16_t height = data[2] | (data[3] << 8);

        // Sanity check dimensions
        if (width == 0 || width > 512 || height == 0 || height > 512) {
            return nullptr;
        }

        *outW = width;
        *outH = height;

        // Create a surface to render to
        SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        if (!surface) return nullptr;

        uint32_t* pixels = static_cast<uint32_t*>(surface->pixels);

        // Initialize with transparency (magenta for visibility)
        for (int i = 0; i < width * height; ++i) {
            pixels[i] = 0xFFFF00FF; // Magenta = transparent
        }

        // After the 12-byte header, there's a row offset table (4 bytes per row)
        size_t headerSize = 12;
        size_t offsetTableSize = height * 4;
        size_t pixelDataStart = headerSize + offsetTableSize;

        if (data.size() < pixelDataStart) {
            // Not enough data for offset table - try simple format
            size_t idx = headerSize;
            for (int y = 0; y < height && idx < data.size(); ++y) {
                for (int x = 0; x < width && idx < data.size(); ++x) {
                    uint8_t c = data[idx++];
                    // Use a simple palette mapping
                    uint8_t r = (c & 0xE0);
                    uint8_t g = (c & 0x1C) << 3;
                    uint8_t b = (c & 0x03) << 6;
                    if (c == 0) {
                        pixels[y * width + x] = 0x00000000; // Transparent
                    } else {
                        pixels[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                    }
                }
            }
        } else {
            // Try to decode with row offsets - this format uses RLE per row
            for (int y = 0; y < height; ++y) {
                // Get row offset from table
                size_t offsetIdx = headerSize + y * 4;
                uint32_t rowOffset = data[offsetIdx] | (data[offsetIdx+1] << 8) |
                                    (data[offsetIdx+2] << 16) | (data[offsetIdx+3] << 24);

                // Decode row (simplified - real format might be more complex)
                int x = 0;
                size_t rowIdx = pixelDataStart + rowOffset;

                while (x < width && rowIdx < data.size()) {
                    uint8_t cmd = data[rowIdx++];

                    if (cmd == 0) break; // End of row

                    if (cmd & 0x80) {
                        // Skip transparent pixels
                        int skip = cmd & 0x7F;
                        x += skip;
                    } else {
                        // Draw pixels
                        int count = cmd;
                        for (int i = 0; i < count && x < width && rowIdx < data.size(); ++i) {
                            uint8_t c = data[rowIdx++];
                            // Map palette index to color (using grayscale for now)
                            if (c == 0) {
                                pixels[y * width + x] = 0x00000000;
                            } else {
                                // Create a visible color from the palette index
                                uint8_t r = ((c >> 5) & 7) * 36;
                                uint8_t g = ((c >> 2) & 7) * 36;
                                uint8_t b = (c & 3) * 85;
                                pixels[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                            }
                            x++;
                        }
                    }
                }
            }
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(
            game_->getRenderer()->getSDLRenderer(), surface);
        SDL_FreeSurface(surface);

        return texture;
    }

    // Visualize raw data as a grid of colored pixels (for debugging unknown formats)
    SDL_Texture* visualizeRawData(const std::vector<uint8_t>& data, int* outW, int* outH) {
        if (data.empty()) return nullptr;

        // Calculate grid size to show all bytes
        int gridW = 32; // bytes per row
        int gridH = (static_cast<int>(data.size()) + gridW - 1) / gridW;
        gridH = std::min(gridH, 128); // Limit height

        *outW = gridW * 4; // 4 pixels per byte for visibility
        *outH = gridH * 4;

        SDL_Surface* surface = SDL_CreateRGBSurface(0, *outW, *outH, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        if (!surface) return nullptr;

        uint32_t* pixels = static_cast<uint32_t*>(surface->pixels);

        // Fill with dark background
        for (int i = 0; i < (*outW) * (*outH); ++i) {
            pixels[i] = 0xFF202020;
        }

        // Draw each byte as a 4x4 colored block
        for (size_t i = 0; i < data.size() && static_cast<int>(i / gridW) < gridH; ++i) {
            uint8_t value = data[i];
            int bx = (i % gridW) * 4;
            int by = (i / gridW) * 4;

            // Create a color from the byte value
            // Use a palette that makes patterns visible
            uint32_t color;
            if (value == 0x00) {
                color = 0xFF000000; // Black for zero
            } else if (value == 0xFF) {
                color = 0xFFFFFFFF; // White for 0xFF
            } else {
                // Rainbow-ish mapping for other values
                uint8_t r = (value < 128) ? (255 - value * 2) : 0;
                uint8_t g = (value < 128) ? (value * 2) : (255 - (value - 128) * 2);
                uint8_t b = (value >= 128) ? ((value - 128) * 2) : 0;
                color = 0xFF000000 | (r << 16) | (g << 8) | b;
            }

            // Draw 4x4 block
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
            text->drawTextAligned(renderer, "ASSET VIEWER", 0, 8, 640, TextAlign::Center,
                                TextColor(255, 255, 100));
        }

        // Left panel - asset list
        renderer->fillRect(Rect(5, 35, 200, 405), Color(25, 25, 30));
        renderer->drawRect(Rect(5, 35, 200, 405), Color(60, 60, 70));

        if (text) {
            text->drawText(renderer, "Source Files:", 10, 40, TextColor(150, 150, 200));

            // Show asset categories (files)
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

            // Separator
            renderer->drawRect(Rect(10, 175, 190, 1), Color(60, 60, 70));

            // Show items in selected category
            char countText[64];
            snprintf(countText, sizeof(countText), "Resources: %zu", assetItems_.size());
            text->drawText(renderer, countText, 10, 182, TextColor(150, 150, 200));

            int itemY = 200;
            int maxVisible = 14;
            for (size_t i = scrollOffset_; i < assetItems_.size() && itemY < 430; ++i) {
                TextColor color = (static_cast<int>(i) == selectedItem_) ?
                    TextColor(100, 255, 100) : TextColor(100, 100, 130);

                if (static_cast<int>(i) == selectedItem_) {
                    renderer->fillRect(Rect(8, itemY - 2, 194, 14), Color(40, 60, 40));
                }

                // Truncate long names
                std::string name = assetItems_[i].first;
                if (name.length() > 22) {
                    name = name.substr(0, 19) + "...";
                }
                text->drawText(renderer, name.c_str(), 12, itemY, color);
                itemY += 16;
            }

            // Scroll indicator
            if (assetItems_.size() > static_cast<size_t>(maxVisible)) {
                char scrollInfo[32];
                snprintf(scrollInfo, sizeof(scrollInfo), "[%zu-%zu of %zu]",
                        scrollOffset_ + 1,
                        std::min(scrollOffset_ + maxVisible, assetItems_.size()),
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
                // Show selected resource info
                const auto& item = assetItems_[selectedItem_];
                text->drawText(renderer, item.first.c_str(), 215, 60, TextColor(200, 200, 255));

                // Parse and show the info string (multi-line if needed)
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

            // Preview area
            renderer->fillRect(Rect(220, 140, 400, 280), Color(15, 15, 20));
            renderer->drawRect(Rect(220, 140, 400, 280), Color(50, 50, 60));

            if (previewTexture_) {
                // Calculate scaled size to fit in preview area while maintaining aspect ratio
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

                // Center in preview area
                int drawX = 220 + (400 - drawW) / 2;
                int drawY = 140 + (280 - drawH) / 2;

                renderer->drawSprite(previewTexture_, Rect(drawX, drawY, drawW, drawH));

                // Show dimensions
                char dimText[64];
                snprintf(dimText, sizeof(dimText), "%dx%d", previewWidth_, previewHeight_);
                text->drawText(renderer, dimText, 225, 400, TextColor(80, 100, 80));
            } else if (!previewHexDump_.empty()) {
                // Show hex dump of raw data
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
            text->drawTextAligned(renderer, "UP/DOWN: Select   LEFT/RIGHT: Category   ENTER: Load   ESC: Back",
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

        // Keyboard navigation
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

        // Mouse wheel scrolling
        int wheelDelta = input->getMouseWheelDelta();
        if (wheelDelta != 0 && mouseX >= 5 && mouseX <= 205) {
            // Category scrolling (if mouse is over category area at top)
            if (mouseY >= 40 && mouseY <= 175) {
                if (wheelDelta > 0 && selectedCategory_ > 0) {
                    selectedCategory_--;
                    loadAssetList();
                } else if (wheelDelta < 0 && selectedCategory_ < static_cast<int>(categories_.size()) - 1) {
                    selectedCategory_++;
                    loadAssetList();
                }
            }
            // Item list scrolling (anywhere else in left panel)
            else if (mouseY >= 180 && mouseY <= 440) {
                if (wheelDelta > 0 && scrollOffset_ > 0) {
                    scrollOffset_--;
                } else if (wheelDelta < 0 && scrollOffset_ + maxVisibleItems_ < assetItems_.size()) {
                    scrollOffset_++;
                }
            }
        }

        // Mouse click handling
        if (input->isMouseButtonPressed(MouseButton::Left)) {
            // Click on category list (y: 60-170, 16px per item)
            if (mouseX >= 8 && mouseX <= 202 && mouseY >= 60 && mouseY <= 170) {
                int clickedCategory = (mouseY - 60) / 16;
                if (clickedCategory >= 0 && clickedCategory < static_cast<int>(categories_.size())) {
                    if (clickedCategory != selectedCategory_) {
                        selectedCategory_ = clickedCategory;
                        loadAssetList();
                    }
                }
            }

            // Click on item list (y: 200-430, 16px per item)
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

        // Initialize categories if empty
        if (categories_.empty()) {
            categories_ = {
                "GIZMO.DAT",
                "GIZMO256.DAT",
                "PUZZLE.DAT",
                "FONT.DAT",
                "ACTSPCH.DAT",
                "GMESPCH.DAT"
            };
        }

        AssetCache* cache = game_->getAssetCache();
        if (!cache) {
            assetItems_.emplace_back("No asset cache", "Asset cache not initialized");
            return;
        }

        if (selectedCategory_ >= 0 && selectedCategory_ < static_cast<int>(categories_.size())) {
            std::string filename = categories_[selectedCategory_];

            // Get raw resource list for preview functionality
            resourceList_ = cache->getNEResourceList(filename);

            // Build display list
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

        needsPreviewUpdate_ = true;
    }

    Game* game_;
    std::vector<std::string> categories_;
    int selectedCategory_ = 0;
    int selectedItem_ = 0;
    int lastPreviewedItem_ = -1;
    size_t scrollOffset_ = 0;
    std::vector<std::pair<std::string, std::string>> assetItems_;
    std::vector<Resource> resourceList_;
    static const int maxVisibleItems_ = 14;

    // Preview
    SDL_Texture* previewTexture_ = nullptr;
    int previewWidth_ = 0;
    int previewHeight_ = 0;
    bool needsPreviewUpdate_ = false;
    size_t rawDataSize_ = 0;
    std::vector<std::string> previewHexDump_;
};

// State transition implementations
void TitleState::advanceToMenu() {
    game_->changeState(std::make_unique<MainMenuState>(game_));
}

// Main function
int main(int argc, char* argv[]) {
    SDL_SetMainReady();

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

    // Set up menu callbacks
    game.setNewGameCallback([&game]() {
        game.changeState(std::make_unique<GameplayState>(&game));
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
