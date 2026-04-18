#pragma once

#include <memory>
#include <vector>
#include "graphics/uv.hpp"
#include <allegro5/allegro.h>

namespace graphics {

struct RenderContext {
    int screenWidth = 0;
    int screenHeight = 0;
    // Optional translation applied by container-like drawables so children
    // render relative to a local origin.
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    void* userData = nullptr; // optional backend-specific pointer
};

struct ScreenBounds {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void draw(const RenderContext& context) const = 0;
    
    // Position and size in UV coordinates
    void setPosition(const UV& pos) { position = pos; }
    UV getPosition() const { return position; }
    
    void setSize(const UV& sz) { size = sz; }
    UV getSize() const { return size; }

    ScreenBounds getScreenBounds(const RenderContext& context) const {
        auto [x, y] = getPosition().toScreenPos(
            static_cast<float>(context.screenWidth),
            static_cast<float>(context.screenHeight)
        );
        auto [w, h] = getSize().toScreenPos(
            static_cast<float>(context.screenWidth),
            static_cast<float>(context.screenHeight)
        );

        x += context.offsetX;
        y += context.offsetY;
        return ScreenBounds{x, y, w, h};
    }

    bool hitTestRect(float x, float y, const RenderContext& context) const {
        const auto bounds = getScreenBounds(context);
        return x >= bounds.x && x <= bounds.x + bounds.w &&
               y >= bounds.y && y <= bounds.y + bounds.h;
    }
    
    // Check if drawable is completely off-screen (accounts for clipping rectangle)
    bool isOffScreen(const RenderContext& context) const {
        const auto bounds = getScreenBounds(context);
        
        // Check against Allegro's actual clipping rectangle
        int clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        al_get_clipping_rectangle(&clipX, &clipY, &clipW, &clipH);
        
        // If no clipping is set (0,0,0,0), don't cull
        if (clipW == 0 || clipH == 0) {
            return false;
        }
        
        int clipRight = clipX + clipW;
        int clipBottom = clipY + clipH;
        
        return (bounds.x + bounds.w < clipX || bounds.x > clipRight ||
            bounds.y + bounds.h < clipY || bounds.y > clipBottom);
    }
    
protected:
    bool enabled = true;
    UV position{0.0f, 0.0f, 0.0f, 0.0f};
    UV size{1.0f, 1.0f, 0.0f, 0.0f};
};

using DrawablePtr = std::shared_ptr<Drawable>;
using DrawableList = std::vector<DrawablePtr>;
using DrawableObserverList = std::vector<Drawable*>;

} // namespace graphics