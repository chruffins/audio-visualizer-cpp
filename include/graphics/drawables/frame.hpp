#pragma once

#include <memory>
#include "graphics/drawables/container.hpp"
#include "graphics/drawable.hpp"
#include "graphics/uv.hpp"

namespace ui {

// Generic frame drawable that provides a bordered/styled container for custom content.
// The template parameter allows different drawable types to be managed within the frame.
class FrameDrawable : public ContainerDrawable {
public:

    FrameDrawable() = default;
    FrameDrawable(graphics::UV position, graphics::UV size)
        : ContainerDrawable(position, size) {}

    void setPadding(float p) noexcept { padding = p; }
    float getPadding() const noexcept { return padding; }

    void setMargin(float m) noexcept { margin = m; }
    float getMargin() const noexcept { return margin; }

    void draw(const graphics::RenderContext& context = {}) const override;

protected:
    float padding = 0.0f;
    float margin = 0.0f;
};

} // namespace ui
