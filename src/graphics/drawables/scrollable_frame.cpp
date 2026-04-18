#include "graphics/drawables/scrollable_frame.hpp"
#include "graphics/clipping.hpp"
#include "graphics/draw_shapes.hpp"

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

    const auto oldClip = graphics::getCurrentClipRect();
    const graphics::ClipRect frameClip{
        static_cast<int>(absX),
        static_cast<int>(absY),
        static_cast<int>(width),
        static_cast<int>(height),
    };
    graphics::setIntersectedClipRect(oldClip, frameClip);

    // Draw background and border (inherited from ContainerDrawable)
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

    // Calculate viewport dimensions accounting for padding
    const float viewportWidth = geometry.viewportWidth;
    const float viewportHeight = geometry.viewportHeight;

    // Clip children to the scrollable viewport (not the full frame) to avoid bleed while scrolling.
    const graphics::ClipRect viewportClip{
        static_cast<int>(geometry.viewportX),
        static_cast<int>(geometry.viewportY),
        static_cast<int>(viewportWidth),
        static_cast<int>(viewportHeight),
    };
    graphics::setIntersectedClipRect(oldClip, viewportClip);
    
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
    graphics::setIntersectedClipRect(oldClip, frameClip);
    if (showScrollbar && geometry.hasScrollableRange && scrollbarWidth > 0.0f) {
        al_draw_filled_rectangle(
            geometry.thumbX,
            geometry.thumbY,
            geometry.thumbX + geometry.thumbWidth,
            geometry.thumbY + geometry.thumbHeight,
            scrollbarColor
        );
    }

    graphics::setClipRect(oldClip);
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
    
    const auto childContext = buildChildContext(baseContext, geometry);
    if (dispatchToChildren(event, childContext, MouseDispatchType::Down, true)) {
        return true;
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
    
    const auto childContext = buildChildContext(baseContext, geometry);

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

    if (dispatchToChildren(event, childContext, MouseDispatchType::Up)) {
        activeMouseChild = nullptr;
        return true;
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
    
    const auto childContext = buildChildContext(baseContext, geometry);

    if (activeMouseChild) {
        graphics::MouseEvent localEvent = event;
        localEvent.context = &childContext;
        localEvent.localX = event.x - childContext.offsetX;
        localEvent.localY = event.y - childContext.offsetY;
        if (activeMouseChild->onMouseMove(localEvent)) {
            return true;
        }
    }

    if (dispatchToChildren(event, childContext, MouseDispatchType::Move)) {
        return true;
    }

    return graphics::IEventHandler::onMouseMove(event);
}

bool ScrollableFrameDrawable::onMouseEnter(const graphics::MouseEvent& event) {
    const auto baseContext = getBaseContext(event.context);
    const auto geometry = computeScrollbarGeometry(baseContext);
    
    const auto childContext = buildChildContext(baseContext, geometry);
    if (dispatchToChildren(event, childContext, MouseDispatchType::Enter)) {
        return true;
    }

    return graphics::IEventHandler::onMouseEnter(event);
}

bool ScrollableFrameDrawable::onMouseLeave(const graphics::MouseEvent& event) {
    const auto baseContext = getBaseContext(event.context);
    const auto geometry = computeScrollbarGeometry(baseContext);
    
    const auto childContext = buildChildContext(baseContext, geometry);
    if (dispatchToChildren(event, childContext, MouseDispatchType::Leave)) {
        return true;
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
    return graphics::resolveEventContext(eventContext);
}

graphics::RenderContext ScrollableFrameDrawable::buildChildContext(
    const graphics::RenderContext& baseContext,
    const ScrollbarGeometry& geometry
) const {
    graphics::RenderContext childContext = baseContext;
    childContext.screenWidth = static_cast<int>(geometry.viewportWidth);
    childContext.screenHeight = static_cast<int>(geometry.viewportHeight);
    childContext.offsetX = geometry.viewportX;
    childContext.offsetY = geometry.viewportY - geometry.clampedScroll;
    return childContext;
}

bool ScrollableFrameDrawable::dispatchToChildren(
    const graphics::MouseEvent& event,
    const graphics::RenderContext& childContext,
    MouseDispatchType dispatchType,
    bool captureHandledChild
) {
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
        if (!handler) {
            continue;
        }

        if (!handler->hitTest(event.x, event.y, childContext)) {
            continue;
        }

        graphics::MouseEvent localEvent = event;
        localEvent.context = &childContext;
        localEvent.localX = event.x - childContext.offsetX;
        localEvent.localY = event.y - childContext.offsetY;

        bool handled = false;
        switch (dispatchType) {
            case MouseDispatchType::Down:
                handled = handler->onMouseDown(localEvent);
                break;
            case MouseDispatchType::Up:
                handled = handler->onMouseUp(localEvent);
                break;
            case MouseDispatchType::Move:
                handled = handler->onMouseMove(localEvent);
                break;
            case MouseDispatchType::Enter:
                handled = handler->onMouseEnter(localEvent);
                break;
            case MouseDispatchType::Leave:
                handled = handler->onMouseLeave(localEvent);
                break;
        }

        if (!handled) {
            continue;
        }

        if (captureHandledChild) {
            activeMouseChild = handler;
        }
        return true;
    }

    return false;
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

    // Reserve horizontal space for scrollbar when visible so children don't render under it.
    if (showScrollbar && geometry.hasScrollableRange && scrollbarWidth > 0.0f) {
        geometry.viewportWidth = std::max(0.0f, geometry.viewportWidth - scrollbarWidth);
    }

    geometry.thumbWidth = std::max(0.0f, scrollbarWidth);
    geometry.thumbX = geometry.viewportX + geometry.viewportWidth;

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
