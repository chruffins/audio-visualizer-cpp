#pragma once

#include <allegro5/allegro_primitives.h>

namespace graphics {

inline void drawFilledRectWithBorder(float x,
                                     float y,
                                     float w,
                                     float h,
                                     ALLEGRO_COLOR fillColor,
                                     ALLEGRO_COLOR borderColor,
                                     float borderThickness) {
    al_draw_filled_rectangle(x, y, x + w, y + h, fillColor);
    if (borderThickness > 0.0f) {
        al_draw_rectangle(x, y, x + w, y + h, borderColor, borderThickness);
    }
}

} // namespace graphics