#pragma once

#include <allegro5/allegro.h>

#include <string>
#include <cstdio> // for std::snprintf
#include "util/font.hpp"
#include "graphics/uv.hpp"

struct ALLEGRO_FONT;

namespace ui {
class TextDrawable {
public:
    TextDrawable() = default;

    TextDrawable(const std::string& text, graphics::UV position, int font_size)
        : text(text), position(position), font_size(font_size), lastUsedFont(nullptr), font() {}

    void setText(const std::string& new_text) {
        text = new_text;
    }

    const std::string& getText() const {
        return text;
    }

    void setPosition(graphics::UV new_position) {
        position = new_position;
    }

    void setFontSize(int new_size) {
        font_size = new_size;
        lastUsedFont = font.getFont(font_size);
    }

    int getFontSize() const {
        return font_size;
    }

    void draw() {
        if (!lastUsedFont) {
            lastUsedFont = font.getFont(font_size);
        }
        auto [x, y] = position.toScreenPos();

        al_draw_text(lastUsedFont, al_map_rgb(255, 255, 255), x, y, textAlignment, text.c_str());
    }

    template <typename... Args>
    void drawF(const char* fmt, Args&&... args) {
        if (!lastUsedFont) {
            lastUsedFont = font.getFont(font_size);
        }
        auto [x, y] = position.toScreenPos();

        // Format into a temporary buffer using snprintf (two-pass)
        int needed = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
        if (needed <= 0) {
            return; // formatting failed or empty
        }
        std::string buffer;
        buffer.resize(static_cast<size_t>(needed) + 1);
        std::snprintf(buffer.data(), buffer.size(), fmt, std::forward<Args>(args)...);
        buffer.resize(static_cast<size_t>(needed)); // drop trailing null for c_str safety

        al_draw_text(lastUsedFont, al_map_rgb(255, 255, 255), x, y, textAlignment, buffer.c_str());
    }

private:
    std::string text;

    graphics::UV position;
    int textAlignment = ALLEGRO_ALIGN_CENTER;

    int font_size;
    ALLEGRO_FONT* lastUsedFont;
    util::Font font;
};
};