#include "renderer.h"
#include "grp_archive.h"
#include <SDL.h>
#include <algorithm>
#include <cstring>

namespace opengg {

// DirtyRectManager implementation
void DirtyRectManager::addDirtyRect(const Rect& rect) {
    if (rect.w > 0 && rect.h > 0) {
        dirtyRects_.push_back(rect);
    }
}

void DirtyRectManager::clear() {
    dirtyRects_.clear();
}

void DirtyRectManager::optimize() {
    if (dirtyRects_.size() <= 1) return;

    // Simple merging: combine overlapping rectangles
    bool merged = true;
    while (merged) {
        merged = false;
        for (size_t i = 0; i < dirtyRects_.size() && !merged; ++i) {
            for (size_t j = i + 1; j < dirtyRects_.size() && !merged; ++j) {
                if (dirtyRects_[i].intersects(dirtyRects_[j])) {
                    // Merge into bounding box
                    int x1 = std::min(dirtyRects_[i].x, dirtyRects_[j].x);
                    int y1 = std::min(dirtyRects_[i].y, dirtyRects_[j].y);
                    int x2 = std::max(dirtyRects_[i].x + dirtyRects_[i].w,
                                      dirtyRects_[j].x + dirtyRects_[j].w);
                    int y2 = std::max(dirtyRects_[i].y + dirtyRects_[i].h,
                                      dirtyRects_[j].y + dirtyRects_[j].h);
                    dirtyRects_[i] = Rect(x1, y1, x2 - x1, y2 - y1);
                    dirtyRects_.erase(dirtyRects_.begin() + j);
                    merged = true;
                }
            }
        }
    }
}

// Renderer implementation
Renderer::Renderer() {
    // Initialize default palette (grayscale)
    palette_.resize(256);
    for (int i = 0; i < 256; ++i) {
        palette_[i] = (0xFF << 24) | (i << 16) | (i << 8) | i;
    }
}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize(const std::string& title, int windowWidth, int windowHeight) {
    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        lastError_ = "SDL_Init failed: " + std::string(SDL_GetError());
        return false;
    }

    // Default to 2x scale if not specified
    if (windowWidth <= 0 || windowHeight <= 0) {
        scale_ = 2;
        windowWidth = GAME_WIDTH * scale_;
        windowHeight = GAME_HEIGHT * scale_;
    } else {
        scale_ = std::min(windowWidth / GAME_WIDTH, windowHeight / GAME_HEIGHT);
        if (scale_ < 1) scale_ = 1;
    }

    // Create window
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window_) {
        lastError_ = "SDL_CreateWindow failed: " + std::string(SDL_GetError());
        return false;
    }

    // Create renderer with vsync
    renderer_ = SDL_CreateRenderer(
        window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer_) {
        lastError_ = "SDL_CreateRenderer failed: " + std::string(SDL_GetError());
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    // Set logical size for automatic scaling
    SDL_RenderSetLogicalSize(renderer_, GAME_WIDTH, GAME_HEIGHT);

    // Enable linear filtering for scaling
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    windowWidth_ = windowWidth;
    windowHeight_ = windowHeight;

    return true;
}

void Renderer::shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

void Renderer::setFullscreen(bool fullscreen) {
    if (window_) {
        SDL_SetWindowFullscreen(window_,
            fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        fullscreen_ = fullscreen;
    }
}

bool Renderer::isFullscreen() const {
    return fullscreen_;
}

void Renderer::setWindowScale(int scale) {
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;

    scale_ = scale;
    if (window_) {
        SDL_SetWindowSize(window_, GAME_WIDTH * scale_, GAME_HEIGHT * scale_);
    }
}

void Renderer::beginFrame() {
    if (useDirtyRects_) {
        dirtyRects_.clear();
    }
}

void Renderer::endFrame() {
    if (useDirtyRects_) {
        dirtyRects_.optimize();
    }

    // Apply fade effect
    if (fadeLevel_ < 1.0f) {
        uint8_t alpha = static_cast<uint8_t>((1.0f - fadeLevel_) * 255);
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, alpha);
        SDL_RenderFillRect(renderer_, nullptr);
    }

    // Apply flash effect
    if (flashIntensity_ > 0.0f) {
        uint8_t alpha = static_cast<uint8_t>(flashIntensity_ * 255);
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, flashColor_.r, flashColor_.g, flashColor_.b, alpha);
        SDL_RenderFillRect(renderer_, nullptr);
    }
}

void Renderer::present() {
    SDL_RenderPresent(renderer_);
}

