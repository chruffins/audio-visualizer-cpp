#pragma once

#include <allegro5/allegro.h>
#include "graphics/drawable.hpp"
#include <functional>
#include <memory>

#define ALLEGRO_EVENT_UI ALLEGRO_GET_EVENT_TYPE('U','I','E','V')

namespace graphics {

class Drawable;

// RenderContext is defined in drawable.hpp

/**
 * Scroll event data passed to event handlers
 */
struct ScrollEvent {
    float dx;
    float dy;
    const RenderContext* context;  // Pointer to current render context
    
    ScrollEvent(float dx_, float dy_ = 0.0f, const RenderContext* ctx = nullptr)
        : dx(dx_), dy(dy_), context(ctx) {}
};

/**
 * Mouse event data passed to event handlers
 */
struct MouseEvent {
    float x;           // Screen coordinates
    float y;
    float localX;      // Local coordinates relative to the drawable
    float localY;
    int button;        // Mouse button (1=left, 2=right, 3=middle)
    bool doubleClick;  // True if this was a double-click
    const RenderContext* context;  // Pointer to current render context
    
    MouseEvent(float x_, float y_, int button_ = 1, bool doubleClick_ = false, const RenderContext* ctx = nullptr)
        : x(x_), y(y_), localX(x_), localY(y_), button(button_), doubleClick(doubleClick_), context(ctx) {
        // Compute local coordinates if context is provided
        if (context) {
            localX = x_ - context->offsetX;
            localY = y_ - context->offsetY;
        }
    }
};

/**
 * Keyboard event data passed to event handlers
 */
struct KeyboardEvent {
    int keycode;       // Allegro keycode
    unsigned int unichar;  // Unicode character
    bool repeat;       // True if key held down
    
    KeyboardEvent(int keycode_, unsigned int unichar_ = 0, bool repeat_ = false)
        : keycode(keycode_), unichar(unichar_), repeat(repeat_) {}
};

/**
 * Event handler interface and implementation combined.
 * Use callbacks for simple interactions, or subclass and override for complex ones.
 */
class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    
    // Callbacks - set these instead of subclassing
    std::function<bool(const MouseEvent&)> m_onMouseDown;
    std::function<bool(const MouseEvent&)> m_onMouseUp;
    std::function<bool(const MouseEvent&)> m_onMouseMove;
    std::function<bool(const MouseEvent&)> m_onMouseEnter;
    std::function<bool(const MouseEvent&)> m_onMouseLeave;
    std::function<bool(const ScrollEvent&)> m_onMouseScroll;
    
    std::function<bool(const KeyboardEvent&)> m_onKeyDown;
    std::function<bool(const KeyboardEvent&)> m_onKeyUp;
    std::function<bool(const KeyboardEvent&)> m_onKeyChar;
    
    std::function<void()> m_onFocusGained;
    std::function<void()> m_onFocusLost;
    
    // hitTest callback now takes RenderContext as well
    std::function<bool(float, float, const RenderContext&)> m_hitTest;
    std::function<bool()> m_isFocusable;
    std::function<bool()> m_isEnabled;
    
    // Virtual methods - override in subclass OR set callbacks above
    virtual bool onMouseDown(const MouseEvent& event) {
        return m_onMouseDown && m_onMouseDown(event);
    }
    virtual bool onMouseUp(const MouseEvent& event) {
        return m_onMouseUp && m_onMouseUp(event);
    }
    virtual bool onMouseMove(const MouseEvent& event) {
        return m_onMouseMove && m_onMouseMove(event);
    }
    virtual bool onMouseEnter(const MouseEvent& event) {
        return m_onMouseEnter && m_onMouseEnter(event);
    }
    virtual bool onMouseLeave(const MouseEvent& event) {
        return m_onMouseLeave && m_onMouseLeave(event);
    }
    virtual bool onMouseScroll(const ScrollEvent& event) {
        return m_onMouseScroll && m_onMouseScroll(event);
    }
    
    virtual bool onKeyDown(const KeyboardEvent& event) {
        return m_onKeyDown && m_onKeyDown(event);
    }
    virtual bool onKeyUp(const KeyboardEvent& event) {
        return m_onKeyUp && m_onKeyUp(event);
    }
    virtual bool onKeyChar(const KeyboardEvent& event) {
        return m_onKeyChar && m_onKeyChar(event);
    }
    
    virtual void onFocusGained() {
        if (m_onFocusGained) m_onFocusGained();
    }
    virtual void onFocusLost() {
        if (m_onFocusLost) m_onFocusLost();
    }
    
    virtual bool hitTest(float x, float y, const RenderContext& context) const {
        return m_hitTest && m_hitTest(x, y, context);
    }
    
    virtual bool isFocusable() const {
        return m_isFocusable && m_isFocusable();
    }
    
    virtual bool isEnabled() const {
        return !m_isEnabled || m_isEnabled();
    }
};

/**
 * Event dispatcher that routes Allegro events to interactive drawables.
 * Handles hit testing, focus management, and event propagation.
 */
class EventDispatcher {
public:
    EventDispatcher();
    ~EventDispatcher();

    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
    EventDispatcher(EventDispatcher&&) = delete;
    EventDispatcher& operator=(EventDispatcher&&) = delete;
    
    /**
     * Process an Allegro event and dispatch to appropriate handlers.
     * Returns true if the event was handled by a GUI element.
     */
    bool dispatchEvent(const ALLEGRO_EVENT& event);
    
    /**
     * Register a drawable as an event target (typically a root view).
     * Targets are stored as weak_ptr, so they don't need manual unregistration.
     */
    void addEventTarget(std::shared_ptr<IEventHandler> target);
    void removeEventTarget(std::shared_ptr<IEventHandler> target);
    void clearEventTargets();

    /**
     * Pass user-generated events to an Allegro queue
     */
    void registerEventSource(ALLEGRO_EVENT_QUEUE* queue) {
        al_register_event_source(queue, &eventSource);
    }

    const ALLEGRO_EVENT_SOURCE* getEventSource() const {
        return &eventSource;
    }
    
    /**
     * Focus management
     */
    void setFocus(std::shared_ptr<IEventHandler> handler);
    void clearFocus();
    std::shared_ptr<IEventHandler> getFocusedElement() const { return m_focusedElement.lock(); }
    
    /**
     * Debugging - get the element currently under the mouse
     */
    std::shared_ptr<IEventHandler> getHoveredElement() const { return m_hoveredElement.lock(); }

private:
    bool dispatchMouseEvent(const ALLEGRO_EVENT& event);
    bool dispatchKeyboardEvent(const ALLEGRO_EVENT& event);
    bool dispatchMouseMove(float x, float y);
    
    std::shared_ptr<IEventHandler> findTargetAt(float x, float y, const RenderContext& context);
    void cleanupExpiredTargets();  // Remove dead weak_ptrs
    
    std::vector<std::weak_ptr<IEventHandler>> m_eventTargets;
    std::weak_ptr<IEventHandler> m_focusedElement;
    std::weak_ptr<IEventHandler> m_hoveredElement;
    std::weak_ptr<IEventHandler> m_mouseDownTarget; // Track element where mouse was pressed

    ALLEGRO_EVENT_SOURCE eventSource;
    
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;
};

} // namespace graphics
