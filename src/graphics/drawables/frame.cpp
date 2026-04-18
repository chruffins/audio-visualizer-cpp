#include "graphics/drawables/frame.hpp"
#include "graphics/clipping.hpp"
#include "graphics/draw_shapes.hpp"

#include <algorithm>

namespace ui {

void FrameDrawable::draw(const graphics::RenderContext& context) const {
    // Culling: skip off-screen frames
    if (isOffScreen(context)) {
        return;
    }
    
    auto sizePx = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto posPx = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));

    const float absX = posPx.first + context.offsetX;
    const float absY = posPx.second + context.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;

    const auto oldClip = graphics::getCurrentClipRect();
    const graphics::ClipRect frameClip{
        static_cast<int>(absX),
        static_cast<int>(absY),
        static_cast<int>(width),
        static_cast<int>(height),
    };
    graphics::setIntersectedClipRect(oldClip, frameClip);
    al_hold_bitmap_drawing(true);

    if (drawBackground) {
        graphics::drawFilledRectWithBorder(
            absX,
            absY,
            width,
            height,
            backgroundColor,
            borderColor,
            static_cast<float>(borderThickness)
        );
    } else if (borderThickness > 0) {
        al_draw_rectangle(absX, absY, absX + width, absY + height, borderColor, static_cast<float>(borderThickness));
    }

    graphics::RenderContext childContext = context;
    childContext.screenWidth = static_cast<int>(width - 2 * padding);
    childContext.screenHeight = static_cast<int>(height - 2 * padding);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding;

    drawChildren(childContext);
    al_hold_bitmap_drawing(false);
    graphics::setClipRect(oldClip);
}

} // namespace ui
