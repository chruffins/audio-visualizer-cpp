#pragma once

#include <allegro5/allegro.h>

namespace graphics {

inline ALLEGRO_COLOR selectStateColor(bool enabled,
                                      bool pressed,
                                      bool hovered,
                                      ALLEGRO_COLOR normalColor,
                                      ALLEGRO_COLOR hoverColor,
                                      ALLEGRO_COLOR pressedColor,
                                      ALLEGRO_COLOR disabledColor) {
    if (!enabled) {
        return disabledColor;
    }
    if (pressed) {
        return pressedColor;
    }
    if (hovered) {
        return hoverColor;
    }
    return normalColor;
}

} // namespace graphics