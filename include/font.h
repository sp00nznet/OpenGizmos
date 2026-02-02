#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct SDL_Texture;
struct SDL_Renderer;

namespace opengg {

class Renderer;

// Font style flags
enum class FontStyle : uint8_t {
    Normal = 0,
    Bold = 1,
    Shadow = 2,
    Outline = 4
};

inline FontStyle operator|(FontStyle a, FontStyle b) {
    return static_cast<FontStyle>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool hasStyle(FontStyle flags, FontStyle test) {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(test)) != 0;
}

// Text alignment
enum class TextAlign {
    Left,
    Center,
    Right
};

// Color for text
struct TextColor {
    uint8_t r, g, b, a;

    TextColor() : r(255), g(255), b(255), a(255) {}
    TextColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    static TextColor white() { return TextColor(255, 255, 255); }
    static TextColor black() { return TextColor(0, 0, 0); }
    static TextColor yellow() { return TextColor(255, 255, 0); }
    static TextColor red() { return TextColor(255, 0, 0); }
    static TextColor green() { return TextColor(0, 255, 0); }
    static TextColor blue() { return TextColor(0, 0, 255); }
    static TextColor gray() { return TextColor(128, 128, 128); }
};

// Glyph info for variable-width fonts
struct Glyph {
    int x, y;           // Position in texture
    int width, height;  // Size of glyph
    int xOffset;        // Horizontal offset when rendering
    int yOffset;        // Vertical offset when rendering
    int advance;        // How far to move cursor after this glyph
};

// Bitmap Font class
class BitmapFont {
public:
    BitmapFont();
    ~BitmapFont();

    // Create built-in 8x8 font
    bool createBuiltin(SDL_Renderer* renderer);

    // Load from texture (assumes fixed-width grid layout)
    bool loadFromTexture(SDL_Texture* texture, int charWidth, int charHeight,
                         int charsPerRow = 16, int startChar = 32);

    // Load from game font data
    bool loadFromGameData(const std::vector<uint8_t>& data, SDL_Renderer* renderer);

    // Get font metrics
    int getCharWidth() const { return charWidth_; }
    int getCharHeight() const { return charHeight_; }
    int getLineHeight() const { return lineHeight_; }

    // Measure text
    int getTextWidth(const std::string& text) const;
    int getTextHeight(const std::string& text) const;

    // Get texture for rendering
    SDL_Texture* getTexture() const { return texture_; }

    // Get glyph info
    const Glyph& getGlyph(char c) const;

    // Check if valid
    bool isValid() const { return texture_ != nullptr; }

private:
    void generateBuiltinBitmap(std::vector<uint8_t>& pixels, int& width, int& height);

    SDL_Texture* texture_ = nullptr;
    int charWidth_ = 8;
    int charHeight_ = 8;
    int lineHeight_ = 10;
    int charsPerRow_ = 16;
    int startChar_ = 32;

    std::vector<Glyph> glyphs_;
    Glyph defaultGlyph_;

    bool ownsTexture_ = false;
};

// Text renderer - handles drawing text with fonts
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Initialize with renderer
    bool initialize(SDL_Renderer* renderer);

    // Shutdown
    void shutdown();

    // Set current font
    void setFont(BitmapFont* font) { currentFont_ = font; }
    BitmapFont* getFont() const { return currentFont_; }

    // Get default font
    BitmapFont* getDefaultFont() { return &defaultFont_; }

    // Draw text
    void drawText(Renderer* renderer, const std::string& text, int x, int y,
                  const TextColor& color = TextColor::white());

    // Draw text with alignment
    void drawTextAligned(Renderer* renderer, const std::string& text,
                         int x, int y, int width,
                         TextAlign align = TextAlign::Left,
                         const TextColor& color = TextColor::white());

    // Draw text with shadow
    void drawTextShadow(Renderer* renderer, const std::string& text, int x, int y,
                        const TextColor& color = TextColor::white(),
                        const TextColor& shadowColor = TextColor::black(),
                        int shadowOffsetX = 1, int shadowOffsetY = 1);

    // Draw text with outline
    void drawTextOutline(Renderer* renderer, const std::string& text, int x, int y,
                         const TextColor& color = TextColor::white(),
                         const TextColor& outlineColor = TextColor::black());

    // Draw wrapped text (word wrap)
    void drawTextWrapped(Renderer* renderer, const std::string& text,
                         int x, int y, int maxWidth,
                         const TextColor& color = TextColor::white());

    // Measure text with current font
    int measureText(const std::string& text) const;
    int measureTextHeight(const std::string& text, int maxWidth = 0) const;

private:
    void drawChar(Renderer* renderer, char c, int x, int y, const TextColor& color);
    std::vector<std::string> wrapText(const std::string& text, int maxWidth) const;

    SDL_Renderer* sdlRenderer_ = nullptr;
    BitmapFont defaultFont_;
    BitmapFont* currentFont_ = nullptr;
};

} // namespace opengg
