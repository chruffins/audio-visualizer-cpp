#pragma once

#include <string>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

namespace core {
class MusicEngine {
public:
    MusicEngine();
    ~MusicEngine();

    bool initialize();
    void shutdown();

    void playSound(const std::string& file_path);
    void pauseSound();
    void resumeSound();
    void stopSound();

    void setGain(float gain);
    void setPan(float pan);
    void setSpeed(float speed);
    bool isPlaying() const;
private:
    ALLEGRO_VOICE* voice = nullptr;
    ALLEGRO_MIXER* mixer = nullptr;
    ALLEGRO_AUDIO_STREAM* current_stream = nullptr;
};
};