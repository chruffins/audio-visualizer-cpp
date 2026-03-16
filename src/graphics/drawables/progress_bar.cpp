#include "graphics/drawables/progress_bar.hpp"
#include <allegro5/allegro_primitives.h>
#include <iostream>

void ui::ProgressBarDrawable::draw(const graphics::RenderContext& context) const
{
    drawSquared(context);
}

bool ui::ProgressBarDrawable::hitTest(float x, float y, const graphics::RenderContext& context) const
{
    auto size = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto position = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    position.first += context.offsetX;
    position.second += context.offsetY;

    return x >= position.first && x <= position.first + size.first
        && y >= position.second && y <= position.second + size.second;
}

void ui::ProgressBarDrawable::drawSquared(const graphics::RenderContext& context) const
{
    if (!model) return;
    
    // Culling: skip off-screen progress bars
    if (isOffScreen(context)) {
        return;
    }

    // use sizes from context
    auto size = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto position = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    position.first += context.offsetX;
    position.second += context.offsetY;

    // draw background
    al_draw_filled_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, bgColor);

    // draw border
    al_draw_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, borderColor, borderThickness);

    // compute filled width based on model's progress
    float rel = model->getProgressRelative();

    al_draw_filled_rectangle(position.first, position.second, position.first + (size.first * rel), position.second + size.second, fgColor);
    al_draw_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, borderColor, borderThickness);
}

void ui::ProgressBarDrawable::drawRounded(const graphics::RenderContext& context) const
{
    if (!model) return;

    auto size = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    auto position = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
    position.first += context.offsetX;
    position.second += context.offsetY;

    float radius = size.second / 2.0f;

    // draw background
    al_draw_filled_rounded_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, radius, radius, bgColor);

    // draw border
    al_draw_rounded_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, radius, radius, borderColor, borderThickness);

    // compute filled width based on model's progress
    float rel = model->getProgressRelative();

    al_draw_filled_rounded_rectangle(position.first, position.second, position.first + (size.first * rel), position.second + size.second, radius, radius, fgColor);
    al_draw_rounded_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, radius, radius, borderColor, borderThickness);
}