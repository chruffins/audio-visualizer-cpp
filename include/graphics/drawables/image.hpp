#pragma once

#include <allegro5/allegro.h>
#include <memory>
#include <string>

#include "graphics/drawable.hpp"
#include "graphics/uv.hpp"
#include "graphics/alignment.hpp"

namespace ui {
class ImageModel;


/**
 * @brief A drawable for rendering ALLEGRO_BITMAP images
 * 
 * Supports positioning via UV coordinates, scaling modes (stretch, fit, fill),
 * tinting, and rotation. The bitmap can be owned by this drawable or shared.
 */
class ImageDrawable : public graphics::Drawable {
public:
    enum class ScaleMode {
        STRETCH,  // Stretch to fill the size (may distort aspect ratio)
        FIT,      // Scale to fit within size (preserve aspect ratio, may letterbox)
        FILL,     // Scale to fill size (preserve aspect ratio, may crop)
        NONE      // Draw at original bitmap size, ignore size parameter
    };

    // Default constructor (no bitmap)
    ImageDrawable() = default;

    // Constructor with bitmap
    explicit ImageDrawable(ALLEGRO_BITMAP* bitmap, bool owned = false)
        : bitmap(bitmap), ownsBitmap(owned) {}

    // Constructor with position and size
    ImageDrawable(ALLEGRO_BITMAP* bitmap, graphics::UV position, graphics::UV size, bool owned = false)
        : bitmap(bitmap), position(position), size(size), ownsBitmap(owned) {}

    ~ImageDrawable() override {
        if (ownsBitmap && bitmap) {
            al_destroy_bitmap(bitmap);
        }
    }

    // Disable copy (because we may own the bitmap)
    ImageDrawable(const ImageDrawable&) = delete;
    ImageDrawable& operator=(const ImageDrawable&) = delete;

    // Enable move
    ImageDrawable(ImageDrawable&& other) noexcept
        : bitmap(other.bitmap), ownsBitmap(other.ownsBitmap),
          position(other.position), size(other.size),
          tint(other.tint), scaleMode(other.scaleMode),
          rotation(other.rotation), visible(other.visible),
          horizontalAlign(other.horizontalAlign),
          verticalAlign(other.verticalAlign) {
        other.bitmap = nullptr;
        other.ownsBitmap = false;
    }

    ImageDrawable& operator=(ImageDrawable&& other) noexcept {
        if (this != &other) {
            if (ownsBitmap && bitmap) {
                al_destroy_bitmap(bitmap);
            }
            bitmap = other.bitmap;
            ownsBitmap = other.ownsBitmap;
            position = other.position;
            size = other.size;
            tint = other.tint;
            scaleMode = other.scaleMode;
            rotation = other.rotation;
            visible = other.visible;
            horizontalAlign = other.horizontalAlign;
            verticalAlign = other.verticalAlign;
            other.bitmap = nullptr;
            other.ownsBitmap = false;
        }
        return *this;
    }

    // Setters
    ImageDrawable& setBitmap(ALLEGRO_BITMAP* bmp, bool owned = false) {
        if (ownsBitmap && bitmap) {
            al_destroy_bitmap(bitmap);
        }
        bitmap = bmp;
        ownsBitmap = owned;
        return *this;
    }

    ImageDrawable& setPosition(graphics::UV pos) {
        position = pos;
        return *this;
    }

    ImageDrawable& setSize(graphics::UV sz) {
        size = sz;
        return *this;
    }

    ImageDrawable& setTint(ALLEGRO_COLOR c) {
        tint = c;
        return *this;
    }

    ImageDrawable& setScaleMode(ScaleMode mode) {
        scaleMode = mode;
        return *this;
    }

    ImageDrawable& setRotation(float radians) {
        rotation = radians;
        return *this;
    }

    ImageDrawable& setVisible(bool v) {
        visible = v;
        return *this;
    }

    ImageDrawable& setHorizontalAlignment(graphics::HorizontalAlignment align) {
        horizontalAlign = align;
        return *this;
    }

    ImageDrawable& setVerticalAlignment(graphics::VerticalAlignment align) {
        verticalAlign = align;
        return *this;
    }

    // Set an ImageModel to get bitmap from (doesn't take ownership)
    ImageDrawable& setImageModel(std::shared_ptr<ImageModel> model) {
        imageModel = model;
        updateBitmapFromModel();
        return *this;
    }

    std::shared_ptr<ImageModel> getImageModel() const { return imageModel; }

    // Getters
    ALLEGRO_BITMAP* getBitmap() const { return bitmap; }
    graphics::UV getPosition() const { return position; }
    graphics::UV getSize() const { return size; }
    ALLEGRO_COLOR getTint() const { return tint; }
    ScaleMode getScaleMode() const { return scaleMode; }
    float getRotation() const { return rotation; }
    bool isVisible() const { return visible; }

    // Load bitmap from file (takes ownership)
    bool loadFromFile(const std::string& filepath) {
        if (ownsBitmap && bitmap) {
            al_destroy_bitmap(bitmap);
        }
        bitmap = al_load_bitmap(filepath.c_str());
        ownsBitmap = (bitmap != nullptr);
        return bitmap != nullptr;
    }

    // Drawable interface
    void draw(const graphics::RenderContext& context) const override;

private:
    void updateBitmapFromModel();

    ALLEGRO_BITMAP* bitmap = nullptr;
    bool ownsBitmap = false;
    std::shared_ptr<ImageModel> imageModel;

    graphics::UV position{0.0f, 0.0f, 0.0f, 0.0f}; // default to top-left
    graphics::UV size{1.0f, 1.0f, 0.0f, 0.0f};     // default to full screen
    ALLEGRO_COLOR tint = al_map_rgb(255, 255, 255); // white = no tint
    ScaleMode scaleMode = ScaleMode::STRETCH;
    float rotation = 0.0f; // radians
    bool visible = true;
    graphics::HorizontalAlignment horizontalAlign = graphics::HorizontalAlignment::CENTER;
    graphics::VerticalAlignment verticalAlign = graphics::VerticalAlignment::CENTER;

    struct DrawParams {
        float dx, dy;      // destination position
        float dw, dh;      // destination size
        float sx, sy;      // source position (for cropping)
        float sw, sh;      // source size (for cropping)
        float cx, cy;      // center point for rotation
    };

    DrawParams calculateDrawParams(const graphics::RenderContext& context) const;
};

} // namespace ui
