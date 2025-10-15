#pragma once

#include <memory>
#include "graphics/models/progress_bar.hpp"
#include <allegro5/color.h>

namespace ui
{
class ProgressBarDrawable
{
public:
    // owns or references a ProgressBar; use shared_ptr so multiple drawables can reflect the same model
    explicit ProgressBarDrawable(std::shared_ptr<ProgressBar> model)
        : model(std::move(model)) {}

    void setPosition(const graphics::UV& p) noexcept { position = p; }
    graphics::UV getPosition() const noexcept { return position; }

    void setSize(const graphics::UV& s) noexcept { size = s; }
    graphics::UV getSize() const noexcept { return size; }

    void setBackgroundColor(ALLEGRO_COLOR c) noexcept { bgColor = c; }
    void setForegroundColor(ALLEGRO_COLOR c) noexcept { fgColor = c; }

    // render using your renderer implementation
    void draw() const;
private:
    std::shared_ptr<ProgressBar> model;
    graphics::UV position{0.2f, 0.8f, 0.0f, 0.0f};
    graphics::UV size{0.6f, 0.05f, 0.0f, 0.0f};
    ALLEGRO_COLOR bgColor = al_map_rgb(32, 32, 32);
    ALLEGRO_COLOR fgColor = al_map_rgb(0, 191, 255);
    ALLEGRO_COLOR borderColor = al_map_rgb(255, 255, 255);
    int borderThickness = 1;
};
};