#pragma once

#include <memory>
#include "graphics/drawables/container.hpp"
#include "graphics/drawable.hpp"
#include "graphics/uv.hpp"
#include "graphics/event_handler.hpp"

namespace ui {

// Generic frame drawable that provides a bordered/styled container for custom content.
// The template parameter allows different drawable types to be managed within the frame.
class FrameDrawable : public ContainerDrawable, public graphics::IEventHandler {
public:

    FrameDrawable() = default;
    FrameDrawable(graphics::UV position, graphics::UV size)
        : ContainerDrawable(position, size) {}

    void setPadding(float p) noexcept { padding = p; }
    float getPadding() const noexcept { return padding; }

    void setMargin(float m) noexcept { margin = m; }
    float getMargin() const noexcept { return margin; }

    void draw(const graphics::RenderContext& context = {}) const override;

    // Event delegation to children
    bool onMouseDown(const graphics::MouseEvent& event) override {
        // First check if click is within frame bounds
        if (!hitTest(event.x, event.y)) {
            return graphics::IEventHandler::onMouseDown(event);
        }
        
        // Check children in reverse order (top-most first)
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
            if (!handler) continue;
            
            if (handler->hitTest(event.x, event.y)) {
                graphics::MouseEvent localEvent = event;
                if (handler->onMouseDown(localEvent)) {
                    return true;
                }
            }
        }
        return graphics::IEventHandler::onMouseDown(event);
    }

    bool onMouseUp(const graphics::MouseEvent& event) override {
        // Check children in reverse order (top-most first)
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
            if (!handler) continue;
            
            if (handler->hitTest(event.x, event.y)) {
                graphics::MouseEvent localEvent = event;
                if (handler->onMouseUp(localEvent)) {
                    return true;
                }
            }
        }
        return graphics::IEventHandler::onMouseUp(event);
    }

    bool onMouseMove(const graphics::MouseEvent& event) override {
        // Check children in reverse order (top-most first)
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
            if (!handler) continue;
            
            if (handler->hitTest(event.x, event.y)) {
                graphics::MouseEvent localEvent = event;
                if (handler->onMouseMove(localEvent)) {
                    return true;
                }
            }
        }
        return graphics::IEventHandler::onMouseMove(event);
    }

    bool onKeyDown(const graphics::KeyboardEvent& event) override {
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
            if (!handler || !handler->isFocusable()) continue;
            
            if (handler->onKeyDown(event)) {
                return true;
            }
        }
        return graphics::IEventHandler::onKeyDown(event);
    }

    bool onKeyUp(const graphics::KeyboardEvent& event) override {
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            auto handler = dynamic_cast<graphics::IEventHandler*>(*it);
            if (!handler || !handler->isFocusable()) continue;
            
            if (handler->onKeyUp(event)) {
                return true;
            }
        }
        return graphics::IEventHandler::onKeyUp(event);
    }
    
    bool hitTest(float x, float y) const override {
        float displayW = static_cast<float>(al_get_display_width(al_get_current_display()));
        float displayH = static_cast<float>(al_get_display_height(al_get_current_display()));
        auto sizePx = size.toScreenPos(displayW, displayH);
        auto posPx = position.toScreenPos(displayW, displayH);
        float absX = posPx.first;
        float absY = posPx.second;
        return x >= absX && x <= absX + sizePx.first && y >= absY && y <= absY + sizePx.second;
    }

protected:
    float padding = 0.0f;
    float margin = 0.0f;
};

} // namespace ui
