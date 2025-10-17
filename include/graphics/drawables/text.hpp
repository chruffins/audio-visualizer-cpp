#pragma once

#include <allegro5/allegro.h>

#include <string>
#include "util/font.hpp"
#include "graphics/uv.hpp"

struct ALLEGRO_FONT;

namespace ui {
class TextDrawable {
public:
    TextDrawable(const std::string& text, graphics::UV position, int font_size)
        : text(text), position(position), font_size(font_size), font() {}

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
        auto [x, y] = position.toScreenPos(); // Assuming a screen size of 800x600 for this example

        al_draw_text(lastUsedFont, al_map_rgb(255, 255, 255), x, y, textAlignment, text.c_str());
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