#include "graphics/drawables/scrollable_frame.hpp"

#include <algorithm>
#include <allegro5/allegro.h>

namespace ui {

void ScrollableFrameDrawable::draw(const graphics::RenderContext& context) const {
    const auto geometry = computeScrollbarGeometry(context);
    const float absX = geometry.absX;
    const float absY = geometry.absY;
    const float width = geometry.width;
    const float height = geometry.height;
    const float clampedScroll = geometry.clampedScroll;

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
    const float viewportWidth = geometry.viewportWidth;
    const float viewportHeight = geometry.viewportHeight;
    
    // Cache viewport height for use in event handlers
    const_cast<ScrollableFrameDrawable*>(this)->cachedViewportHeight = viewportHeight;

    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = context;
    childContext.screenWidth = static_cast<int>(viewportWidth);
    childContext.screenHeight = static_cast<int>(viewportHeight);
    childContext.offsetX = absX + padding;
    childContext.offsetY = absY + padding - clampedScroll;

    drawChildren(childContext);

    // Draw scrollbar indicator if content exceeds viewport
    if (showScrollbar && geometry.hasScrollableRange && scrollbarWidth > 0.0f) {
        al_draw_filled_rectangle(
            geometry.thumbX,
            geometry.thumbY,
            geometry.thumbX + geometry.thumbWidth,
            geometry.thumbY + geometry.thumbHeight,
            scrollbarColor
        );
    }

    al_set_clipping_rectangle(oldClipX, oldClipY, oldClipW, oldClipH);
}

bool ScrollableFrameDrawable::onMouseDown(const graphics::MouseEvent& event) {
    const auto baseContext = getBaseContext(event.context);
    const auto geometry = computeScrollbarGeometry(baseContext);
    activeMouseChild = nullptr;
    
    if (!hitTest(event.x, event.y, baseContext)) {
        return graphics::IEventHandler::onMouseDown(event);
    }

    const bool leftButton = event.button == 1;
    const bool onScrollbarTrack =
        event.x >= geometry.thumbX &&
        event.x <= geometry.thumbX + geometry.thumbWidth &&
        event.y >= geometry.viewportY &&
        event.y <= geometry.viewportY + geometry.viewportHeight;

    const bool onScrollbarThumb =
        event.x >= geometry.thumbX &&
        event.x <= geometry.thumbX + geometry.thumbWidth &&
        event.y >= geometry.thumbY &&
        event.y <= geometry.thumbY + geometry.thumbHeight;

    if (leftButton && showScrollbar && geometry.hasScrollableRange && scrollbarWidth > 0.0f) {
        if (onScrollbarThumb) {
            isDraggingScrollbar = true;
            scrollbarDragOffsetY = event.y - geometry.thumbY;
            return true;
        }

        if (onScrollbarTrack) {
            const float targetThumbTop = clampThumbTop(event.y - geometry.thumbHeight * 0.5f, geometry);
            scrollOffset = scrollOffsetFromThumbTop(targetThumbTop, geometry);
            isDraggingScrollbar = true;
            scrollbarDragOffsetY = event.y - targetThumbTop;
            return true;
        }
    }
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(geometry.viewportWidth);
    childContext.screenHeight = static_cast<int>(geometry.viewportHeight);
    childContext.offsetX = geometry.viewportX;
    childContext.offsetY = geometry.viewportY - geometry.clampedScroll;
    
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
                activeMouseChild = handler;
                return true;
            }
        }
    }
    return graphics::IEventHandler::onMouseDown(event);
}

bool ScrollableFrameDrawable::onMouseUp(const graphics::MouseEvent& event) {
    const auto baseContext = getBaseContext(event.context);
    const auto geometry = computeScrollbarGeometry(baseContext);

    if (isDraggingScrollbar && event.button == 1) {
        isDraggingScrollbar = false;
        return true;
    }
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(geometry.viewportWidth);
    childContext.screenHeight = static_cast<int>(geometry.viewportHeight);
    childContext.offsetX = geometry.viewportX;
    childContext.offsetY = geometry.viewportY - geometry.clampedScroll;

    if (activeMouseChild) {
        graphics::MouseEvent localEvent = event;
        localEvent.context = &childContext;
        localEvent.localX = event.x - childContext.offsetX;
        localEvent.localY = event.y - childContext.offsetY;
        const bool handled = activeMouseChild->onMouseUp(localEvent);
        activeMouseChild = nullptr;
        if (handled) {
            return true;
        }
    }
    
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
                activeMouseChild = nullptr;
                return true;
            }
        }
    }
    activeMouseChild = nullptr;
    return graphics::IEventHandler::onMouseUp(event);
}

