#include "graphics/drawables/slider.hpp"

#include <cmath>
#include <algorithm>
#include <allegro5/allegro_primitives.h>

namespace ui {

void SliderDrawable::draw(const graphics::RenderContext& context) const {
    if (!enabled || !model) return;
    if (isOffScreen(context)) return;

    auto [x, y] = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto [w, h] = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    x += context.offsetX;
    y += context.offsetY;

    const float centerY = y + (h * 0.5f);
    const float halfTrack = trackHeight * 0.5f;
    const float thumbX = getThumbCenterX(x, w);

    al_draw_filled_rounded_rectangle(
        x,
        centerY - halfTrack,
        x + w,
        centerY + halfTrack,
        halfTrack,
        halfTrack,
        trackBackgroundColor
    );

    al_draw_filled_rounded_rectangle(
        x,
        centerY - halfTrack,
        thumbX,
        centerY + halfTrack,
        halfTrack,
        halfTrack,
        trackFillColor
    );

    ALLEGRO_COLOR activeThumbColor = thumbColor;
    if (dragging) {
        activeThumbColor = thumbDragColor;
    } else if (hovered) {
        activeThumbColor = thumbHoverColor;
    }

    al_draw_filled_circle(thumbX, centerY, thumbRadius, activeThumbColor);
}

bool SliderDrawable::onMouseDown(const graphics::MouseEvent& event) {
    if (!enabled || !model || !event.context || event.button != 1) return false;
    dragging = true;
    return updateValueFromMouseX(event.x, *event.context);
}

bool SliderDrawable::onMouseMove(const graphics::MouseEvent& event) {
    if (!enabled || !model || !event.context) return false;
    if (!dragging) return false;
    return updateValueFromMouseX(event.x, *event.context);
}

bool SliderDrawable::onMouseUp(const graphics::MouseEvent& event) {
    if (!enabled || !model || !event.context || event.button != 1) return false;

    const bool wasDragging = dragging;
    if (dragging) {
        updateValueFromMouseX(event.x, *event.context);
    }
    dragging = false;
    return wasDragging;
}

bool SliderDrawable::onMouseEnter(const graphics::MouseEvent&) {
    hovered = true;
    return false;
}

bool SliderDrawable::onMouseLeave(const graphics::MouseEvent&) {
    hovered = false;
    return false;
}

bool SliderDrawable::hitTest(float x, float y, const graphics::RenderContext& context) const {
    auto [left, top] = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto [width, height] = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    left += context.offsetX;
    top += context.offsetY;

    const float expandedTop = top - thumbRadius;
    const float expandedBottom = top + height + thumbRadius;
    return x >= left && x <= left + width && y >= expandedTop && y <= expandedBottom;
}

bool SliderDrawable::updateValueFromMouseX(float x, const graphics::RenderContext& context) {
    auto [left, top] = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto [width, height] = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    left += context.offsetX;

    if (width <= 0.0f) return false;

    const float normalized = std::clamp((x - left) / width, 0.0f, 1.0f);
    const float oldValue = model->getValue();
    model->setNormalizedValue(normalized);
    const float newValue = model->getValue();

    if (std::abs(newValue - oldValue) > 0.001f) {
        if (onValueChanged) {
            onValueChanged(newValue);
        }
    }

    return true;
}

float SliderDrawable::getThumbCenterX(float left, float width) const {
    if (!model) return left;
    return left + (width * model->getNormalizedValue());
}

} // namespace ui