void Renderer::clear(const Color& color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);

    if (useDirtyRects_) {
        markFullDirty();
    }
}

void Renderer::drawSprite(SDL_Texture* texture, int x, int y) {
    if (!texture) return;

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer_, texture, nullptr, &dst);

    if (useDirtyRects_) {
        markDirty(Rect(x, y, w, h));
    }
}

void Renderer::drawSprite(SDL_Texture* texture, int x, int y, const Rect& srcRect) {
    if (!texture) return;

    SDL_Rect src = {srcRect.x, srcRect.y, srcRect.w, srcRect.h};
    SDL_Rect dst = {x, y, srcRect.w, srcRect.h};
    SDL_RenderCopy(renderer_, texture, &src, &dst);

    if (useDirtyRects_) {
        markDirty(Rect(x, y, srcRect.w, srcRect.h));
    }
}

void Renderer::drawSprite(SDL_Texture* texture, const Rect& destRect) {
    if (!texture) return;

    SDL_Rect dst = {destRect.x, destRect.y, destRect.w, destRect.h};
    SDL_RenderCopy(renderer_, texture, nullptr, &dst);

    if (useDirtyRects_) {
        markDirty(destRect);
    }
}

void Renderer::drawSprite(SDL_Texture* texture, const Rect& srcRect, const Rect& destRect) {
    if (!texture) return;

    SDL_Rect src = {srcRect.x, srcRect.y, srcRect.w, srcRect.h};
    SDL_Rect dst = {destRect.x, destRect.y, destRect.w, destRect.h};
    SDL_RenderCopy(renderer_, texture, &src, &dst);

    if (useDirtyRects_) {
        markDirty(destRect);
    }
}

void Renderer::drawSpriteFlipped(SDL_Texture* texture, int x, int y, bool flipH, bool flipV) {
    if (!texture) return;

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    SDL_Rect dst = {x, y, w, h};
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (flipH) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
    if (flipV) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_VERTICAL);

    SDL_RenderCopyEx(renderer_, texture, nullptr, &dst, 0, nullptr, flip);

    if (useDirtyRects_) {
        markDirty(Rect(x, y, w, h));
    }
}

void Renderer::drawSprite(const Sprite& sprite, int x, int y) {
    // Create texture from sprite data
    SDL_Texture* texture = createPalettedTexture(sprite);
    if (!texture) return;

    // Apply hotspot offset
    int drawX = x - sprite.hotspotX;
    int drawY = y - sprite.hotspotY;

    SDL_Rect dst = {drawX, drawY, sprite.width, sprite.height};
    SDL_RenderCopy(renderer_, texture, nullptr, &dst);

    // Free temporary texture
    SDL_DestroyTexture(texture);

    if (useDirtyRects_) {
        markDirty(Rect(drawX, drawY, sprite.width, sprite.height));
    }
}

SDL_Texture* Renderer::createPalettedTexture(const Sprite& sprite) {
    // Create RGBA surface from indexed pixels
    SDL_Surface* surface = SDL_CreateRGBSurface(
        0, sprite.width, sprite.height, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );

    if (!surface) return nullptr;

    // Convert indexed to RGBA
    uint32_t* pixels = static_cast<uint32_t*>(surface->pixels);
    const auto& pal = sprite.hasPalette ? sprite.palette : palette_;

    for (int y = 0; y < sprite.height; ++y) {
        for (int x = 0; x < sprite.width; ++x) {
            int idx = y * sprite.width + x;
            uint8_t colorIndex = sprite.pixels[idx];

            // Index 0 is typically transparent
            if (colorIndex == 0) {
                pixels[idx] = 0;
            } else {
                pixels[idx] = pal[colorIndex];
            }
        }
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);

    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }

    return texture;
}

void Renderer::drawRect(const Rect& rect, const Color& color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    SDL_Rect r = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderDrawRect(renderer_, &r);

    if (useDirtyRects_) {
        markDirty(rect);
    }
}

void Renderer::fillRect(const Rect& rect, const Color& color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    SDL_Rect r = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(renderer_, &r);

    if (useDirtyRects_) {
        markDirty(rect);
    }
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, const Color& color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);

    if (useDirtyRects_) {
        int minX = std::min(x1, x2);
        int minY = std::min(y1, y2);
        int maxX = std::max(x1, x2);
        int maxY = std::max(y1, y2);
        markDirty(Rect(minX, minY, maxX - minX + 1, maxY - minY + 1));
    }
}

void Renderer::drawPoint(int x, int y, const Color& color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderDrawPoint(renderer_, x, y);

    if (useDirtyRects_) {
        markDirty(Rect(x, y, 1, 1));
    }
}

