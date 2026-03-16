#pragma once

#include "graphics/drawables/frame.hpp"
#include "graphics/event_handler.hpp"

namespace ui {

// Scrollable frame that combines frame styling (padding/margin) with vertical
// scrolling capabilities. Content is clipped to the frame viewport while
// maintaining padding offsets. A scrollbar indicator is drawn when content
// exceeds the available viewport height.
class ScrollableFrameDrawable : public FrameDrawable {
public:
    ScrollableFrameDrawable() = default;
    ScrollableFrameDrawable(graphics::UV position, graphics::UV size)
        : FrameDrawable(position, size) {}

    void setContentHeight(float h) noexcept { contentHeight = h; }
    float getContentHeight() const noexcept { return contentHeight; }

    void setScrollOffset(float y) noexcept { scrollOffset = y; }
    float getScrollOffset() const noexcept { return scrollOffset; }
    void scrollBy(float delta) noexcept { scrollOffset += delta; }

    void setScrollbarColor(ALLEGRO_COLOR c) noexcept { scrollbarColor = c; }
    void setScrollbarWidth(float w) noexcept { scrollbarWidth = w; }
    void enableScrollbar(bool enable) noexcept { showScrollbar = enable; }

    void draw(const graphics::RenderContext& context = {}) const override;

    // graphics::IEventHandler
    bool onMouseDown(const graphics::MouseEvent& event) override;
    bool onMouseUp(const graphics::MouseEvent& event) override;
    bool onMouseMove(const graphics::MouseEvent& event) override;
    bool onMouseEnter(const graphics::MouseEvent& event) override;
    bool onMouseLeave(const graphics::MouseEvent& event) override;
    bool onMouseScroll(const graphics::ScrollEvent& event) override;
    bool onKeyDown(const graphics::KeyboardEvent& event) override;
    bool onKeyUp(const graphics::KeyboardEvent& event) override;
    bool onKeyChar(const graphics::KeyboardEvent& event) override;
    bool hitTest(float x, float y, const graphics::RenderContext& context) const override;
    bool isEnabled() const override { return true; }

    void setScrollStep(float step) noexcept { scrollStep = step; }

private:
    struct ScrollbarGeometry {
        float absX = 0.0f;
        float absY = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float viewportX = 0.0f;
        float viewportY = 0.0f;
        float viewportWidth = 0.0f;
        float viewportHeight = 0.0f;
        float maxScroll = 0.0f;
        float clampedScroll = 0.0f;
        float thumbX = 0.0f;
        float thumbY = 0.0f;
        float thumbWidth = 0.0f;
        float thumbHeight = 0.0f;
        bool hasScrollableRange = false;
    };

    float scrollOffset = 0.0f;
    float contentHeight = 0.0f;
    float cachedViewportHeight = 0.0f;  // Cache viewport height from draw() for use in event handlers
    ALLEGRO_COLOR scrollbarColor = al_map_rgb(160, 160, 160);
    float scrollbarWidth = 6.0f;
    bool showScrollbar = true;
    float scrollStep = 24.0f; // pixels per wheel notch
    bool isDraggingScrollbar = false;
    float scrollbarDragOffsetY = 0.0f;
    graphics::IEventHandler* activeMouseChild = nullptr;

    float computeMaxScroll(float screenW, float screenH) const noexcept;
    graphics::RenderContext getBaseContext(const graphics::RenderContext* eventContext) const;
    ScrollbarGeometry computeScrollbarGeometry(const graphics::RenderContext& context) const noexcept;
    float scrollOffsetFromThumbTop(float thumbTop, const ScrollbarGeometry& geometry) const noexcept;
    float clampThumbTop(float thumbTop, const ScrollbarGeometry& geometry) const noexcept;
};

} // namespace ui