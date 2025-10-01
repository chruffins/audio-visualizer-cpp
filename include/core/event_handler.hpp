#pragma once
#include <allegro5/allegro.h>
#include <core/inputstate.hpp>
#include <core/app_state.hpp>

namespace core {
class EventHandler {
public:
    EventHandler(AppState& appState);
    ~EventHandler();

    void processInput(ALLEGRO_EVENT *event);
    void resetInput();
private:
    AppState& appState;
};
};