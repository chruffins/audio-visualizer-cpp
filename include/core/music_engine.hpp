#pragma once

#include <string>
#include <memory>
#include <functional>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include "graphics/models/progress_bar.hpp"
#include "music/play_queue.hpp"

// forward-declare Song
namespace music { struct Song; }

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
    void setPlayQueue(std::shared_ptr<music::PlayQueue> playQueue) {
        playQueueModel = playQueue;
    }

    // Callback invoked when a new song begins playback. User can assign a
    // handler to update UI (NowPlayingView) or other systems.
    std::function<void(const music::Song&)> onSongChanged;

    // Advance to the next song in the injected play queue and start playback.
    // If there is no next song, this is a no-op.
    void playNext();

    bool isPlaying() const;

    std::shared_ptr<ui::ProgressBar> progressBarModel = std::make_shared<ui::ProgressBar>();
    std::shared_ptr<music::PlayQueue> playQueueModel = nullptr; // starts null

    void update(); // Call periodically to update progress among other things
private:
    ALLEGRO_VOICE* voice = nullptr;
    ALLEGRO_MIXER* mixer = nullptr;
    ALLEGRO_AUDIO_STREAM* current_stream = nullptr;

    double current_time = 0.0;
    double duration = 0.0;
    bool is_shutdown = false;
};
};