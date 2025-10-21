#pragma once

#include <allegro5/allegro.h>

#include <string>
#include <memory>
#include <cstdio> // for std::snprintf
#include "util/font.hpp"
#include "graphics/uv.hpp"
#include "graphics/alignment.hpp"
#include "graphics/drawable.hpp"

struct ALLEGRO_FONT;

namespace ui {
class TextDrawable : public graphics::Drawable {
public:
    TextDrawable() {
        initializeFallbackFont();
        font = fallbackFont;
    }

    // Main constructor
    TextDrawable(const std::string& text, graphics::UV position, graphics::UV size, int font_size)
        : text(text), position(position), size(size), font_size(font_size), lastUsedFont(nullptr) {
        initializeFallbackFont();
        font = fallbackFont;
        setFontSize(font_size);
    }

    TextDrawable& setText(const std::string& new_text) {
        text = new_text;
        return *this;
    }

    const std::string& getText() const {
        return text;
    }

    TextDrawable& setPosition(graphics::UV new_position) {
        position = new_position;
        return *this;
    }

    TextDrawable& setFontSize(int new_size) {
        font_size = new_size;
        lastUsedFont = font->getFont(font_size);
        _font_height = al_get_font_line_height(lastUsedFont);
        return *this;
    }

    int getFontSize() const {
        return font_size;
    }

    TextDrawable& setFont(std::shared_ptr<util::Font> new_font) {
        font = new_font;
        if (font) {
            lastUsedFont = font->getFont(font_size);
            _font_height = al_get_font_line_height(lastUsedFont);
        }
        return *this;
    }

    TextDrawable& setColor(ALLEGRO_COLOR new_color) {
        color = new_color;
        return *this;
    }

    ALLEGRO_COLOR getColor() const {
        return color;
    }

    TextDrawable& setMultiline(bool enable) {
        multiline = enable;
        return *this;
    }

    bool isMultiline() const {
        return multiline;
    }

    TextDrawable& setLineHeight(int height) {
        line_height = height;
        return *this;
    }

    int getLineHeight() const {
        return line_height;
    }

    TextDrawable& setSize(const graphics::UV& new_size) {
        size = new_size;
        return *this;
    }

    const graphics::UV& getSize() const {
        return size;
    }

    TextDrawable& setAlignment(int alignment) {
        textAlignment = alignment;
        return *this;
    }

    int getAlignment() const {
        return textAlignment;
    }

    TextDrawable& setVerticalAlignment(graphics::VerticalAlignment alignment) {
        verticalAlignment = alignment;
        return *this;
    }

    graphics::VerticalAlignment getVerticalAlignment() const {
        return verticalAlignment;
    }

    void draw(const graphics::RenderContext& context = {}) const override;

    template <typename... Args>
    void drawF(const char* fmt, Args&&... args);

private:
    static void initializeFallbackFont();
    void drawTextInternal(const char* str) const;
    
    static std::shared_ptr<util::Font> fallbackFont;
    static bool fallbackFontInitialized;
    
    std::string text;

    graphics::UV position;
    graphics::UV size;

    int textAlignment = ALLEGRO_ALIGN_CENTER;
    graphics::VerticalAlignment verticalAlignment = graphics::VerticalAlignment::CENTER;
    int line_height = 0;
    bool multiline = false;
    ALLEGRO_COLOR color = al_map_rgb(255, 255, 255);

    int font_size;
    int _font_height;
    ALLEGRO_FONT* lastUsedFont;
    std::shared_ptr<util::Font> font;
};
};