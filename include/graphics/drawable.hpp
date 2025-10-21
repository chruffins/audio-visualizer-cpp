#pragma once

#include <memory>
#include <vector>

namespace graphics {

struct RenderContext {};

class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void draw(const RenderContext& context) const = 0;
};

using DrawablePtr = std::shared_ptr<Drawable>;
using DrawableList = std::vector<DrawablePtr>;

} // namespace graphics