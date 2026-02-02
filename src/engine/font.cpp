#include "font.h"
#include "renderer.h"
#include <SDL.h>
#include <cstring>
#include <algorithm>
#include <sstream>

namespace opengg {

// Built-in 8x8 font bitmap data (ASCII 32-127)
// Each character is 8 bytes, one byte per row, MSB on left
static const uint8_t BUILTIN_FONT_8X8[] = {
    // Space (32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00,
    // " (34)
    0x6C, 0x6C, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
    // # (35)
    0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00,
    // $ (36)
    0x18, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x18, 0x00,
    // % (37)
    0x00, 0x66, 0xAC, 0xD8, 0x36, 0x6A, 0xCC, 0x00,
    // & (38)
    0x38, 0x6C, 0x68, 0x76, 0xDC, 0xCC, 0x76, 0x00,
    // ' (39)
    0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ( (40)
    0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00,
    // ) (41)
    0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00,
    // * (42)
    0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00,
    // + (43)
    0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00,
    // , (44)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30,
    // - (45)
    0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    // . (46)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
    // / (47)
    0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00,
    // 0 (48)
    0x7C, 0xC6, 0xCE, 0xD6, 0xE6, 0xC6, 0x7C, 0x00,
    // 1 (49)
    0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00,
    // 2 (50)
    0x7C, 0xC6, 0x06, 0x1C, 0x30, 0x66, 0xFE, 0x00,
    // 3 (51)
    0x7C, 0xC6, 0x06, 0x3C, 0x06, 0xC6, 0x7C, 0x00,
    // 4 (52)
    0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00,
    // 5 (53)
    0xFE, 0xC0, 0xFC, 0x06, 0x06, 0xC6, 0x7C, 0x00,
    // 6 (54)
    0x38, 0x60, 0xC0, 0xFC, 0xC6, 0xC6, 0x7C, 0x00,
    // 7 (55)
    0xFE, 0xC6, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00,
    // 8 (56)
    0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C, 0x00,
    // 9 (57)
    0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0x78, 0x00,
    // : (58)
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00,
    // ; (59)
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30,
    // < (60)
    0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00,
    // = (61)
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00,
    // > (62)
    0x60, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x60, 0x00,
    // ? (63)
    0x7C, 0xC6, 0x0C, 0x18, 0x18, 0x00, 0x18, 0x00,
    // @ (64)
    0x7C, 0xC6, 0xDE, 0xDE, 0xDE, 0xC0, 0x7C, 0x00,
    // A (65)
    0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0x00,
    // B (66)
    0xFC, 0x66, 0x66, 0x7C, 0x66, 0x66, 0xFC, 0x00,
    // C (67)
    0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C, 0x00,
    // D (68)
    0xF8, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00,
    // E (69)
    0xFE, 0x62, 0x68, 0x78, 0x68, 0x62, 0xFE, 0x00,
    // F (70)
    0xFE, 0x62, 0x68, 0x78, 0x68, 0x60, 0xF0, 0x00,
    // G (71)
    0x3C, 0x66, 0xC0, 0xC0, 0xCE, 0x66, 0x3A, 0x00,
    // H (72)
    0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0x00,
    // I (73)
    0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // J (74)
    0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0x00,
    // K (75)
    0xE6, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0xE6, 0x00,
    // L (76)
    0xF0, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00,
    // M (77)
    0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6, 0xC6, 0x00,
    // N (78)
    0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00,
    // O (79)
    0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    // P (80)
    0xFC, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00,
    // Q (81)
    0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xCE, 0x7C, 0x0E,
    // R (82)
    0xFC, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0xE6, 0x00,
    // S (83)
    0x7C, 0xC6, 0x60, 0x38, 0x0C, 0xC6, 0x7C, 0x00,
    // T (84)
    0x7E, 0x7E, 0x5A, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // U (85)
    0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    // V (86)
    0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00,
    // W (87)
    0xC6, 0xC6, 0xC6, 0xD6, 0xD6, 0xFE, 0x6C, 0x00,
    // X (88)
    0xC6, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0xC6, 0x00,
    // Y (89)
    0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x00,
    // Z (90)
    0xFE, 0xC6, 0x8C, 0x18, 0x32, 0x66, 0xFE, 0x00,
    // [ (91)
    0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00,
    // \ (92)
    0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00,
    // ] (93)
    0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00,
    // ^ (94)
    0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00,
    // _ (95)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    // ` (96)
    0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    // a (97)
    0x00, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00,
    // b (98)
    0xE0, 0x60, 0x7C, 0x66, 0x66, 0x66, 0xDC, 0x00,
    // c (99)
    0x00, 0x00, 0x7C, 0xC6, 0xC0, 0xC6, 0x7C, 0x00,
    // d (100)
    0x1C, 0x0C, 0x7C, 0xCC, 0xCC, 0xCC, 0x76, 0x00,
    // e (101)
    0x00, 0x00, 0x7C, 0xC6, 0xFE, 0xC0, 0x7C, 0x00,
    // f (102)
    0x3C, 0x66, 0x60, 0xF8, 0x60, 0x60, 0xF0, 0x00,
    // g (103)
    0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0x78,
    // h (104)
    0xE0, 0x60, 0x6C, 0x76, 0x66, 0x66, 0xE6, 0x00,
    // i (105)
    0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // j (106)
    0x06, 0x00, 0x0E, 0x06, 0x06, 0x66, 0x66, 0x3C,
    // k (107)
    0xE0, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0xE6, 0x00,
    // l (108)
    0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // m (109)
    0x00, 0x00, 0xEC, 0xFE, 0xD6, 0xD6, 0xD6, 0x00,
    // n (110)
    0x00, 0x00, 0xDC, 0x66, 0x66, 0x66, 0x66, 0x00,
    // o (111)
    0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0x00,
    // p (112)
    0x00, 0x00, 0xDC, 0x66, 0x66, 0x7C, 0x60, 0xF0,
    // q (113)
    0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0x1E,
    // r (114)
    0x00, 0x00, 0xDC, 0x76, 0x60, 0x60, 0xF0, 0x00,
    // s (115)
    0x00, 0x00, 0x7C, 0xC0, 0x7C, 0x06, 0xFC, 0x00,
    // t (116)
    0x30, 0x30, 0xFC, 0x30, 0x30, 0x36, 0x1C, 0x00,
    // u (117)
    0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00,
    // v (118)
    0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00,
    // w (119)
    0x00, 0x00, 0xC6, 0xD6, 0xD6, 0xFE, 0x6C, 0x00,
    // x (120)
    0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00,
    // y (121)
    0x00, 0x00, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x7C,
    // z (122)
    0x00, 0x00, 0xFE, 0x8C, 0x18, 0x32, 0xFE, 0x00,
    // { (123)
    0x0E, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0E, 0x00,
    // | (124)
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
    // } (125)
    0x70, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x70, 0x00,
    // ~ (126)
    0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// BitmapFont implementation
BitmapFont::BitmapFont() {
    defaultGlyph_.x = 0;
    defaultGlyph_.y = 0;
    defaultGlyph_.width = 8;
    defaultGlyph_.height = 8;
    defaultGlyph_.xOffset = 0;
    defaultGlyph_.yOffset = 0;
    defaultGlyph_.advance = 8;
}

BitmapFont::~BitmapFont() {
    if (ownsTexture_ && texture_) {
        SDL_DestroyTexture(texture_);
    }
}

bool BitmapFont::createBuiltin(SDL_Renderer* renderer) {
    if (!renderer) return false;

    std::vector<uint8_t> pixels;
    int width, height;
    generateBuiltinBitmap(pixels, width, height);

    // Create texture
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);

    if (!surface) return false;

    // Copy pixels
    memcpy(surface->pixels, pixels.data(), pixels.size());

    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture_) return false;

    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);

