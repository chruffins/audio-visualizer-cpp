#include "graphics/drawables/scrollable_frame.hpp"

#include <algorithm>
#include <allegro5/allegro.h>
#include <iostream>

namespace ui {

void ScrollableFrameDrawable::draw(const graphics::RenderContext& context) const {
    auto sizePx = size.toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto posPx = position.toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));

    const float absX = posPx.first + context.offsetX;
    const float absY = posPx.second + context.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;

    int oldClipX = 0, oldClipY = 0, oldClipW = 0, oldClipH = 0;
    al_get_clipping_rectangle(&oldClipX, &oldClipY, &oldClipW, &oldClipH);
    al_set_clipping_rectangle(static_cast<int>(absX), static_cast<int>(absY), static_cast<int>(width), static_cast<int>(height));

    // Draw background and border (inherited from ContainerDrawable)
    if (drawBackground) {
        al_draw_filled_rectangle(absX, absY, absX + width, absY + height, backgroundColor);
    }
    if (borderThickness > 0) {
        al_draw_rectangle(absX, absY, absX + width, absY + height, borderColor, static_cast<float>(borderThickness));
    }

    // Calculate viewport dimensions accounting for padding
    const float viewportWidth = width - 2 * padding;
    const float viewportHeight = height - 2 * padding;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    const float maxScroll = std::max(0.0f, effectiveContentHeight - viewportHeight);
    
    // Clamp scroll offset to valid range
    float clampedScroll = scrollOffset;
    if (clampedScroll < 0.0f) clampedScroll = 0.0f;
    if (clampedScroll > maxScroll) clampedScroll = maxScroll;

    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = context;
    childContext.screenWidth = static_cast<int>(viewportWidth);
    childContext.screenHeight = static_cast<int>(viewportHeight);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding - clampedScroll;

    drawChildren(childContext);

    // Draw scrollbar indicator if content exceeds viewport
    if (showScrollbar && maxScroll > 0.0f && scrollbarWidth > 0.0f) {
        const float ratio = viewportHeight / effectiveContentHeight;
        const float indicatorHeight = std::max(10.0f, viewportHeight * ratio);
        const float scrollRatio = clampedScroll / maxScroll;
        const float indicatorY = absY + padding + scrollRatio * (viewportHeight - indicatorHeight);
        const float indicatorX = absX + width - scrollbarWidth; // - padding
        al_draw_filled_rectangle(indicatorX, indicatorY, indicatorX + scrollbarWidth, indicatorY + indicatorHeight, scrollbarColor);
    }

    al_set_clipping_rectangle(oldClipX, oldClipY, oldClipW, oldClipH);
}

bool ScrollableFrameDrawable::onMouseScroll(float /*dx*/, float dy) {
    // Allegro: positive dy means scroll up; decrease offset to move content down
    float displayW = static_cast<float>(al_get_display_width(al_get_current_display()));
    float displayH = static_cast<float>(al_get_display_height(al_get_current_display()));
    const float maxScroll = computeMaxScroll(displayW, displayH);

    scrollOffset -= dy * scrollStep;
    if (scrollOffset < 0.0f) scrollOffset = 0.0f;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    return true; // consume scroll
}

bool ScrollableFrameDrawable::hitTest(float x, float y) const {
    float displayW = static_cast<float>(al_get_display_width(al_get_current_display()));
    float displayH = static_cast<float>(al_get_display_height(al_get_current_display()));
    auto posPx = position.toScreenPos(displayW, displayH);
    auto sizePx = size.toScreenPos(displayW, displayH);
    const float absX = posPx.first;
    const float absY = posPx.second;
    return x >= absX && x <= absX + sizePx.first && y >= absY && y <= absY + sizePx.second;
}

float ScrollableFrameDrawable::computeMaxScroll(float screenW, float screenH) const noexcept {
    auto sizePx = size.toScreenPos(screenW, screenH);
    const float viewportHeight = sizePx.second - 2 * padding;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    return std::max(0.0f, effectiveContentHeight - viewportHeight);
}

} // namespace ui
