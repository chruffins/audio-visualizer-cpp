#include "graphics/drawables/scrollable_frame.hpp"

#include <algorithm>
#include <allegro5/allegro.h>
#include <iostream>

namespace ui {

void ScrollableFrameDrawable::draw(const graphics::RenderContext& context) const {
    auto sizePx = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto posPx = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));

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
    
    // Cache viewport height for use in event handlers
    const_cast<ScrollableFrameDrawable*>(this)->cachedViewportHeight = viewportHeight;
    
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

bool ScrollableFrameDrawable::onMouseDown(const graphics::MouseEvent& event) {
    // First check if click is within frame bounds
    graphics::RenderContext baseContext;
    if (event.context) {
        baseContext = *event.context;
    } else {
        int displayW = al_get_display_width(al_get_current_display());
        int displayH = al_get_display_height(al_get_current_display());
        baseContext = graphics::RenderContext{displayW, displayH, 0.0f, 0.0f, nullptr};
    }
    
    if (!hitTest(event.x, event.y, baseContext)) {
        return graphics::IEventHandler::onMouseDown(event);
    }
    
    // Get viewport bounds for scroll offset
    auto posPx = getPosition().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    auto sizePx = getSize().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    const float absX = posPx.first + baseContext.offsetX;
    const float absY = posPx.second + baseContext.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;
    const float viewportHeight = height - 2 * padding;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    const float maxScroll = std::max(0.0f, effectiveContentHeight - viewportHeight);
    float clampedScroll = std::min(std::max(0.0f, scrollOffset), maxScroll);
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(width - 2 * padding);
    childContext.screenHeight = static_cast<int>(viewportHeight);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding - clampedScroll;
    
    // Check children in reverse order (top-most first)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler) continue;
        
        if (handler->hitTest(event.x, event.y, childContext)) {
            graphics::MouseEvent localEvent = event;
            localEvent.context = &childContext;
            localEvent.localX = event.x - childContext.offsetX;
            localEvent.localY = event.y - childContext.offsetY;
            if (handler->onMouseDown(localEvent)) {
                return true;
            }
        }
    }
    return graphics::IEventHandler::onMouseDown(event);
}

bool ScrollableFrameDrawable::onMouseUp(const graphics::MouseEvent& event) {
    // Get viewport bounds for scroll calculation
    graphics::RenderContext baseContext;
    if (event.context) {
        baseContext = *event.context;
    } else {
        int displayW = al_get_display_width(al_get_current_display());
        int displayH = al_get_display_height(al_get_current_display());
        baseContext = graphics::RenderContext{displayW, displayH, 0.0f, 0.0f, nullptr};
    }
    
    // Get viewport bounds
    auto posPx = getPosition().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    auto sizePx = getSize().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    const float absX = posPx.first + baseContext.offsetX;
    const float absY = posPx.second + baseContext.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;
    const float viewportHeight = height - 2 * padding;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    const float maxScroll = std::max(0.0f, effectiveContentHeight - viewportHeight);
    float clampedScroll = std::min(std::max(0.0f, scrollOffset), maxScroll);
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(width - 2 * padding);
    childContext.screenHeight = static_cast<int>(viewportHeight);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding - clampedScroll;
    
    // Check children in reverse order (top-most first)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler) continue;
        
        if (handler->hitTest(event.x, event.y, childContext)) {
            graphics::MouseEvent localEvent = event;
            localEvent.context = &childContext;
            localEvent.localX = event.x - childContext.offsetX;
            localEvent.localY = event.y - childContext.offsetY;
            if (handler->onMouseUp(localEvent)) {
                return true;
            }
        }
    }
    return graphics::IEventHandler::onMouseUp(event);
}

bool ScrollableFrameDrawable::onMouseMove(const graphics::MouseEvent& event) {
    // Get viewport bounds for scroll calculation
    graphics::RenderContext baseContext;
    if (event.context) {
        baseContext = *event.context;
    } else {
        int displayW = al_get_display_width(al_get_current_display());
        int displayH = al_get_display_height(al_get_current_display());
        baseContext = graphics::RenderContext{displayW, displayH, 0.0f, 0.0f, nullptr};
    }
    
    // Get viewport bounds
    auto posPx = getPosition().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    auto sizePx = getSize().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    const float absX = posPx.first + baseContext.offsetX;
    const float absY = posPx.second + baseContext.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;
    const float viewportHeight = height - 2 * padding;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    const float maxScroll = std::max(0.0f, effectiveContentHeight - viewportHeight);
    float clampedScroll = std::min(std::max(0.0f, scrollOffset), maxScroll);
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(width - 2 * padding);
    childContext.screenHeight = static_cast<int>(viewportHeight);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding - clampedScroll;
    
    // Check children in reverse order (top-most first)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler) continue;
        
        if (handler->hitTest(event.x, event.y, childContext)) {
            graphics::MouseEvent localEvent = event;
            localEvent.context = &childContext;
            localEvent.localX = event.x - childContext.offsetX;
            localEvent.localY = event.y - childContext.offsetY;
            if (handler->onMouseMove(localEvent)) {
                return true;
            }
        }
    }
    return graphics::IEventHandler::onMouseMove(event);
}

