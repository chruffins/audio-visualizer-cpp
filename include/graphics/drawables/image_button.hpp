#pragma once

#include "graphics/drawables/image.hpp"
#include "graphics/drawable.hpp"
#include "graphics/event_handler.hpp"
#include <allegro5/allegro_color.h>
#include <functional>
#include <memory>
#include <string>

namespace ui {

class ImageButtonDrawable : public graphics::Drawable, public graphics::IEventHandler {
public:
    using ClickCallback = std::function<void()>;

    ImageButtonDrawable(graphics::UV position, graphics::UV size, ALLEGRO_BITMAP* bitmap = nullptr, bool owned = false);
    ~ImageButtonDrawable() override = default;

    void draw(const graphics::RenderContext& context) const override;

    bool onMouseDown(const graphics::MouseEvent& event) override;
    bool onMouseUp(const graphics::MouseEvent& event) override;
    bool onMouseEnter(const graphics::MouseEvent& event) override;
    bool onMouseLeave(const graphics::MouseEvent& event) override;
    bool hitTest(float x, float y, const graphics::RenderContext& context) const override;
    bool isFocusable() const override { return true; }
    bool isEnabled() const override { return m_enabled; }

    void setPosition(const graphics::UV& pos) {
        graphics::Drawable::setPosition(pos);
    }
    void setSize(const graphics::UV& sz) {
        graphics::Drawable::setSize(sz);
    }

    void setEnabled(bool enabled) { m_enabled = enabled; }

    void setOnClick(ClickCallback callback) { m_onClick = std::move(callback); }

    void setImageBitmap(ALLEGRO_BITMAP* bitmap, bool owned = false) { m_image.setBitmap(bitmap, owned); }
    bool loadImageFromFile(const std::string& filepath) { return m_image.loadFromFile(filepath); }
    void setImageModel(std::shared_ptr<ImageModel> model) { m_image.setImageModel(std::move(model)); }
    void setImageTint(ALLEGRO_COLOR color) { m_image.setTint(color); }
    void setImageScaleMode(ImageDrawable::ScaleMode mode) { m_image.setScaleMode(mode); }
    void setImageRotation(float radians) { m_image.setRotation(radians); }
    void setImageVisible(bool visible) { m_image.setVisible(visible); }

    void setDrawBackground(bool enabled) { m_drawBackground = enabled; }
    void setDrawBorder(bool enabled) { m_drawBorder = enabled; }

    void setBackgroundColors(ALLEGRO_COLOR normal, ALLEGRO_COLOR hover, ALLEGRO_COLOR pressed, ALLEGRO_COLOR disabled) {
        m_colorNormal = normal;
        m_colorHover = hover;
        m_colorPressed = pressed;
        m_colorDisabled = disabled;
    }

    void setBorderColors(ALLEGRO_COLOR normal, ALLEGRO_COLOR hover, ALLEGRO_COLOR pressed, ALLEGRO_COLOR disabled) {
        m_borderNormal = normal;
        m_borderHover = hover;
        m_borderPressed = pressed;
        m_borderDisabled = disabled;
    }

    void setBorderThickness(float thickness) { m_borderThickness = thickness; }

    void setImagePadding(float padding) {
        m_paddingLeft = padding;
        m_paddingTop = padding;
        m_paddingRight = padding;
        m_paddingBottom = padding;
    }
    void setImagePadding(float left, float top, float right, float bottom) {
        m_paddingLeft = left;
        m_paddingTop = top;
        m_paddingRight = right;
        m_paddingBottom = bottom;
    }

private:
    mutable ImageDrawable m_image;

    bool m_drawBackground = false;
    bool m_drawBorder = false;

    ALLEGRO_COLOR m_colorNormal = al_map_rgba(0, 0, 0, 0);
    ALLEGRO_COLOR m_colorHover = al_map_rgba(0, 0, 0, 0);
    ALLEGRO_COLOR m_colorPressed = al_map_rgba(0, 0, 0, 0);
    ALLEGRO_COLOR m_colorDisabled = al_map_rgba(0, 0, 0, 0);

    ALLEGRO_COLOR m_borderNormal = al_map_rgb(80, 80, 80);
    ALLEGRO_COLOR m_borderHover = al_map_rgb(120, 120, 120);
    ALLEGRO_COLOR m_borderPressed = al_map_rgb(160, 160, 160);
    ALLEGRO_COLOR m_borderDisabled = al_map_rgb(50, 50, 60);

    float m_borderThickness = 1.0f;

    float m_paddingLeft = 0.0f;
    float m_paddingTop = 0.0f;
    float m_paddingRight = 0.0f;
    float m_paddingBottom = 0.0f;

    bool m_enabled = true;
    bool m_isHovered = false;
    bool m_isPressed = false;

    ClickCallback m_onClick;
};

} // namespace ui
