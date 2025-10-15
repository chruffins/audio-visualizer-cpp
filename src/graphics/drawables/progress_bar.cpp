#include "graphics/drawables/progress_bar.hpp"
#include <allegro5/allegro_primitives.h>
#include <iostream>

void ui::ProgressBarDrawable::draw() const
{
    if (!model) return;

    // get screen size
    auto screenX = al_get_display_width(al_get_current_display());
    auto screenY = al_get_display_height(al_get_current_display());

    auto size = this->size.toScreenPos(screenX, screenY);
    auto position = this->position.toScreenPos(screenX, screenY);

    // draw background
    al_draw_filled_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, bgColor);

    // draw border
    al_draw_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, borderColor, borderThickness);

    // compute filled width based on model's progress
    float rel = model->getProgressRelative();

    al_draw_filled_rectangle(position.first, position.second, position.first + (size.first * rel), position.second + size.second, fgColor);
    al_draw_rectangle(position.first, position.second, position.first + size.first, position.second + size.second, borderColor, borderThickness);
}