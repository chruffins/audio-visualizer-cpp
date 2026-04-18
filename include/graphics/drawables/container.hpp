#pragma once

#include <vector>
#include <memory>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "graphics/drawable.hpp"
#include "graphics/event_handler.hpp"
#include "graphics/uv.hpp"

namespace ui {

// Basic container that clips its children to a rectangular area and applies a
// local origin so nested drawables can position relative to the container.
class ContainerDrawable : public graphics::Drawable {
public:
    ContainerDrawable() = default;
    ContainerDrawable(graphics::UV position, graphics::UV size) {
        setPosition(position);
        setSize(size);
    }

    // setPosition and setSize inherited from Drawable

    void setBackgroundColor(ALLEGRO_COLOR c) noexcept {
        backgroundColor = c;
        drawBackground = true;
    }
    void disableBackground() noexcept { drawBackground = false; }

    void setBorderColor(ALLEGRO_COLOR c) noexcept { borderColor = c; }
    void setBorderThickness(int t) noexcept { borderThickness = t; }

    void addChild(graphics::Drawable* child) { children.push_back(child); }
    void clearChildren() { children.clear(); }
    const graphics::DrawableObserverList& getChildren() const noexcept { return children; }

    void draw(const graphics::RenderContext& context = {}) const override;

protected:
    virtual void drawChildren(const graphics::RenderContext& childContext) const;

    graphics::DrawableObserverList children;

    // Static cache for batch clipping optimization
    static int s_lastClipX;
    static int s_lastClipY;
    static int s_lastClipW;
    static int s_lastClipH;
    
    bool drawBackground = false;
    ALLEGRO_COLOR backgroundColor = al_map_rgba(0, 0, 0, 0);
    ALLEGRO_COLOR borderColor = al_map_rgb(255, 255, 255);
    int borderThickness = 1;
};

// Scrollable container that offsets child rendering by a vertical scroll value
// while keeping children clipped to the viewport. A simple scrollbar indicator
// is drawn on the right edge when content exceeds the viewport height.
class ScrollContainerDrawable : public ContainerDrawable, public graphics::IEventHandler {
public:
    using ContainerDrawable::ContainerDrawable;

    void setContentHeight(float h) noexcept { contentHeight = h; }
    float getContentHeight() const noexcept { return contentHeight; }

    void setScrollOffset(float y) noexcept { scrollOffset = y; }
    float getScrollOffset() const noexcept { return scrollOffset; }
    void scrollBy(float delta) noexcept { scrollOffset += delta; }

    void setScrollbarColor(ALLEGRO_COLOR c) noexcept { scrollbarColor = c; }
    void setScrollbarWidth(float w) noexcept { scrollbarWidth = w; }
    void enableScrollbar(bool enable) noexcept { showScrollbar = enable; }
    void setScrollStep(float step) noexcept { scrollStep = step; }

    void draw(const graphics::RenderContext& context = {}) const override;

    bool onMouseDown(const graphics::MouseEvent& event) override;
    bool onMouseUp(const graphics::MouseEvent& event) override;
    bool onMouseMove(const graphics::MouseEvent& event) override;
    bool onMouseScroll(const graphics::ScrollEvent& event) override;
    bool hitTest(float x, float y, const graphics::RenderContext& context) const override;
    bool isEnabled() const override { return true; }

private:
    enum class MouseDispatchType {
        Down,
        Up,
        Move,
    };

    struct ScrollbarGeometry {
        float absX = 0.0f;
        float absY = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float viewportX = 0.0f;
        float viewportY = 0.0f;
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
    ALLEGRO_COLOR scrollbarColor = al_map_rgb(160, 160, 160);
    float scrollbarWidth = 6.0f;
    bool showScrollbar = true;
    float scrollStep = 24.0f;
    bool isDraggingScrollbar = false;
    float scrollbarDragOffsetY = 0.0f;
    graphics::IEventHandler* activeMouseChild = nullptr;

    graphics::RenderContext getBaseContext(const graphics::RenderContext* eventContext) const;
    graphics::RenderContext buildChildContext(const graphics::RenderContext& baseContext,
                                              const ScrollbarGeometry& geometry) const;
    bool dispatchToChildren(const graphics::MouseEvent& event,
                            const graphics::RenderContext& childContext,
                            MouseDispatchType dispatchType,
                            bool captureHandledChild = false);
    ScrollbarGeometry computeScrollbarGeometry(const graphics::RenderContext& context) const noexcept;
    float clampThumbTop(float thumbTop, const ScrollbarGeometry& geometry) const noexcept;
    float scrollOffsetFromThumbTop(float thumbTop, const ScrollbarGeometry& geometry) const noexcept;
};

} // namespace ui