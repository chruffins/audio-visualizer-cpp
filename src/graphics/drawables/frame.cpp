#include "graphics/drawables/frame.hpp"

#include <algorithm>

namespace ui {

void FrameDrawable::draw(const graphics::RenderContext& context) const {
    auto sizePx = size.toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto posPx = position.toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));

    const float absX = posPx.first + context.offsetX;
    const float absY = posPx.second + context.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;

    int oldClipX = 0, oldClipY = 0, oldClipW = 0, oldClipH = 0;
    al_get_clipping_rectangle(&oldClipX, &oldClipY, &oldClipW, &oldClipH);
    
    // Intersect with existing clipping rectangle to respect parent bounds
    int newClipX = static_cast<int>(absX);
    int newClipY = static_cast<int>(absY);
    int newClipW = static_cast<int>(width);
    int newClipH = static_cast<int>(height);
    
    // Intersect with old clip bounds
    if (oldClipW > 0 && oldClipH > 0) {
        int clipRight = std::min(oldClipX + oldClipW, newClipX + newClipW);
        int clipBottom = std::min(oldClipY + oldClipH, newClipY + newClipH);
        newClipX = std::max(oldClipX, newClipX);
        newClipY = std::max(oldClipY, newClipY);
        newClipW = std::max(0, clipRight - newClipX);
        newClipH = std::max(0, clipBottom - newClipY);
    }
    
    al_set_clipping_rectangle(newClipX, newClipY, newClipW, newClipH);
    al_hold_bitmap_drawing(true);

    if (drawBackground) {
        al_draw_filled_rectangle(absX, absY, absX + width, absY + height, backgroundColor);
    }
    if (borderThickness > 0) {
        al_draw_rectangle(absX, absY, absX + width, absY + height, borderColor, static_cast<float>(borderThickness));
    }

    graphics::RenderContext childContext = context;
    childContext.screenWidth = static_cast<int>(width - 2 * padding);
    childContext.screenHeight = static_cast<int>(height - 2 * padding);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding;

    drawChildren(childContext);
    al_hold_bitmap_drawing(false);
    al_set_clipping_rectangle(oldClipX, oldClipY, oldClipW, oldClipH);
}

} // namespace ui
