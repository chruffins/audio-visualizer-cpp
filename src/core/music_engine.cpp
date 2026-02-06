#include "core/music_engine.hpp"
#include <allegro5/allegro.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_audio.h>
#include <algorithm>
#include <iostream>
#include "core/app_state.hpp"

namespace core {
MusicEngine::MusicEngine() {
    // Constructor
    voice = nullptr;
    mixer = nullptr;
    current_stream = nullptr;
    is_shutdown = false;
    song_finished_fired = false;
}

MusicEngine::~MusicEngine() {
    // Destructor
    shutdown();
}

bool MusicEngine::initialize() {
    // Initialization code
    voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);
    if (!voice) {
        return false;
    }

    mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);
    if (!mixer) {
        al_destroy_voice(voice);
        return false;
    }

    if (!al_attach_mixer_to_voice(mixer, voice)) {
        al_destroy_mixer(mixer);
        al_destroy_voice(voice);
        return false;
    }

    return true;
}

void MusicEngine::shutdown() {
    if (is_shutdown) {
        return; // Already shut down
    }
    
    // Destroy in reverse order of creation: stream -> mixer -> voice
    if (current_stream) {
        al_destroy_audio_stream(current_stream);
        current_stream = nullptr;
    }
    if (mixer) {
        al_destroy_mixer(mixer);
        mixer = nullptr;
    }
    if (voice) {
        al_destroy_voice(voice);
        voice = nullptr;
    }
    
    is_shutdown = true;
}

void MusicEngine::playSound(const std::string& file_path) {
    if (current_stream) {
        al_destroy_audio_stream(current_stream);
        current_stream = nullptr;
    }
    current_stream = al_load_audio_stream(file_path.c_str(), 4, 4096);

    if (current_stream) {
        current_time = 0.0;
        duration = al_get_audio_stream_length_secs(current_stream);
        progressBarModel->setFinishesAt(duration);
        song_finished_fired = false; // Reset the flag for the new song
        std::cout << "Loaded audio stream. Duration: " << duration << " seconds.\n";
        auto attachResult = al_attach_audio_stream_to_mixer(current_stream, mixer);
        auto playResult = al_set_audio_stream_playing(current_stream, true);

        if (!attachResult || !playResult) {
            std::cerr << "Failed to play audio stream: attachResult=" << attachResult
                      << ", playResult=" << playResult << "\n";
        }

        if (event_queue) {
            al_register_event_source(event_queue, al_get_audio_stream_event_source(current_stream));
        }

        al_get_audio_stream_event_source(current_stream); // ensure event source is valid
        // If possible, find Song model in the global library and notify
        // listeners about the change.
        // (This is best-effort: filenames may be relative or differently
        // normalized; adjust lookup as needed.)
        const music::SongView* song = nullptr;
        for (const auto& s : AppState::instance().library->getSongViews()) {
            if (s.filename == file_path) {
                song = &s;
                break;
            }
        }
        if (song && onSongChanged) {
            onSongChanged(*song);
        }
    }
}

void MusicEngine::pauseSound() {
    if (current_stream) {
        al_set_audio_stream_playing(current_stream, false);
    }
}

void MusicEngine::resumeSound() {
    if (current_stream) {
        al_set_audio_stream_playing(current_stream, true);
    }
}

void MusicEngine::stopSound() {
    // Stop sound code
    if (current_stream) {
        al_set_audio_stream_playing(current_stream, false);
        current_stream = nullptr;
    }
}

double MusicEngine::getCurrentTime() const {
    return current_time;
}

double MusicEngine::getDuration() const {
    return duration;
}

void MusicEngine::setGain(float gain) {
    // Set gain code
    if (mixer) {
        al_set_mixer_gain(mixer, gain);
    }
}

void MusicEngine::setPan(float pan) {
    // Set pan code
    if (current_stream) al_set_audio_stream_pan(current_stream, pan);
}

void MusicEngine::setSpeed(float speed) {
    // Set speed code
    if (current_stream) al_set_audio_stream_speed(current_stream, speed);
}

bool MusicEngine::isPlaying() const {
    // Check if playing code
    return current_stream && al_get_audio_stream_playing(current_stream);
}

void MusicEngine::update() {
    if (current_stream) {
        current_time = al_get_audio_stream_position_secs(current_stream);
        progressBarModel->setProgress(current_time);
    } 
    /*
    else {
        current_time = 0.0;
        duration = 0.0;
        progressBarModel->setProgress(0.0f);
        progressBarModel->setFinishesAt(1.0f);
    }
    */
}

void MusicEngine::setProgress(double position) {
    if (current_stream) {
        al_seek_audio_stream_secs(current_stream, position);
        current_time = position;
        progressBarModel->setProgress(current_time);
    }
}

void MusicEngine::playNext() {
    if (!playQueueModel) return;
    int nextId = playQueueModel->next();
    if (nextId < 0) {
        // no next song
        return;
    }

    // Look up song path from the global library (AppState owns the library)
    const music::SongView* song = AppState::instance().library->getSongById(nextId);
    if (!song) {
        std::cerr << "MusicEngine::playNext: song id " << nextId << " not found in library\n";
        return;
    }

    playSound(song->filename);
}

void MusicEngine::playPrevious() {
    if (!playQueueModel) return;
    int prevId = playQueueModel->previous();
    if (prevId < 0) {
        // no previous song
        return;
    }

    const music::SongView* song = AppState::instance().library->getSongById(prevId);
    if (!song) {
        std::cerr << "MusicEngine::playPrevious: song id " << prevId << " not found in library\n";
        return;
    }

    playSound(song->filename);
}

void MusicEngine::playAlbum(int album_id) {
    if (!library) {
        std::cerr << "MusicEngine::playAlbum: library not set\n";
        return;
    }

    // Collect all songs for this album
    std::vector<const music::Song*> albumSongs;
    for (const auto& [id, song] : library->getAllSongs()) {
        if (song.album_id == album_id) {
            albumSongs.push_back(&song);
        }
    }

    if (albumSongs.empty()) {
        std::cerr << "MusicEngine::playAlbum: no songs found for album " << album_id << "\n";
        return;
    }

    // Sort by track number
    std::sort(albumSongs.begin(), albumSongs.end(),
        [](const music::Song* a, const music::Song* b) {
            return a->track_number < b->track_number;
        });

    // Clear current play queue and enqueue album songs
    if (playQueueModel) {
        playQueueModel->clear();
        for (const auto& song : albumSongs) {
            playQueueModel->enqueue(song->id);
        }
    }
    
    // Play the first song directly (don't use playNext which advances first)
    const music::SongView* firstSong = library->getSongById(albumSongs[0]->id);
    if (firstSong) {
        playSound(firstSong->filename);
    }
}

};