#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Rect;

namespace opengg {

struct Sprite;

// Color structure
struct Color {
    uint8_t r, g, b, a;

    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    static Color fromRGB(uint32_t rgb) {
        return Color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
    }

    static Color fromRGBA(uint32_t rgba) {
        return Color((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF,
                     (rgba >> 8) & 0xFF, rgba & 0xFF);
    }
};

// Rectangle structure
struct Rect {
    int x, y, w, h;

    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}

    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    bool intersects(const Rect& other) const {
        return !(x + w <= other.x || other.x + other.w <= x ||
                 y + h <= other.y || other.y + other.h <= y);
    }

    Rect intersection(const Rect& other) const {
        int nx = std::max(x, other.x);
        int ny = std::max(y, other.y);
        int nx2 = std::min(x + w, other.x + other.w);
        int ny2 = std::min(y + h, other.y + other.h);
        if (nx2 > nx && ny2 > ny) {
            return Rect(nx, ny, nx2 - nx, ny2 - ny);
        }
        return Rect();
    }
};

// Render target for off-screen rendering
class RenderTarget {
public:
    RenderTarget(SDL_Texture* texture, int w, int h)
        : texture_(texture), width_(w), height_(h) {}

    SDL_Texture* getTexture() { return texture_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    SDL_Texture* texture_;
    int width_;
    int height_;
};

// Dirty rectangle tracking for optimized rendering
class DirtyRectManager {
public:
    void addDirtyRect(const Rect& rect);
    void clear();
    const std::vector<Rect>& getDirtyRects() const { return dirtyRects_; }
    bool isEmpty() const { return dirtyRects_.empty(); }

    // Merge overlapping rectangles to reduce draw calls
    void optimize();

private:
    std::vector<Rect> dirtyRects_;
};

// Main renderer class
class Renderer {
public:
    // Original game resolution
    static constexpr int GAME_WIDTH = 640;
    static constexpr int GAME_HEIGHT = 480;

    Renderer();
    ~Renderer();

    // Initialize window and renderer
    bool initialize(const std::string& title, int windowWidth = 0, int windowHeight = 0);

    // Shutdown
    void shutdown();

    // Get SDL handles (for asset cache etc.)
    SDL_Renderer* getSDLRenderer() { return renderer_; }
    SDL_Window* getSDLWindow() { return window_; }

    // Window management
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const;
    void setWindowScale(int scale);
    int getWindowScale() const { return scale_; }

    // Frame control
    void beginFrame();
    void endFrame();
    void present();

    // Clear with color
    void clear(const Color& color = Color(0, 0, 0));

    // Sprite drawing
    void drawSprite(SDL_Texture* texture, int x, int y);
    void drawSprite(SDL_Texture* texture, int x, int y, const Rect& srcRect);
    void drawSprite(SDL_Texture* texture, const Rect& destRect);
    void drawSprite(SDL_Texture* texture, const Rect& srcRect, const Rect& destRect);
    void drawSpriteFlipped(SDL_Texture* texture, int x, int y, bool flipH, bool flipV);

    // Sprite drawing with palette-based sprite data
    void drawSprite(const Sprite& sprite, int x, int y);

    // Primitive drawing
    void drawRect(const Rect& rect, const Color& color);
    void fillRect(const Rect& rect, const Color& color);
    void drawLine(int x1, int y1, int x2, int y2, const Color& color);
    void drawPoint(int x, int y, const Color& color);

    // Text rendering (bitmap font)
    void setFont(SDL_Texture* fontTexture, int charWidth, int charHeight);
    void drawText(const std::string& text, int x, int y, const Color& color = Color(255, 255, 255));
    int getTextWidth(const std::string& text) const;

    // Render targets
    std::unique_ptr<RenderTarget> createRenderTarget(int width, int height);
    void setRenderTarget(RenderTarget* target);
    void resetRenderTarget();

    // Clipping
    void setClipRect(const Rect& rect);
    void clearClipRect();

    // Palette effects
    void setPalette(const std::vector<uint32_t>& palette);
    void fadeIn(float progress);  // 0.0 = black, 1.0 = full color
    void fadeOut(float progress); // 0.0 = full color, 1.0 = black
    void flash(const Color& color, float intensity);

    // Dirty rectangle system
    void enableDirtyRects(bool enable) { useDirtyRects_ = enable; }
    void markDirty(const Rect& rect);
    void markFullDirty();

    // Screenshot
    bool saveScreenshot(const std::string& path);

    // Get last error
    std::string getLastError() const { return lastError_; }

private:
    void updateScale();
    SDL_Texture* createPalettedTexture(const Sprite& sprite);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    int windowWidth_ = 0;
    int windowHeight_ = 0;
    int scale_ = 1;
    bool fullscreen_ = false;

    // Font
    SDL_Texture* fontTexture_ = nullptr;
    int fontCharWidth_ = 8;
    int fontCharHeight_ = 8;

    // Dirty rectangles
    bool useDirtyRects_ = false;
    DirtyRectManager dirtyRects_;

    // Fade/flash effects
    float fadeLevel_ = 1.0f;
    Color flashColor_;
    float flashIntensity_ = 0.0f;

    // Current palette (for indexed color sprites)
    std::vector<uint32_t> palette_;

    std::string lastError_;
};

} // namespace opengg
