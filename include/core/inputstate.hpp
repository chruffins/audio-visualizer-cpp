#pragma once
#include <allegro5/allegro.h>

#include<cstdint>

namespace core {
struct InputState {
    bool key_down[ALLEGRO_KEY_MAX];
    bool key_state[ALLEGRO_KEY_MAX];
    bool mouse_down[10];
    int mouse_dx;
    int mouse_dy;

    InputState() {
        clear();
    }

    void resetDeltas() {
        mouse_dx = 0;
        mouse_dy = 0;
    }

    void clear() {
        for (int i = 0; i < ALLEGRO_KEY_MAX; i++) {
            key_down[i] = false;
            key_state[i] = false;
        }
        for (int i = 0; i < 10; i++) {
            mouse_down[i] = false;
        }
        mouse_dx = 0;
        mouse_dy = 0;
    }

    bool isKeyDown(int keycode) const {
        if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX) return false;
        return key_down[keycode];
    }
};
};