bool ScrollableFrameDrawable::onMouseEnter(const graphics::MouseEvent& event) {
    // Get viewport bounds for scroll calculation
    graphics::RenderContext baseContext;
    if (event.context) {
        baseContext = *event.context;
    } else {
        int displayW = al_get_display_width(al_get_current_display());
        int displayH = al_get_display_height(al_get_current_display());
        baseContext = graphics::RenderContext{displayW, displayH, 0.0f, 0.0f, nullptr};
    }
    
    // Get viewport bounds
    auto posPx = getPosition().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    auto sizePx = getSize().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    const float absX = posPx.first + baseContext.offsetX;
    const float absY = posPx.second + baseContext.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;
    const float viewportHeight = height - 2 * padding;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    const float maxScroll = std::max(0.0f, effectiveContentHeight - viewportHeight);
    float clampedScroll = std::min(std::max(0.0f, scrollOffset), maxScroll);
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(width - 2 * padding);
    childContext.screenHeight = static_cast<int>(viewportHeight);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding - clampedScroll;
    
    // Check children in reverse order (top-most first)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler) continue;
        
            if (handler->hitTest(event.x, event.y, childContext)) {
                graphics::MouseEvent localEvent = event;
                localEvent.context = &childContext;
                localEvent.localX = event.x - childContext.offsetX;
                localEvent.localY = event.y - childContext.offsetY;
                if (handler->onMouseEnter(localEvent)) {
                    return true;
                }
            }
    }
    return graphics::IEventHandler::onMouseEnter(event);
}

bool ScrollableFrameDrawable::onMouseLeave(const graphics::MouseEvent& event) {
    // Get viewport bounds for scroll calculation
    graphics::RenderContext baseContext;
    if (event.context) {
        baseContext = *event.context;
    } else {
        int displayW = al_get_display_width(al_get_current_display());
        int displayH = al_get_display_height(al_get_current_display());
        baseContext = graphics::RenderContext{displayW, displayH, 0.0f, 0.0f, nullptr};
    }
    
    // Get viewport bounds
    auto posPx = getPosition().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    auto sizePx = getSize().toScreenPos(static_cast<float>(baseContext.screenWidth), static_cast<float>(baseContext.screenHeight));
    const float absX = posPx.first + baseContext.offsetX;
    const float absY = posPx.second + baseContext.offsetY;
    const float width = sizePx.first;
    const float height = sizePx.second;
    const float viewportHeight = height - 2 * padding;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    const float maxScroll = std::max(0.0f, effectiveContentHeight - viewportHeight);
    float clampedScroll = std::min(std::max(0.0f, scrollOffset), maxScroll);
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(width - 2 * padding);
    childContext.screenHeight = static_cast<int>(viewportHeight);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding - clampedScroll;
    
    // Check children in reverse order (top-most first)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler) continue;
        
        if (handler->hitTest(event.x, event.y, childContext)) {
            graphics::MouseEvent localEvent = event;
            localEvent.context = &childContext;
            localEvent.localX = event.x - childContext.offsetX;
            localEvent.localY = event.y - childContext.offsetY;
            if (handler->onMouseLeave(localEvent)) {
                return true;
            }
        }
    }
    return graphics::IEventHandler::onMouseLeave(event);
}

bool ScrollableFrameDrawable::onMouseScroll(const graphics::ScrollEvent& event) {
    // Use the cached viewport height from the last draw() call
    const float viewportHeight = cachedViewportHeight > 0.0f ? cachedViewportHeight : 100.0f;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    const float maxScroll = std::max(0.0f, effectiveContentHeight - viewportHeight);

    scrollOffset -= event.dy * scrollStep;
    if (scrollOffset < 0.0f) scrollOffset = 0.0f;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    return true; // consume scroll
}

bool ScrollableFrameDrawable::onKeyDown(const graphics::KeyboardEvent& event) {
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler || !handler->isFocusable()) continue;
        
        if (handler->onKeyDown(event)) {
            return true;
        }
    }
    return graphics::IEventHandler::onKeyDown(event);
}

bool ScrollableFrameDrawable::onKeyUp(const graphics::KeyboardEvent& event) {
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler || !handler->isFocusable()) continue;
        
        if (handler->onKeyUp(event)) {
            return true;
        }
    }
    return graphics::IEventHandler::onKeyUp(event);
}

bool ScrollableFrameDrawable::onKeyChar(const graphics::KeyboardEvent& event) {
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler || !handler->isFocusable()) continue;
        
        if (handler->onKeyChar(event)) {
            return true;
        }
    }
    return graphics::IEventHandler::onKeyChar(event);
}

bool ScrollableFrameDrawable::hitTest(float x, float y, const graphics::RenderContext& context) const {
    auto posPx = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto sizePx = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    const float absX = posPx.first + context.offsetX;
    const float absY = posPx.second + context.offsetY;
    return x >= absX && x <= absX + sizePx.first && y >= absY && y <= absY + sizePx.second;
}

float ScrollableFrameDrawable::computeMaxScroll(float screenW, float screenH) const noexcept {
    auto sizePx = getSize().toScreenPos(screenW, screenH);
    const float viewportHeight = sizePx.second - 2 * padding;
    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : viewportHeight;
    return std::max(0.0f, effectiveContentHeight - viewportHeight);
}

} // namespace ui
