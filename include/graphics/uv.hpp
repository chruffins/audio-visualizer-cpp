#pragma once

#include <utility>
#include <allegro5/allegro.h>

namespace graphics {
struct UV {
    float abs_u;
    float abs_v;
    float pix_u;
    float pix_v;

    UV(float u = 0.0f, float v = 0.0f, float p_u = 0.0f, float p_v = 0.0f) : abs_u(u), abs_v(v), pix_u(p_u), pix_v(p_v) {}

    std::pair<float, float> getAbsolute() const {
        return {abs_u, abs_v};
    }

    std::pair<float, float> getPixel() const {
        return {pix_u, pix_v};
    }

    std::pair<float, float> toScreenPos(float textureWidth = 0, float textureHeight = 0) const {
        if (textureWidth == 0 || textureHeight == 0) {
            textureWidth = al_get_bitmap_width(al_get_target_bitmap());
            textureHeight = al_get_bitmap_height(al_get_target_bitmap());
        }
        
        return {abs_u * textureWidth + pix_u, abs_v * textureHeight + pix_v};
    }
};
} // namespace graphics