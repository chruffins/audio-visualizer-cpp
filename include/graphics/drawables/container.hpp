#pragma once

#include <vector>
#include <memory>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "graphics/drawable.hpp"
#include "graphics/uv.hpp"

namespace ui {

// Basic container that clips its children to a rectangular area and applies a
// local origin so nested drawables can position relative to the container.
class ContainerDrawable : public graphics::Drawable {
public:
    ContainerDrawable() = default;
    ContainerDrawable(graphics::UV position, graphics::UV size)
        : position(position), size(size) {}

    void setPosition(const graphics::UV& p) noexcept { position = p; }
    graphics::UV getPosition() const noexcept { return position; }

    void setSize(const graphics::UV& s) noexcept { size = s; }
    graphics::UV getSize() const noexcept { return size; }

    void setBackgroundColor(ALLEGRO_COLOR c) noexcept {
        backgroundColor = c;
        drawBackground = true;
    }
    void disableBackground() noexcept { drawBackground = false; }

    void setBorderColor(ALLEGRO_COLOR c) noexcept { borderColor = c; }
    void setBorderThickness(int t) noexcept { borderThickness = t; }

    void addChild(const graphics::DrawablePtr& child) { children.push_back(child); }
    void clearChildren() { children.clear(); }
    const graphics::DrawableList& getChildren() const noexcept { return children; }

    void draw(const graphics::RenderContext& context = {}) const override;

protected:
    virtual void drawChildren(const graphics::RenderContext& childContext) const;

    graphics::UV position{0.0f, 0.0f, 0.0f, 0.0f};
    graphics::UV size{1.0f, 1.0f, 0.0f, 0.0f};
    graphics::DrawableList children;

    bool drawBackground = false;
    ALLEGRO_COLOR backgroundColor = al_map_rgba(0, 0, 0, 0);
    ALLEGRO_COLOR borderColor = al_map_rgb(255, 255, 255);
    int borderThickness = 1;
};

// Scrollable container that offsets child rendering by a vertical scroll value
// while keeping children clipped to the viewport. A simple scrollbar indicator
// is drawn on the right edge when content exceeds the viewport height.
class ScrollContainerDrawable : public ContainerDrawable {
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

    void draw(const graphics::RenderContext& context = {}) const override;

private:
    float scrollOffset = 0.0f;
    float contentHeight = 0.0f;
    ALLEGRO_COLOR scrollbarColor = al_map_rgb(160, 160, 160);
    float scrollbarWidth = 6.0f;
    bool showScrollbar = true;
};

} // namespace ui
