#pragma once

#include <allegro5/allegro.h>

#include <algorithm>

namespace graphics {

struct ClipRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

inline ClipRect getCurrentClipRect() {
    ClipRect rect;
    al_get_clipping_rectangle(&rect.x, &rect.y, &rect.w, &rect.h);
    return rect;
}

inline void setClipRect(const ClipRect& rect) {
    al_set_clipping_rectangle(rect.x, rect.y, rect.w, rect.h);
}

inline ClipRect intersectClipRect(const ClipRect& base, const ClipRect& requested) {
    ClipRect result = requested;

    if (base.w > 0 && base.h > 0) {
        const int clipRight = std::min(base.x + base.w, requested.x + requested.w);
        const int clipBottom = std::min(base.y + base.h, requested.y + requested.h);
        result.x = std::max(base.x, requested.x);
        result.y = std::max(base.y, requested.y);
        result.w = std::max(0, clipRight - result.x);
        result.h = std::max(0, clipBottom - result.y);
    }

    return result;
}

inline void setIntersectedClipRect(const ClipRect& base, const ClipRect& requested) {
    setClipRect(intersectClipRect(base, requested));
}

} // namespace graphics