#include "graphics/drawables/button.hpp"
#include <allegro5/allegro_primitives.h>

namespace ui {

ButtonDrawable::ButtonDrawable(graphics::UV position, graphics::UV size, const std::string& label)
    : m_label(label) {
    setPosition(position);
    setSize(size);
}

void ButtonDrawable::updateBounds(const graphics::RenderContext& context) const {
    // Only recalculate if screen dimensions changed
    if (m_lastScreenWidth == context.screenWidth && m_lastScreenHeight == context.screenHeight) {
        return;
    }
    
    m_lastScreenWidth = context.screenWidth;
    m_lastScreenHeight = context.screenHeight;
    
    // Calculate absolute position and size from UV coordinates
    auto pos = getPosition().toScreenPos(context.screenWidth, context.screenHeight);
    auto size = getSize().toScreenPos(context.screenWidth, context.screenHeight);
    
    m_cachedX = pos.first + context.offsetX;
    m_cachedY = pos.second + context.offsetY;
    m_cachedWidth = size.first;
    m_cachedHeight = size.second;
}

void ButtonDrawable::draw(const graphics::RenderContext& context) const {
    updateBounds(context);
    
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
    al_draw_filled_rectangle(
        m_cachedX, m_cachedY,
        m_cachedX + m_cachedWidth,
        m_cachedY + m_cachedHeight,
        bgColor
    );
    
    // Draw border
    ALLEGRO_COLOR borderColor = m_isHovered ? al_map_rgb(120, 120, 120) : al_map_rgb(80, 80, 80);
    al_draw_rectangle(
        m_cachedX, m_cachedY,
        m_cachedX + m_cachedWidth,
        m_cachedY + m_cachedHeight,
        borderColor, 2.0f
    );
    
    // Draw label text centered
    if (m_font && !m_label.empty()) {
        float textX = m_cachedX + m_cachedWidth / 2.0f;
        float textY = m_cachedY + m_cachedHeight / 2.0f - al_get_font_line_height(m_font) / 2.0f;
        
        ALLEGRO_COLOR textColor = m_enabled ? m_colorText : al_map_rgb(100, 100, 100);
        al_draw_text(m_font, textColor, textX, textY, ALLEGRO_ALIGN_CENTRE, m_label.c_str());
    }
}

bool ButtonDrawable::hitTest(float x, float y) const {
    return x >= m_cachedX && x <= m_cachedX + m_cachedWidth &&
           y >= m_cachedY && y <= m_cachedY + m_cachedHeight;
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
    if (wasPressed && hitTest(event.x, event.y)) {
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