    charWidth_ = 8;
    charHeight_ = 8;
    lineHeight_ = 10;
    charsPerRow_ = 16;
    startChar_ = 32;
    ownsTexture_ = true;

    // Setup glyphs for ASCII 32-127
    glyphs_.resize(128);
    for (int c = 32; c < 128; ++c) {
        int idx = c - 32;
        Glyph& g = glyphs_[c];
        g.x = (idx % charsPerRow_) * charWidth_;
        g.y = (idx / charsPerRow_) * charHeight_;
        g.width = charWidth_;
        g.height = charHeight_;
        g.xOffset = 0;
        g.yOffset = 0;
        g.advance = charWidth_;
    }

    return true;
}

void BitmapFont::generateBuiltinBitmap(std::vector<uint8_t>& pixels, int& width, int& height) {
    // 16 chars per row, 6 rows for 96 characters (32-127)
    width = 16 * 8;   // 128 pixels
    height = 6 * 8;   // 48 pixels

    pixels.resize(width * height * 4, 0);

    int numChars = sizeof(BUILTIN_FONT_8X8) / 8;

    for (int c = 0; c < numChars && c < 96; ++c) {
        int charX = (c % 16) * 8;
        int charY = (c / 16) * 8;

        for (int row = 0; row < 8; ++row) {
            uint8_t rowBits = BUILTIN_FONT_8X8[c * 8 + row];

            for (int col = 0; col < 8; ++col) {
                bool set = (rowBits & (0x80 >> col)) != 0;

                int px = charX + col;
                int py = charY + row;
                int idx = (py * width + px) * 4;

                if (set) {
                    pixels[idx + 0] = 255;  // R
                    pixels[idx + 1] = 255;  // G
                    pixels[idx + 2] = 255;  // B
                    pixels[idx + 3] = 255;  // A
                } else {
                    pixels[idx + 3] = 0;    // Transparent
                }
            }
        }
    }
}

