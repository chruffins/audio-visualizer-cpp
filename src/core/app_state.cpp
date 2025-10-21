#define MINIMP3_IMPLEMENTATION

#include <core/app_state.hpp>
#include "mp3/mp3_streaming.hpp"
#include "music/album.hpp"

namespace core {
bool AppState::init() {
    static bool initialized = false;
    if (initialized) return true; // Already initialized

    mp3streaming::addMP3Support();

    // update some flags
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR | ALLEGRO_VIDEO_BITMAP | ALLEGRO_MIPMAP);

    // Update your application state here
    this->display = al_create_display(800, 300);
    this->default_font = al_create_builtin_font();
    this->event_queue = al_create_event_queue();
    this->graphics_timer = al_create_timer(1.0 / 60.0);
    this->discord_callback_timer = al_create_timer(1.0);

    al_register_event_source(this->event_queue, al_get_display_event_source(this->display));
    al_register_event_source(this->event_queue, al_get_timer_event_source(this->graphics_timer));
    al_register_event_source(this->event_queue, al_get_timer_event_source(this->discord_callback_timer));
    al_register_event_source(this->event_queue, al_get_keyboard_event_source());
    
    al_start_timer(this->graphics_timer);
    al_start_timer(this->discord_callback_timer);

    al_set_window_title(this->display, "∫");

    // init the music engine
    if (!this->music_engine.initialize()) {
        return false; // Failed to initialize music engine
    }

    // init the database
    if (!this->db.open()) {
        return false; // Failed to open database
    }

    // init the library
    if (!this->library.loadFromDatabase(this->db)) {
        return false; // Failed to load library from database
    }

    for (const auto& [id, album] : this->library.getAllAlbums()) {
        std::cout << "Loaded album: " << album.title << " (ID: " << album.id << ")\n";
    }

    // load fonts
    this->fontManager.loadFont("courier", "C:\\Windows\\Fonts\\cour.ttf");

    initialized = true;
    return true;
}

void AppState::shutdown() {
    // Render your application state here

}
};