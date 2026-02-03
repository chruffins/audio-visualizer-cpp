#include "graphics/drawables/button.hpp"
#include <allegro5/allegro_primitives.h>

namespace ui {

ButtonDrawable::ButtonDrawable(graphics::UV position, graphics::UV size, const std::string& label)
    : m_label(label) {
    setPosition(position);
    setSize(size);
}

void ButtonDrawable::draw(const graphics::RenderContext& context) const {
    auto [x, y] = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto [w, h] = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    x += context.offsetX;
    y += context.offsetY;
    
    // Choose color based on state
    ALLEGRO_COLOR bgColor;
    if (!m_enabled) {
        bgColor = m_colorDisabled;
    } else if (m_isPressed) {
        bgColor = m_colorPressed;
    } else if (m_isHovered) {
        bgColor = m_colorHover;
    } else {
        bgColor = m_colorNormal;
    }
    
    // Draw button background
    al_draw_filled_rectangle(x, y, x + w, y + h, bgColor);
    
    // Draw border
    ALLEGRO_COLOR borderColor = m_isHovered ? al_map_rgb(120, 120, 120) : al_map_rgb(80, 80, 80);
    al_draw_rectangle(x, y, x + w, y + h, borderColor, 2.0f);
    
    // Draw label text centered
    if (m_font && !m_label.empty()) {
        float textX = x + w / 2.0f;
        float textY = y + h / 2.0f - al_get_font_line_height(m_font) / 2.0f;
        
        ALLEGRO_COLOR textColor = m_enabled ? m_colorText : al_map_rgb(100, 100, 100);
        al_draw_text(m_font, textColor, textX, textY, ALLEGRO_ALIGN_CENTRE, m_label.c_str());
    }
}

bool ButtonDrawable::hitTest(float x, float y, const graphics::RenderContext& context) const {
    auto [bx, by] = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto [bw, bh] = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    bx += context.offsetX;
    by += context.offsetY;
    
    return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
}

bool ButtonDrawable::onMouseDown(const graphics::MouseEvent& event) {
    if (!m_enabled) return false;
    
    m_isPressed = true;
    return true; // Consume event
}

bool ButtonDrawable::onMouseUp(const graphics::MouseEvent& event) {
    if (!m_enabled) return false;
    
    bool wasPressed = m_isPressed;
    m_isPressed = false;
    
    // Only trigger click if mouse is still over button
    // Use event's context to get proper hit test with offsets
    if (wasPressed && event.context && hitTest(event.x, event.y, *event.context)) {
        if (m_onClick) {
            m_onClick();
        }
    }
    
    return wasPressed; // Consume if we handled the press
}

bool ButtonDrawable::onMouseEnter(const graphics::MouseEvent& event) {
    m_isHovered = true;
    return false; // Don't consume, allow other handlers
}

bool ButtonDrawable::onMouseLeave(const graphics::MouseEvent& event) {
    m_isHovered = false;
    m_isPressed = false; // Cancel press if mouse leaves
    return false;
}

} // namespace ui
