#pragma once

#include <allegro5/allegro.h>

#include <string>
#include <string_view>
#include <memory>
#include <cstdio> // for std::snprintf
#include <mutex>
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
        // sensible default font size
        font_size = 12;
        setFontSize(font_size);
    }

    // Main constructor
    TextDrawable(const std::string& text, graphics::UV position, graphics::UV size, int font_size)
        : text(text), position(position), size(size), font_size(font_size) {
        initializeFallbackFont();
        font = fallbackFont;
        setFontSize(font_size);
    }

    TextDrawable& setText(std::string_view new_text) {
        text = std::string(new_text);
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
        // Font metrics are calculated at draw time to avoid stale/dangling
        // cached pointers if the Font is reloaded. Keep setter lightweight.
        return *this;
    }

    int getFontSize() const {
        return font_size;
    }

    TextDrawable& setFont(std::shared_ptr<util::Font> new_font) {
        font = new_font;
        // Defer measuring the font until draw time for safety.
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

    enum class HorizontalAlignment {
        Left = ALLEGRO_ALIGN_LEFT,
        Center = ALLEGRO_ALIGN_CENTER,
        Right = ALLEGRO_ALIGN_RIGHT
    };

    TextDrawable& setAlignment(HorizontalAlignment alignment) {
        textAlignment = alignment;
        return *this;
    }

    HorizontalAlignment getAlignment() const noexcept {
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
    void drawF(const char* fmt, Args&&... args) {
        int needed = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
        if (needed <= 0) {
            return; // formatting failed or empty
        }
        std::string buffer;
        buffer.resize(static_cast<size_t>(needed) + 1);
        std::snprintf(buffer.data(), buffer.size(), fmt, std::forward<Args>(args)...);
        buffer.resize(static_cast<size_t>(needed));

        drawTextInternal(buffer.c_str());
    }

private:
    static void initializeFallbackFont();
    // Use std::once_flag to ensure thread-safe one-time initialization
    static std::once_flag fallbackInitFlag;
    void drawTextInternal(const char* str) const;
    
    static std::shared_ptr<util::Font> fallbackFont;
    
    std::string text;

    graphics::UV position;
    graphics::UV size;

    HorizontalAlignment textAlignment = HorizontalAlignment::Center;
    graphics::VerticalAlignment verticalAlignment = graphics::VerticalAlignment::CENTER;
    int line_height = 0;
    bool multiline = false;
    ALLEGRO_COLOR color = al_map_rgb(255, 255, 255);

    int font_size;
    
    std::shared_ptr<util::Font> font;
};
};