bool BitmapFont::loadFromTexture(SDL_Texture* texture, int charWidth, int charHeight,
                                  int charsPerRow, int startChar) {
    texture_ = texture;
    charWidth_ = charWidth;
    charHeight_ = charHeight;
    lineHeight_ = charHeight + 2;
    charsPerRow_ = charsPerRow;
    startChar_ = startChar;
    ownsTexture_ = false;

    // Setup glyphs
    glyphs_.resize(256);
    for (int c = startChar; c < 256; ++c) {
        int idx = c - startChar;
        Glyph& g = glyphs_[c];
        g.x = (idx % charsPerRow) * charWidth;
        g.y = (idx / charsPerRow) * charHeight;
        g.width = charWidth;
        g.height = charHeight;
        g.xOffset = 0;
        g.yOffset = 0;
        g.advance = charWidth;
    }

    return true;
}

bool BitmapFont::loadFromGameData(const std::vector<uint8_t>& data, SDL_Renderer* renderer) {
    // TODO: Parse original game font format
    // For now, fall back to builtin
    return createBuiltin(renderer);
}

int BitmapFont::getTextWidth(const std::string& text) const {
    int width = 0;
    int maxWidth = 0;

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, width);
            width = 0;
        } else {
            const Glyph& g = getGlyph(c);
            width += g.advance;
        }
    }

    return std::max(maxWidth, width);
}

int BitmapFont::getTextHeight(const std::string& text) const {
    int lines = 1;
    for (char c : text) {
        if (c == '\n') lines++;
    }
    return lines * lineHeight_;
}

const Glyph& BitmapFont::getGlyph(char c) const {
    unsigned char uc = static_cast<unsigned char>(c);
    if (uc < glyphs_.size() && glyphs_[uc].width > 0) {
        return glyphs_[uc];
    }
    return defaultGlyph_;
}

// TextRenderer implementation
TextRenderer::TextRenderer() = default;
TextRenderer::~TextRenderer() { shutdown(); }

