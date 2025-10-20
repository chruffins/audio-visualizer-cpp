#pragma once

#include <string>
#include <memory>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include "graphics/models/progress_bar.hpp"

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

    double getCurrentTime() const;
    double getDuration() const;

    void setGain(float gain);
    void setPan(float pan);
    void setSpeed(float speed);
    bool isPlaying() const;

    std::shared_ptr<ui::ProgressBar> progressBarModel = std::make_shared<ui::ProgressBar>();

    void update(); // Call periodically to update progress among other things
private:
    ALLEGRO_VOICE* voice = nullptr;
    ALLEGRO_MIXER* mixer = nullptr;
    ALLEGRO_AUDIO_STREAM* current_stream = nullptr;

    double current_time = 0.0;
    double duration = 0.0;
};
};