bool ScrollableFrameDrawable::onMouseMove(const graphics::MouseEvent& event) {
    const auto baseContext = getBaseContext(event.context);
    const auto geometry = computeScrollbarGeometry(baseContext);

    if (isDraggingScrollbar) {
        const float targetThumbTop = clampThumbTop(event.y - scrollbarDragOffsetY, geometry);
        scrollOffset = scrollOffsetFromThumbTop(targetThumbTop, geometry);
        return true;
    }
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(geometry.viewportWidth);
    childContext.screenHeight = static_cast<int>(geometry.viewportHeight);
    childContext.offsetX = geometry.viewportX;
    childContext.offsetY = geometry.viewportY - geometry.clampedScroll;

    if (activeMouseChild) {
        graphics::MouseEvent localEvent = event;
        localEvent.context = &childContext;
        localEvent.localX = event.x - childContext.offsetX;
        localEvent.localY = event.y - childContext.offsetY;
        if (activeMouseChild->onMouseMove(localEvent)) {
            return true;
        }
    }
    
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
    const auto baseContext = getBaseContext(event.context);
    const auto geometry = computeScrollbarGeometry(baseContext);
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(geometry.viewportWidth);
    childContext.screenHeight = static_cast<int>(geometry.viewportHeight);
    childContext.offsetX = geometry.viewportX;
    childContext.offsetY = geometry.viewportY - geometry.clampedScroll;
    
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
    const auto baseContext = getBaseContext(event.context);
    const auto geometry = computeScrollbarGeometry(baseContext);
    
    // Set up child context with padding and scroll offset
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(geometry.viewportWidth);
    childContext.screenHeight = static_cast<int>(geometry.viewportHeight);
    childContext.offsetX = geometry.viewportX;
    childContext.offsetY = geometry.viewportY - geometry.clampedScroll;
    
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

graphics::RenderContext ScrollableFrameDrawable::getBaseContext(const graphics::RenderContext* eventContext) const {
    if (eventContext) {
        return *eventContext;
    }

    int displayW = al_get_display_width(al_get_current_display());
    int displayH = al_get_display_height(al_get_current_display());
    return graphics::RenderContext{displayW, displayH, 0.0f, 0.0f, nullptr};
}

ScrollableFrameDrawable::ScrollbarGeometry ScrollableFrameDrawable::computeScrollbarGeometry(const graphics::RenderContext& context) const noexcept {
    ScrollbarGeometry geometry;

    const auto sizePx = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    const auto posPx = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));

    geometry.absX = posPx.first + context.offsetX;
    geometry.absY = posPx.second + context.offsetY;
    geometry.width = sizePx.first;
    geometry.height = sizePx.second;
    geometry.viewportX = geometry.absX + padding;
    geometry.viewportY = geometry.absY + padding;
    geometry.viewportWidth = geometry.width - 2 * padding;
    geometry.viewportHeight = geometry.height - 2 * padding;

    const float effectiveContentHeight = contentHeight > 0.0f ? contentHeight : geometry.viewportHeight;
    geometry.maxScroll = std::max(0.0f, effectiveContentHeight - geometry.viewportHeight);
    geometry.clampedScroll = std::clamp(scrollOffset, 0.0f, geometry.maxScroll);
    geometry.hasScrollableRange = geometry.maxScroll > 0.0f && geometry.viewportHeight > 0.0f;

    geometry.thumbWidth = std::max(0.0f, scrollbarWidth);
    geometry.thumbX = geometry.absX + geometry.width - geometry.thumbWidth;

    if (!geometry.hasScrollableRange || geometry.thumbWidth <= 0.0f) {
        geometry.thumbHeight = 0.0f;
        geometry.thumbY = geometry.viewportY;
        return geometry;
    }

    const float ratio = geometry.viewportHeight / effectiveContentHeight;
    geometry.thumbHeight = std::max(10.0f, geometry.viewportHeight * ratio);
    const float thumbTravel = geometry.viewportHeight - geometry.thumbHeight;
    const float scrollRatio = geometry.maxScroll > 0.0f ? geometry.clampedScroll / geometry.maxScroll : 0.0f;
    geometry.thumbY = geometry.viewportY + scrollRatio * std::max(0.0f, thumbTravel);

    return geometry;
}

float ScrollableFrameDrawable::scrollOffsetFromThumbTop(float thumbTop, const ScrollbarGeometry& geometry) const noexcept {
    const float thumbTravel = std::max(0.0f, geometry.viewportHeight - geometry.thumbHeight);
    if (thumbTravel <= 0.0f || geometry.maxScroll <= 0.0f) {
        return 0.0f;
    }

    const float ratio = (thumbTop - geometry.viewportY) / thumbTravel;
    return std::clamp(ratio, 0.0f, 1.0f) * geometry.maxScroll;
}

float ScrollableFrameDrawable::clampThumbTop(float thumbTop, const ScrollbarGeometry& geometry) const noexcept {
    const float minTop = geometry.viewportY;
    const float maxTop = geometry.viewportY + std::max(0.0f, geometry.viewportHeight - geometry.thumbHeight);
    return std::clamp(thumbTop, minTop, maxTop);
}

} // namespace ui
