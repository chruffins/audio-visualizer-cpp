#include "graphics/drawables/button.hpp"
#include "graphics/draw_shapes.hpp"
#include "graphics/state_color.hpp"
#include <allegro5/allegro_primitives.h>

namespace ui {

ButtonDrawable::ButtonDrawable(graphics::UV position, graphics::UV size, const std::string& label)
    : m_label(label) {
    setPosition(position);
    setSize(size);
}

void ButtonDrawable::draw(const graphics::RenderContext& context) const {
    // Culling: skip off-screen buttons
    if (isOffScreen(context)) {
        return;
    }
    
    const auto bounds = getScreenBounds(context);
    const float x = bounds.x;
    const float y = bounds.y;
    const float w = bounds.w;
    const float h = bounds.h;
    
    // Choose color based on state
    const ALLEGRO_COLOR bgColor = graphics::selectStateColor(
        m_enabled,
        m_isPressed,
        m_isHovered,
        m_colorNormal,
        m_colorHover,
        m_colorPressed,
        m_colorDisabled
    );
    
    ALLEGRO_COLOR borderColor = m_isHovered ? al_map_rgb(120, 120, 120) : al_map_rgb(80, 80, 80);
    graphics::drawFilledRectWithBorder(x, y, w, h, bgColor, borderColor, 2.0f);
    
    // Draw label text centered
    if (m_font && !m_label.empty()) {
        float textX = x + w / 2.0f;
        float textY = y + h / 2.0f - al_get_font_line_height(m_font) / 2.0f;
        
        ALLEGRO_COLOR textColor = m_enabled ? m_colorText : al_map_rgb(100, 100, 100);
        al_draw_text(m_font, textColor, textX, textY, ALLEGRO_ALIGN_CENTRE, m_label.c_str());
    }
}

bool ButtonDrawable::hitTest(float x, float y, const graphics::RenderContext& context) const {
    return hitTestRect(x, y, context);
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