void Renderer::setFont(SDL_Texture* fontTexture, int charWidth, int charHeight) {
    fontTexture_ = fontTexture;
    fontCharWidth_ = charWidth;
    fontCharHeight_ = charHeight;
}

void Renderer::drawText(const std::string& text, int x, int y, const Color& color) {
    if (!fontTexture_) return;

    SDL_SetTextureColorMod(fontTexture_, color.r, color.g, color.b);

    int curX = x;
    for (char c : text) {
        if (c == '\n') {
            curX = x;
            y += fontCharHeight_;
            continue;
        }

        // Assume ASCII font starting from space (32)
        int charIndex = static_cast<int>(c) - 32;
        if (charIndex < 0 || charIndex >= 96) {
            curX += fontCharWidth_;
            continue;
        }

        // Calculate source rect in font texture (16 chars per row)
        int srcX = (charIndex % 16) * fontCharWidth_;
        int srcY = (charIndex / 16) * fontCharHeight_;

        SDL_Rect src = {srcX, srcY, fontCharWidth_, fontCharHeight_};
        SDL_Rect dst = {curX, y, fontCharWidth_, fontCharHeight_};
        SDL_RenderCopy(renderer_, fontTexture_, &src, &dst);

        curX += fontCharWidth_;
    }

    if (useDirtyRects_) {
        markDirty(Rect(x, y, getTextWidth(text), fontCharHeight_));
    }
}

int Renderer::getTextWidth(const std::string& text) const {
    int maxWidth = 0;
    int curWidth = 0;

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, curWidth);
            curWidth = 0;
        } else {
            curWidth += fontCharWidth_;
        }
    }

    return std::max(maxWidth, curWidth);
}

std::unique_ptr<RenderTarget> Renderer::createRenderTarget(int width, int height) {
    SDL_Texture* texture = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width, height
    );

    if (!texture) {
        lastError_ = "Failed to create render target: " + std::string(SDL_GetError());
        return nullptr;
    }

    return std::make_unique<RenderTarget>(texture, width, height);
}

void Renderer::setRenderTarget(RenderTarget* target) {
    if (target) {
        SDL_SetRenderTarget(renderer_, target->getTexture());
    } else {
        SDL_SetRenderTarget(renderer_, nullptr);
    }
}

void Renderer::resetRenderTarget() {
    SDL_SetRenderTarget(renderer_, nullptr);
}

void Renderer::setClipRect(const Rect& rect) {
    SDL_Rect r = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderSetClipRect(renderer_, &r);
}

void Renderer::clearClipRect() {
    SDL_RenderSetClipRect(renderer_, nullptr);
}

void Renderer::setPalette(const std::vector<uint32_t>& palette) {
    palette_ = palette;
    if (palette_.size() < 256) {
        palette_.resize(256, 0xFF000000);
    }
}

void Renderer::fadeIn(float progress) {
    fadeLevel_ = std::clamp(progress, 0.0f, 1.0f);
}

void Renderer::fadeOut(float progress) {
    fadeLevel_ = std::clamp(1.0f - progress, 0.0f, 1.0f);
}

void Renderer::flash(const Color& color, float intensity) {
    flashColor_ = color;
    flashIntensity_ = std::clamp(intensity, 0.0f, 1.0f);
}

void Renderer::markDirty(const Rect& rect) {
    dirtyRects_.addDirtyRect(rect);
}

void Renderer::markFullDirty() {
    dirtyRects_.clear();
    dirtyRects_.addDirtyRect(Rect(0, 0, GAME_WIDTH, GAME_HEIGHT));
}

bool Renderer::saveScreenshot(const std::string& path) {
    // Create surface for screenshot
    SDL_Surface* surface = SDL_CreateRGBSurface(
        0, GAME_WIDTH, GAME_HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );

    if (!surface) {
        lastError_ = "Failed to create screenshot surface";
        return false;
    }

    // Read pixels from renderer
    if (SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_ARGB8888,
                             surface->pixels, surface->pitch) != 0) {
        lastError_ = "Failed to read pixels: " + std::string(SDL_GetError());
        SDL_FreeSurface(surface);
        return false;
    }

    // Save as BMP
    if (SDL_SaveBMP(surface, path.c_str()) != 0) {
        lastError_ = "Failed to save BMP: " + std::string(SDL_GetError());
        SDL_FreeSurface(surface);
        return false;
    }

    SDL_FreeSurface(surface);
    return true;
}

} // namespace opengg
