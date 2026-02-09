#include "graphics/drawables/image_button.hpp"
#include <allegro5/allegro_primitives.h>
#include <algorithm>

namespace ui {

ImageButtonDrawable::ImageButtonDrawable(graphics::UV position, graphics::UV size, ALLEGRO_BITMAP* bitmap, bool owned)
    : m_image(bitmap, owned) {
    setPosition(position);
    setSize(size);
}

void ImageButtonDrawable::draw(const graphics::RenderContext& context) const {
    auto [x, y] = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto [w, h] = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    x += context.offsetX;
    y += context.offsetY;

    ALLEGRO_COLOR bgColor = m_colorNormal;
    if (!m_enabled) {
        bgColor = m_colorDisabled;
    } else if (m_isPressed) {
        bgColor = m_colorPressed;
    } else if (m_isHovered) {
        bgColor = m_colorHover;
    }

    ALLEGRO_COLOR borderColor = m_borderNormal;
    if (!m_enabled) {
        borderColor = m_borderDisabled;
    } else if (m_isPressed) {
        borderColor = m_borderPressed;
    } else if (m_isHovered) {
        borderColor = m_borderHover;
    }

    if (m_drawBackground) {
        al_draw_filled_rectangle(x, y, x + w, y + h, bgColor);
    }

    if (m_drawBorder && m_borderThickness > 0.0f) {
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
    auto [bx, by] = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto [bw, bh] = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    bx += context.offsetX;
    by += context.offsetY;

    return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
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
