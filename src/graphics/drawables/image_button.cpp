#include "graphics/drawables/image_button.hpp"
#include "graphics/draw_shapes.hpp"
#include "graphics/state_color.hpp"
#include <allegro5/allegro_primitives.h>
#include <algorithm>

namespace ui {

ImageButtonDrawable::ImageButtonDrawable(graphics::UV position, graphics::UV size, ALLEGRO_BITMAP* bitmap, bool owned)
    : m_image(bitmap, owned) {
    setPosition(position);
    setSize(size);
}

void ImageButtonDrawable::draw(const graphics::RenderContext& context) const {
    // Culling: skip off-screen buttons
    if (isOffScreen(context)) {
        return;
    }
    
    const auto bounds = getScreenBounds(context);
    const float x = bounds.x;
    const float y = bounds.y;
    const float w = bounds.w;
    const float h = bounds.h;

    const ALLEGRO_COLOR bgColor = graphics::selectStateColor(
        m_enabled,
        m_isPressed,
        m_isHovered,
        m_colorNormal,
        m_colorHover,
        m_colorPressed,
        m_colorDisabled
    );

    const ALLEGRO_COLOR borderColor = graphics::selectStateColor(
        m_enabled,
        m_isPressed,
        m_isHovered,
        m_borderNormal,
        m_borderHover,
        m_borderPressed,
        m_borderDisabled
    );

    if (m_drawBackground) {
        graphics::drawFilledRectWithBorder(
            x,
            y,
            w,
            h,
            bgColor,
            borderColor,
            m_drawBorder ? m_borderThickness : 0.0f
        );
    } else if (m_drawBorder && m_borderThickness > 0.0f) {
        al_draw_rectangle(x, y, x + w, y + h, borderColor, m_borderThickness);
    }

    float innerX = x + m_paddingLeft;
    float innerY = y + m_paddingTop;
    float innerW = std::max(0.0f, w - (m_paddingLeft + m_paddingRight));
    float innerH = std::max(0.0f, h - (m_paddingTop + m_paddingBottom));

    if (innerW <= 0.0f || innerH <= 0.0f) {
        return;
    }

    graphics::RenderContext imageContext = context;
    imageContext.offsetX = innerX;
    imageContext.offsetY = innerY;

    m_image.setPosition(graphics::UV(0.0f, 0.0f, 0.0f, 0.0f));
    m_image.setSize(graphics::UV(0.0f, 0.0f, innerW, innerH));
    m_image.draw(imageContext);
}

bool ImageButtonDrawable::hitTest(float x, float y, const graphics::RenderContext& context) const {
    return hitTestRect(x, y, context);
}

bool ImageButtonDrawable::onMouseDown(const graphics::MouseEvent& event) {
    if (!m_enabled) return false;

    m_isPressed = true;
    return true;
}

bool ImageButtonDrawable::onMouseUp(const graphics::MouseEvent& event) {
    if (!m_enabled) return false;

    bool wasPressed = m_isPressed;
    m_isPressed = false;

    if (wasPressed && event.context && hitTest(event.x, event.y, *event.context)) {
        if (m_onClick) {
            m_onClick();
        }
    }

    return wasPressed;
}

bool ImageButtonDrawable::onMouseEnter(const graphics::MouseEvent& event) {
    m_isHovered = true;
    return false;
}

bool ImageButtonDrawable::onMouseLeave(const graphics::MouseEvent& event) {
    m_isHovered = false;
    m_isPressed = false;
    return false;
}

} // namespace ui
