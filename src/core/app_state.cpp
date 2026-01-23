#define MINIMP3_IMPLEMENTATION

#include <core/app_state.hpp>
#include "mp3/mp3_streaming.hpp"
#include "music/album.hpp"

namespace core {
bool AppState::init() {
    static bool initialized = false;
    if (initialized) return true; // Already initialized

    mp3streaming::addMP3Support();

    // Load configuration first
    if (!this->config.load("config.ini")) {
        std::cerr << "Warning: Could not load config.ini, using defaults.\n";
    }

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
    if (!this->library->loadFromDatabase(this->db)) {
        return false; // Failed to load library from database
    }

    /*
    for (const auto& [id, album] : this->library.getAllAlbums()) {
        std::cout << "Loaded album: " << album.title << " (ID: " << album.id << ")\n";
    }
    */

    // Fill the application-owned play queue with all songs from the library
    // (enqueueing by song id). This is a simple example of populating the
    // queue; a real app would probably use playlists or user selection.
    std::vector<int> allSongIds;
    for (const auto& [sid, song] : this->library->getAllSongs()) {
        allSongIds.push_back(sid);
        // std::cout << "Loaded song: " << song.title << " (ID: " << song.id << ")\n";
    }
    // preserve insertion order from the map iteration
    this->play_queue->enqueue_many(allSongIds);
    std::cout << "Loaded " << allSongIds.size() << " songs into the play queue.\n";

    // load fonts
    this->fontManager->loadFont("courier", "../assets/CourierPrime-Regular.ttf");

    // Wire the shared play queue into the music engine so playback can operate
    // on the application-owned queue.
    this->music_engine.setPlayQueue(this->play_queue);

    // Optionally start playback of the first song in the queue (if any)
    int first = this->play_queue->current();
    if (first >= 0) {
        const music::Song* s = this->library->getSongById(first);
        if (s) {
            this->music_engine.playSound(s->filename);
        }
    }

    initialized = true;
    return true;
}

void AppState::shutdown() {
    music_engine.shutdown();
    
    // Clear library (destroys album cover ImageModels) before destroying display
    // This is critical because Allegro bitmaps must be destroyed before the display
    if (library) {
        library.reset();
    }
    if (play_queue) {
        play_queue.reset();
    }
    
    if (fontManager) {
        fontManager->cleanup();
    }
    
    // Destroy Allegro resources in reverse order of creation
    // Stop timers first
    if (discord_callback_timer) {
        al_stop_timer(discord_callback_timer);
        al_destroy_timer(discord_callback_timer);
        discord_callback_timer = nullptr;
    }
    if (graphics_timer) {
        al_stop_timer(graphics_timer);
        al_destroy_timer(graphics_timer);
        graphics_timer = nullptr;
    }
    
    // Destroy event queue
    if (event_queue) {
        al_destroy_event_queue(event_queue);
        event_queue = nullptr;
    }
    
    // Destroy fonts
    if (default_font) {
        al_destroy_font(default_font);
        default_font = nullptr;
    }
    
    // Destroy display last
    if (display) {
        al_destroy_display(display);
        display = nullptr;
    }
}
};