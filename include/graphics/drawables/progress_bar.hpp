#pragma once

#include <memory>
#include "graphics/models/progress_bar.hpp"
#include <allegro5/color.h>

#include "graphics/drawable.hpp"

namespace ui
{
class ProgressBarDrawable : public graphics::Drawable
{
public:
    // owns or references a ProgressBar; use shared_ptr so multiple drawables can reflect the same model
    explicit ProgressBarDrawable(std::shared_ptr<ProgressBar> model)
        : model(std::move(model)) {}

    // setPosition and setSize inherited from Drawable

    void setBackgroundColor(ALLEGRO_COLOR c) noexcept { bgColor = c; }
    void setForegroundColor(ALLEGRO_COLOR c) noexcept { fgColor = c; }

    // render using your renderer implementation
    void draw(const graphics::RenderContext& context = {}) const override;
private:
    std::shared_ptr<ProgressBar> model;
    ALLEGRO_COLOR bgColor = al_map_rgb(32, 32, 32);
    ALLEGRO_COLOR fgColor = al_map_rgb(0, 191, 255);
    ALLEGRO_COLOR borderColor = al_map_rgb(255, 255, 255);
    int borderThickness = 1;

    void drawSquared(const graphics::RenderContext& context) const;
    void drawRounded(const graphics::RenderContext& context) const;
};
};