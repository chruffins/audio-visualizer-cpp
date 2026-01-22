#include "graphics/drawables/image.hpp"
#include "graphics/models/image.hpp"
#include <algorithm>
#include <cmath>

using namespace ui;

void ImageDrawable::updateBitmapFromModel() {
    if (imageModel && imageModel->hasBitmap()) {
        bitmap = imageModel->getPrimaryBitmap();
        ownsBitmap = false; // ImageModel owns the bitmap
    }
}

void ImageDrawable::draw(const graphics::RenderContext& context) const {
    if (!visible || !bitmap) {
        return;
    }

    DrawParams params = calculateDrawParams(context);

    // Apply rotation if needed
    if (rotation != 0.0f) {
        al_draw_tinted_scaled_rotated_bitmap_region(
            bitmap,
            params.sx, params.sy, params.sw, params.sh,
            tint,
            params.cx, params.cy,           // center point in source bitmap
            params.dx + params.dw / 2.0f,   // destination center x
            params.dy + params.dh / 2.0f,   // destination center y
            params.dw / params.sw,          // scale x
            params.dh / params.sh,          // scale y
            rotation,
            0 // flags
        );
    } else {
        al_draw_tinted_scaled_bitmap(
            bitmap,
            tint,
            params.sx, params.sy, params.sw, params.sh,
            params.dx, params.dy, params.dw, params.dh,
            0 // flags
        );
    }
}

ImageDrawable::DrawParams ImageDrawable::calculateDrawParams(const graphics::RenderContext& context) const {
    DrawParams params;

    // Convert UV to screen coordinates
    auto [x, y] = position.toScreenPos(static_cast<float>(context.screenWidth), 
                                       static_cast<float>(context.screenHeight));
    auto [w, h] = size.toScreenPos(static_cast<float>(context.screenWidth), 
                                    static_cast<float>(context.screenHeight));
    
    // Apply container offset
    x += context.offsetX;
    y += context.offsetY;

    // Get bitmap dimensions
    int bmpW = al_get_bitmap_width(bitmap);
    int bmpH = al_get_bitmap_height(bitmap);
    float bmpAspect = static_cast<float>(bmpW) / static_cast<float>(bmpH);

    // Default: use full source bitmap
    params.sx = 0.0f;
    params.sy = 0.0f;
    params.sw = static_cast<float>(bmpW);
    params.sh = static_cast<float>(bmpH);
    params.cx = params.sw / 2.0f;
    params.cy = params.sh / 2.0f;

    switch (scaleMode) {
        case ScaleMode::NONE: {
            // Draw at original size, aligned within the target area
            params.dw = static_cast<float>(bmpW);
            params.dh = static_cast<float>(bmpH);
            
            // Calculate alignment offsets
            float xOffset = 0.0f;
            float yOffset = 0.0f;
            
            switch (horizontalAlign) {
                case graphics::HorizontalAlignment::LEFT:
                    xOffset = 0.0f;
                    break;
                case graphics::HorizontalAlignment::CENTER:
                    xOffset = (w - params.dw) / 2.0f;
                    break;
                case graphics::HorizontalAlignment::RIGHT:
                    xOffset = w - params.dw;
                    break;
            }
            
            switch (verticalAlign) {
                case graphics::VerticalAlignment::TOP:
                    yOffset = 0.0f;
                    break;
                case graphics::VerticalAlignment::CENTER:
                    yOffset = (h - params.dh) / 2.0f;
                    break;
                case graphics::VerticalAlignment::BOTTOM:
                    yOffset = h - params.dh;
                    break;
            }
            
            params.dx = x + xOffset;
            params.dy = y + yOffset;
            break;
        }
        
        case ScaleMode::STRETCH: {
            // Simple stretch to fill
            params.dx = x;
            params.dy = y;
            params.dw = w;
            params.dh = h;
            break;
        }
        
        case ScaleMode::FIT: {
            // Scale to fit, preserve aspect ratio (letterbox)
            float targetAspect = w / h;
            
            if (bmpAspect > targetAspect) {
                // Bitmap is wider, fit to width
                params.dw = w;
                params.dh = w / bmpAspect;
            } else {
                // Bitmap is taller, fit to height
                params.dh = h;
                params.dw = h * bmpAspect;
            }
            
            // Center within target area (can be aligned differently if needed)
            float xOffset = 0.0f;
            float yOffset = 0.0f;
            
            switch (horizontalAlign) {
                case graphics::HorizontalAlignment::LEFT:
                    xOffset = 0.0f;
                    break;
                case graphics::HorizontalAlignment::CENTER:
                    xOffset = (w - params.dw) / 2.0f;
                    break;
                case graphics::HorizontalAlignment::RIGHT:
                    xOffset = w - params.dw;
                    break;
            }
            
            switch (verticalAlign) {
                case graphics::VerticalAlignment::TOP:
                    yOffset = 0.0f;
                    break;
                case graphics::VerticalAlignment::CENTER:
                    yOffset = (h - params.dh) / 2.0f;
                    break;
                case graphics::VerticalAlignment::BOTTOM:
                    yOffset = h - params.dh;
                    break;
            }
            
            params.dx = x + xOffset;
            params.dy = y + yOffset;
            break;
        }
        
        case ScaleMode::FILL: {
            // Scale to fill, preserve aspect ratio (crop)
            float targetAspect = w / h;
            
            if (bmpAspect > targetAspect) {
                // Bitmap is wider, crop sides
                float usableWidth = bmpH * targetAspect;
                params.sx = (bmpW - usableWidth) / 2.0f;
                params.sw = usableWidth;
            } else {
                // Bitmap is taller, crop top/bottom
                float usableHeight = bmpW / targetAspect;
                params.sy = (bmpH - usableHeight) / 2.0f;
                params.sh = usableHeight;
            }
            
            params.dx = x;
            params.dy = y;
            params.dw = w;
            params.dh = h;
            
            // Update center point for rotation
            params.cx = params.sw / 2.0f;
            params.cy = params.sh / 2.0f;
            break;
        }
    }

    return params;
}
