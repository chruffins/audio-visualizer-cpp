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
    al_set_clipping_rectangle(static_cast<int>(absX), static_cast<int>(absY), static_cast<int>(width), static_cast<int>(height));

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

    al_set_clipping_rectangle(oldClipX, oldClipY, oldClipW, oldClipH);
}

} // namespace ui
