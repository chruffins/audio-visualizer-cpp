#include "graphics/event_handler.hpp"
#include "graphics/drawable.hpp"
#include <algorithm>
#include <iostream>

namespace graphics {

EventDispatcher::EventDispatcher() 
    : m_lastMouseX(0.0f), m_lastMouseY(0.0f) {
}

void EventDispatcher::addEventTarget(std::shared_ptr<IEventHandler> target) {
    if (target) {
        m_eventTargets.push_back(target);
    }
}

void EventDispatcher::removeEventTarget(std::shared_ptr<IEventHandler> target) {
    auto it = std::find_if(m_eventTargets.begin(), m_eventTargets.end(),
        [&target](const std::weak_ptr<IEventHandler>& weak) {
            auto locked = weak.lock();
            return locked == target;
        });
    if (it != m_eventTargets.end()) {
        m_eventTargets.erase(it);
    }
}

void EventDispatcher::clearEventTargets() {
    m_eventTargets.clear();
    clearFocus();
}

void EventDispatcher::cleanupExpiredTargets() {
    auto it = std::remove_if(m_eventTargets.begin(), m_eventTargets.end(),
        [](const std::weak_ptr<IEventHandler>& weak) {
            return weak.expired();
        });
    m_eventTargets.erase(it, m_eventTargets.end());
}

void EventDispatcher::setFocus(std::shared_ptr<IEventHandler> handler) {
    auto currentFocus = m_focusedElement.lock();
    if (currentFocus == handler) {
        return; // Already focused
    }
    
    // Notify old element of focus loss
    if (currentFocus) {
        currentFocus->onFocusLost();
    }
    
    // Set new focus
    m_focusedElement = handler;
    
    // Notify new element of focus gain
    if (handler) {
        handler->onFocusGained();
    }
}

void EventDispatcher::clearFocus() {
    auto currentFocus = m_focusedElement.lock();
    if (currentFocus) {
        currentFocus->onFocusLost();
    }
    m_focusedElement.reset();
}

bool EventDispatcher::dispatchEvent(const ALLEGRO_EVENT& event) {
    switch (event.type) {
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        case ALLEGRO_EVENT_MOUSE_AXES:
            return dispatchMouseEvent(event);
            
        case ALLEGRO_EVENT_KEY_DOWN:
        case ALLEGRO_EVENT_KEY_UP:
        case ALLEGRO_EVENT_KEY_CHAR:
            return dispatchKeyboardEvent(event);
            
        default:
            return false;
    }
}

bool EventDispatcher::dispatchMouseEvent(const ALLEGRO_EVENT& event) {
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    
    switch (event.type) {
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN: {
            mouseX = event.mouse.x;
            mouseY = event.mouse.y;
            auto target = findTargetAt(mouseX, mouseY);
            
            if (target && target->isEnabled()) {
                // Store the target where mouse was pressed
                m_mouseDownTarget = target;
                
                // Set focus if focusable
                if (target->isFocusable()) {
                    setFocus(target);
                }
                
                MouseEvent mouseEvent(mouseX, mouseY, event.mouse.button);
                return target->onMouseDown(mouseEvent);
            }
            
            // Click on empty space clears focus
            clearFocus();
            return false;
        }
        
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP: {
            mouseX = event.mouse.x;
            mouseY = event.mouse.y;
            
            // Mouse up goes to the element where mouse was originally pressed
            auto target = m_mouseDownTarget.lock();
            if (target && target->isEnabled()) {
                MouseEvent mouseEvent(mouseX, mouseY, event.mouse.button);
                bool handled = target->onMouseUp(mouseEvent);
                m_mouseDownTarget.reset();
                return handled;
            }
            
            m_mouseDownTarget.reset();
            return false;
        }
        
        case ALLEGRO_EVENT_MOUSE_AXES: {
            mouseX = event.mouse.x;
            mouseY = event.mouse.y;
            
            // Handle mouse wheel scrolling
            if (event.mouse.dz != 0 || event.mouse.dw != 0) {
                auto target = findTargetAt(mouseX, mouseY);
                if (target && target->isEnabled()) {
                    return target->onMouseScroll(event.mouse.dw, event.mouse.dz);
                }
                return false;
            }
            
            // Handle mouse movement
            return dispatchMouseMove(mouseX, mouseY);
        }
        
        default:
            return false;
    }
}

bool EventDispatcher::dispatchMouseMove(float x, float y) {
    m_lastMouseX = x;
    m_lastMouseY = y;
    
    auto newTarget = findTargetAt(x, y);
    auto oldTarget = m_hoveredElement.lock();
    
    // Handle hover state changes
    if (newTarget != oldTarget) {
        if (oldTarget) {
            MouseEvent leaveEvent(x, y);
            oldTarget->onMouseLeave(leaveEvent);
        }
        
        if (newTarget) {
            MouseEvent enterEvent(x, y);
            newTarget->onMouseEnter(enterEvent);
        }
        
        m_hoveredElement = newTarget;
    }
    
    // Send move event to current target
    if (newTarget && newTarget->isEnabled()) {
        MouseEvent moveEvent(x, y);
        return newTarget->onMouseMove(moveEvent);
    }
    
    return false;
}

bool EventDispatcher::dispatchKeyboardEvent(const ALLEGRO_EVENT& event) {
    auto focused = m_focusedElement.lock();
    if (!focused || !focused->isEnabled()) {
        return false;
    }
    
    KeyboardEvent keyEvent(
        event.keyboard.keycode,
        event.keyboard.unichar,
        event.keyboard.repeat
    );
    
    switch (event.type) {
        case ALLEGRO_EVENT_KEY_DOWN:
            return focused->onKeyDown(keyEvent);
        case ALLEGRO_EVENT_KEY_UP:
            return focused->onKeyUp(keyEvent);
        case ALLEGRO_EVENT_KEY_CHAR:
            return focused->onKeyChar(keyEvent);
        default:
            return false;
    }
}

std::shared_ptr<IEventHandler> EventDispatcher::findTargetAt(float x, float y) {
    // Cleanup expired targets periodically
    cleanupExpiredTargets();
    
    // Search in reverse order (top to bottom) to find the topmost element
    for (auto it = m_eventTargets.rbegin(); it != m_eventTargets.rend(); ++it) {
        auto target = it->lock();
        if (target && target->isEnabled() && target->hitTest(x, y)) {
            return target;
        }
    }
    return nullptr;
}

} // namespace graphics
