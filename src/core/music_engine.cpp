#include "core/music_engine.hpp"

namespace core {
MusicEngine::MusicEngine() {
    // Constructor
    voice = nullptr;
    mixer = nullptr;
    current_stream = nullptr;
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
    if (voice) {
        al_destroy_voice(voice);
        voice = nullptr;
    }
    if (mixer) {
        al_destroy_mixer(mixer);
        mixer = nullptr;
    }
    if (current_stream) {
        al_destroy_audio_stream(current_stream);
        current_stream = nullptr;
    }
}

void MusicEngine::playSound(const std::string& file_path) {
    if (current_stream) {
        al_destroy_audio_stream(current_stream);
        current_stream = nullptr;
    }
    current_stream = al_load_audio_stream(file_path.c_str(), 4, 4096);

    if (current_stream) {
        al_attach_audio_stream_to_mixer(current_stream, mixer);
        al_set_audio_stream_playing(current_stream, true);
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
};