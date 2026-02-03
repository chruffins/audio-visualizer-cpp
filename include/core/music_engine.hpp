#pragma once

#include <functional>
#include <memory>
#include <string>

#include "graphics/models/progress_bar.hpp"
#include "music/play_queue.hpp"

struct ALLEGRO_AUDIO_STREAM;
struct ALLEGRO_MIXER;
struct ALLEGRO_VOICE;
struct ALLEGRO_EVENT_QUEUE;

// forward-declare Song
namespace music { struct SongView; }

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
    void setProgress(double position); // position in seconds
    void setPlayQueue(std::shared_ptr<music::PlayQueue> playQueue) {
        playQueueModel = playQueue;
    }

    void setEventQueue(ALLEGRO_EVENT_QUEUE* queue) {
        event_queue = queue;
    }

    // Callback invoked when a new song begins playback. User can assign a
    // handler to update UI (NowPlayingView) or other systems.
    std::function<void(const music::SongView&)> onSongChanged;

    // Callback invoked when the current song finishes playing.
    std::function<void()> onSongFinished;

    // Advance to the next song in the injected play queue and start playback.
    // If there is no next song, this is a no-op.
    void playNext();

    // Play the previous song from the injected play queue.
    // If there is no previous song, this is a no-op.
    void playPrevious();

    bool isPlaying() const;

    std::shared_ptr<ui::ProgressBar> progressBarModel = std::make_shared<ui::ProgressBar>();
    std::shared_ptr<music::PlayQueue> playQueueModel = nullptr; // starts null

    void update(); // Call periodically to update progress among other things
private:
    ALLEGRO_VOICE* voice = nullptr;
    ALLEGRO_MIXER* mixer = nullptr;
    ALLEGRO_AUDIO_STREAM* current_stream = nullptr;
    ALLEGRO_EVENT_QUEUE* event_queue = nullptr;

    double current_time = 0.0;
    double duration = 0.0;
    bool is_shutdown = false;
    bool song_finished_fired = false; // Track if we already fired the callback
};
};