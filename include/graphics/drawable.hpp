#pragma once

#include <memory>
#include <vector>
#include "graphics/uv.hpp"

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

class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void draw(const RenderContext& context) const = 0;
    
    // Position and size in UV coordinates
    void setPosition(const UV& pos) { position = pos; }
    UV getPosition() const { return position; }
    
    void setSize(const UV& sz) { size = sz; }
    UV getSize() const { return size; }
    
protected:
    bool enabled = true;
    UV position{0.0f, 0.0f, 0.0f, 0.0f};
    UV size{1.0f, 1.0f, 0.0f, 0.0f};
};

using DrawablePtr = std::shared_ptr<Drawable>;
using DrawableList = std::vector<DrawablePtr>;
using DrawableObserverList = std::vector<Drawable*>;

} // namespace graphics