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
    bool onMouseScroll(float dx, float dy) override;
    bool hitTest(float x, float y) const override;
    bool isEnabled() const override { return true; }

    void setScrollStep(float step) noexcept { scrollStep = step; }

private:
    float scrollOffset = 0.0f;
    float contentHeight = 0.0f;
    ALLEGRO_COLOR scrollbarColor = al_map_rgb(160, 160, 160);
    float scrollbarWidth = 6.0f;
    bool showScrollbar = true;
    float scrollStep = 24.0f; // pixels per wheel notch

    float computeMaxScroll(float screenW, float screenH) const noexcept;
};

} // namespace ui