bool TextRenderer::initialize(SDL_Renderer* renderer) {
    sdlRenderer_ = renderer;

    if (!defaultFont_.createBuiltin(renderer)) {
        return false;
    }

    currentFont_ = &defaultFont_;
    return true;
}

void TextRenderer::shutdown() {
    // defaultFont_ cleans itself up
    currentFont_ = nullptr;
    sdlRenderer_ = nullptr;
}

void TextRenderer::drawText(Renderer* renderer, const std::string& text, int x, int y,
                            const TextColor& color) {
    if (!currentFont_ || !currentFont_->isValid()) return;

    SDL_Texture* tex = currentFont_->getTexture();
    SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(tex, color.a);

    int curX = x;
    int curY = y;

    for (char c : text) {
        if (c == '\n') {
            curX = x;
            curY += currentFont_->getLineHeight();
            continue;
        }

        const Glyph& g = currentFont_->getGlyph(c);

        SDL_Rect src = {g.x, g.y, g.width, g.height};
        SDL_Rect dst = {curX + g.xOffset, curY + g.yOffset, g.width, g.height};

        SDL_RenderCopy(sdlRenderer_, tex, &src, &dst);

        curX += g.advance;
    }
}

void TextRenderer::drawTextAligned(Renderer* renderer, const std::string& text,
                                    int x, int y, int width, TextAlign align,
                                    const TextColor& color) {
    if (!currentFont_) return;

    int textWidth = currentFont_->getTextWidth(text);
    int drawX = x;

    switch (align) {
        case TextAlign::Center:
            drawX = x + (width - textWidth) / 2;
            break;
        case TextAlign::Right:
            drawX = x + width - textWidth;
            break;
        default:
            break;
    }

    drawText(renderer, text, drawX, y, color);
}

void TextRenderer::drawTextShadow(Renderer* renderer, const std::string& text, int x, int y,
                                   const TextColor& color, const TextColor& shadowColor,
                                   int shadowOffsetX, int shadowOffsetY) {
    // Draw shadow
    drawText(renderer, text, x + shadowOffsetX, y + shadowOffsetY, shadowColor);
    // Draw main text
    drawText(renderer, text, x, y, color);
}

void TextRenderer::drawTextOutline(Renderer* renderer, const std::string& text, int x, int y,
                                    const TextColor& color, const TextColor& outlineColor) {
    // Draw outline in 8 directions
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx != 0 || dy != 0) {
                drawText(renderer, text, x + dx, y + dy, outlineColor);
            }
        }
    }
    // Draw main text
    drawText(renderer, text, x, y, color);
}

void TextRenderer::drawTextWrapped(Renderer* renderer, const std::string& text,
                                    int x, int y, int maxWidth, const TextColor& color) {
    auto lines = wrapText(text, maxWidth);

    int curY = y;
    for (const auto& line : lines) {
        drawText(renderer, line, x, curY, color);
        curY += currentFont_->getLineHeight();
    }
}

std::vector<std::string> TextRenderer::wrapText(const std::string& text, int maxWidth) const {
    std::vector<std::string> lines;
    if (!currentFont_ || maxWidth <= 0) {
        lines.push_back(text);
        return lines;
    }

    std::istringstream stream(text);
    std::string word;
    std::string currentLine;

    while (stream >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;

        if (currentFont_->getTextWidth(testLine) <= maxWidth) {
            currentLine = testLine;
        } else {
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
            }
            currentLine = word;
        }
    }

    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }

    return lines;
}

int TextRenderer::measureText(const std::string& text) const {
    if (!currentFont_) return 0;
    return currentFont_->getTextWidth(text);
}

int TextRenderer::measureTextHeight(const std::string& text, int maxWidth) const {
    if (!currentFont_) return 0;

    if (maxWidth > 0) {
        auto lines = wrapText(text, maxWidth);
        return static_cast<int>(lines.size()) * currentFont_->getLineHeight();
    }

    return currentFont_->getTextHeight(text);
}

} // namespace opengg
