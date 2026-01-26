#pragma once

#include "graphics/drawable.hpp"
#include "graphics/event_handler.hpp"
#include "graphics/uv.hpp"
#include <allegro5/allegro_font.h>
#include <functional>
#include <string>

namespace ui {

/**
 * Interactive button drawable that responds to mouse clicks.
 * Demonstrates the event handling pattern.
 */
class ButtonDrawable : public graphics::Drawable, public graphics::IEventHandler {
public:
    using ClickCallback = std::function<void()>;
    
    ButtonDrawable(graphics::UV position, graphics::UV size, const std::string& label = "");
    ~ButtonDrawable() override = default;
    
    // Drawable interface
    void draw(const graphics::RenderContext& context) const override;
    
    // IEventHandler interface
    bool onMouseDown(const graphics::MouseEvent& event) override;
    bool onMouseUp(const graphics::MouseEvent& event) override;
    bool onMouseEnter(const graphics::MouseEvent& event) override;
    bool onMouseLeave(const graphics::MouseEvent& event) override;
    bool hitTest(float x, float y) const override;
    bool isFocusable() const override { return true; }
    bool isEnabled() const override { return m_enabled; }
    
    // Button configuration
    void setLabel(const std::string& label) { m_label = label; }
    std::string getLabel() const { return m_label; }
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    void setFont(ALLEGRO_FONT* font) { m_font = font; }
    
    void setColors(ALLEGRO_COLOR normal, ALLEGRO_COLOR hover, ALLEGRO_COLOR pressed) {
        m_colorNormal = normal;
        m_colorHover = hover;
        m_colorPressed = pressed;
    }
    
    void setTextColor(ALLEGRO_COLOR color) { m_colorText = color; }
    
    void setDisabledColor(ALLEGRO_COLOR color) { m_colorDisabled = color; }
    
    // Event callback
    void setOnClick(ClickCallback callback) { m_onClick = callback; }
    
private:
    void updateBounds(const graphics::RenderContext& context) const;
    
    // Position and size
    graphics::UV m_position;
    graphics::UV m_size;
    
    // Cached screen-space bounds (mutable for lazy evaluation)
    mutable float m_cachedX = 0.0f;
    mutable float m_cachedY = 0.0f;
    mutable float m_cachedWidth = 0.0f;
    mutable float m_cachedHeight = 0.0f;
    mutable int m_lastScreenWidth = 0;
    mutable int m_lastScreenHeight = 0;
    
    // Visual properties
    std::string m_label;
    ALLEGRO_FONT* m_font = nullptr;
    
    // Colors
    ALLEGRO_COLOR m_colorNormal = al_map_rgb(70, 70, 90);
    ALLEGRO_COLOR m_colorHover = al_map_rgb(90, 90, 110);
    ALLEGRO_COLOR m_colorPressed = al_map_rgb(110, 110, 130);
    ALLEGRO_COLOR m_colorDisabled = al_map_rgb(50, 50, 60);
    ALLEGRO_COLOR m_colorText = al_map_rgb(255, 255, 255);
    
    // State
    bool m_enabled = true;
    bool m_isHovered = false;
    bool m_isPressed = false;
    
    // Callback
    ClickCallback m_onClick;
};

} // namespace ui
