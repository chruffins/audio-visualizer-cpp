#include "graphics/drawables/container.hpp"

#include <algorithm>

namespace ui {

// Static cache for batch clipping optimization
int ContainerDrawable::s_lastClipX = -1;
int ContainerDrawable::s_lastClipY = -1;
int ContainerDrawable::s_lastClipW = -1;
int ContainerDrawable::s_lastClipH = -1;

void ContainerDrawable::draw(const graphics::RenderContext& context) const {
    auto sizePx = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto posPx = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));

    const float absX = posPx.first + context.offsetX;
    const float absY = posPx.second + context.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;

    int oldClipX = 0, oldClipY = 0, oldClipW = 0, oldClipH = 0;
    al_get_clipping_rectangle(&oldClipX, &oldClipY, &oldClipW, &oldClipH);
    
    // Batch clipping: only set if rectangle changed
    int newClipX = static_cast<int>(absX);
    int newClipY = static_cast<int>(absY);
    int newClipW = static_cast<int>(width);
    int newClipH = static_cast<int>(height);
    
    if (s_lastClipX != newClipX || s_lastClipY != newClipY || 
        s_lastClipW != newClipW || s_lastClipH != newClipH) {
        al_set_clipping_rectangle(newClipX, newClipY, newClipW, newClipH);
        s_lastClipX = newClipX;
        s_lastClipY = newClipY;
        s_lastClipW = newClipW;
        s_lastClipH = newClipH;
    }

    if (drawBackground) {
        al_draw_filled_rectangle(absX, absY, absX + width, absY + height, backgroundColor);
    }
    if (borderThickness > 0) {
        al_draw_rectangle(absX, absY, absX + width, absY + height, borderColor, static_cast<float>(borderThickness));
    }

    graphics::RenderContext childContext = context;
    childContext.screenWidth = static_cast<int>(width);
    childContext.screenHeight = static_cast<int>(height);
    childContext.offsetX = absX;
    childContext.offsetY = absY;

    drawChildren(childContext);

    al_set_clipping_rectangle(oldClipX, oldClipY, oldClipW, oldClipH);
}

void ContainerDrawable::drawChildren(const graphics::RenderContext& childContext) const {
    for (const auto& child : children) {
        if (child) {
            child->draw(childContext);
        }
    }
}

void ScrollContainerDrawable::draw(const graphics::RenderContext& context) const {
    auto sizePx = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto posPx = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));

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

    const float viewportHeight = height;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    const float maxScroll = std::max(0.0f, effectiveContentHeight - viewportHeight);
    float clampedScroll = scrollOffset;
    if (clampedScroll < 0.0f) clampedScroll = 0.0f;
    if (clampedScroll > maxScroll) clampedScroll = maxScroll;

    graphics::RenderContext childContext = context;
    childContext.screenWidth = static_cast<int>(width);
    childContext.screenHeight = static_cast<int>(height);
    childContext.offsetX = absX;
    childContext.offsetY = absY - clampedScroll;

    drawChildren(childContext);

    if (showScrollbar && maxScroll > 0.0f && scrollbarWidth > 0.0f) {
        const float ratio = viewportHeight / effectiveContentHeight;
        const float indicatorHeight = std::max(10.0f, viewportHeight * ratio);
        const float scrollRatio = clampedScroll / maxScroll;
        const float indicatorY = absY + scrollRatio * (viewportHeight - indicatorHeight);
        const float indicatorX = absX + width - scrollbarWidth;
        al_draw_filled_rectangle(indicatorX, indicatorY, indicatorX + scrollbarWidth, indicatorY + indicatorHeight, scrollbarColor);
    }

    al_set_clipping_rectangle(oldClipX, oldClipY, oldClipW, oldClipH);
}

} // namespace ui
