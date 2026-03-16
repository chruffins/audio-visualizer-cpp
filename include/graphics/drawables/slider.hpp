#pragma once

#include <functional>
#include <memory>
#include <allegro5/color.h>

#include "graphics/drawable.hpp"
#include "graphics/event_handler.hpp"
#include "graphics/models/slider.hpp"

namespace ui {

class SliderDrawable : public graphics::Drawable, public graphics::IEventHandler {
public:
    explicit SliderDrawable(std::shared_ptr<Slider> model)
        : model(std::move(model)) {}

    void draw(const graphics::RenderContext& context) const override;

    bool onMouseDown(const graphics::MouseEvent& event) override;
    bool onMouseMove(const graphics::MouseEvent& event) override;
    bool onMouseUp(const graphics::MouseEvent& event) override;
    bool onMouseEnter(const graphics::MouseEvent& event) override;
    bool onMouseLeave(const graphics::MouseEvent& event) override;

    bool hitTest(float x, float y, const graphics::RenderContext& context) const override;
    bool isFocusable() const override { return true; }
    bool isEnabled() const override { return enabled; }

    void setEnabled(bool isEnabled) { enabled = isEnabled; }
    void setOnValueChanged(std::function<void(float)> callback) { onValueChanged = std::move(callback); }

    void setTrackColors(ALLEGRO_COLOR background, ALLEGRO_COLOR fill) {
        trackBackgroundColor = background;
        trackFillColor = fill;
    }

    void setThumbColors(ALLEGRO_COLOR normal, ALLEGRO_COLOR hover, ALLEGRO_COLOR draggingColor) {
        thumbColor = normal;
        thumbHoverColor = hover;
        thumbDragColor = draggingColor;
    }

private:
    std::shared_ptr<Slider> model;
    std::function<void(float)> onValueChanged;

    bool enabled = true;
    bool hovered = false;
    bool dragging = false;

    float trackHeight = 6.0f;
    float thumbRadius = 7.0f;

    ALLEGRO_COLOR trackBackgroundColor = al_map_rgb(40, 40, 56);
    ALLEGRO_COLOR trackFillColor = al_map_rgb(0, 191, 255);
    ALLEGRO_COLOR thumbColor = al_map_rgb(220, 220, 230);
    ALLEGRO_COLOR thumbHoverColor = al_map_rgb(245, 245, 255);
    ALLEGRO_COLOR thumbDragColor = al_map_rgb(255, 255, 255);

    bool updateValueFromMouseX(float x, const graphics::RenderContext& context);
    float getThumbCenterX(float left, float width) const;
};

} // namespace ui
