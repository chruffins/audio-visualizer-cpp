#include "core/allegro_init.hpp"
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <iostream>

namespace core {

bool initializeAllegro() {
    if (!al_init()) {
        std::cerr << "Failed to initialize Allegro\n";
        return false;
    }
    
    if (!al_install_mouse()) {
        std::cerr << "Failed to install mouse\n";
        return false;
    }
    
    if (!al_install_keyboard()) {
        std::cerr << "Failed to install keyboard\n";
        return false;
    }
    
    if (!al_install_audio()) {
        std::cerr << "Failed to install audio\n";
        return false;
    }
    
    if (!al_init_font_addon()) {
        std::cerr << "Failed to initialize font addon\n";
        return false;
    }
    
    if (!al_init_ttf_addon()) {
        std::cerr << "Failed to initialize TTF addon\n";
        return false;
    }
    
    if (!al_reserve_samples(16)) {
        std::cerr << "Failed to reserve audio samples\n";
        return false;
    }
    
    if (!al_init_acodec_addon()) {
        std::cerr << "Failed to initialize audio codec addon\n";
        return false;
    }
    
    if (!al_init_image_addon()) {
        std::cerr << "Failed to initialize image addon\n";
        return false;
    }
    
    if (!al_init_primitives_addon()) {
        std::cerr << "Failed to initialize primitives addon\n";
        return false;
    }
    
    return true;
}

} // namespace core
