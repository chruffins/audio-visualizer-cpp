#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "graphics/models/progress_bar.hpp"
#include "music/play_queue.hpp"

struct ALLEGRO_AUDIO_STREAM;
struct ALLEGRO_MIXER;
struct ALLEGRO_VOICE;
struct ALLEGRO_EVENT_QUEUE;

// forward-declare Song
namespace music { 
    struct SongView;
    class Library;
    enum class PlaybackContextType;
}

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
    float getGain() const;
    void setPan(float pan);
    void setSpeed(float speed);
    void setProgress(double position); // position in seconds
    
    // Enable or disable audio sample capture from the active mixer output.
    void setSampleCaptureEnabled(bool enabled);
    bool isSampleCaptureEnabled() const;

    // Capacity is expressed in individual PCM samples (interleaved channels).
    void setSampleBufferCapacity(size_t sample_count);
    size_t getSampleBufferCapacity() const;
    size_t getBufferedSampleCount() const;
    void clearSampleBuffer();

    // Returns up to max_samples from newest captured audio data.
    // Samples are normalized to [-1.0, 1.0].
    std::vector<float> copyRecentSamples(size_t max_samples) const;

    // Returns up to max_frames downmixed to mono from the newest captured audio data.
    // This is better for waveform visualization when the source is interleaved stereo.
    std::vector<float> copyRecentMonoSamples(size_t max_frames) const;

    void setPlayQueue(std::shared_ptr<music::PlayQueue> playQueue) {
        playQueueModel = playQueue;
    }

    void setLibrary(music::Library* lib) {
        library = lib;
    }

    // Play all songs in the specified album
    void playAlbum(int album_id);

    // Play all songs in the specified playlist
    void playPlaylist(int playlist_id);

    // Get current playback context type
    // Returns: Individual (single song), Album (all album songs), or Playlist (playlist songs)
    music::PlaybackContextType getCurrentContextType() const;
    
    // Get current context ID (album_id or playlist_id). Returns -1 for Individual playback.
    int getCurrentContextId() const;

    // Set repeat mode on/off for the play queue
    void setRepeat(bool enabled);
    
    // Check if repeat mode is enabled
    bool isRepeating() const;

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
    music::Library* library = nullptr;

    void update(); // Call periodically to update progress among other things
private:
    struct SampleCaptureState {
        bool enabled = true;
        size_t channels = 2;
        std::vector<float> ring_buffer;
        size_t ring_capacity = 0;
        size_t ring_write_pos = 0;
        size_t ring_size = 0;
        mutable std::mutex ring_mutex;

        void setEnabled(bool value);
        bool isEnabled() const;
        void setCapacity(size_t sample_count);
        size_t getCapacity() const;
        size_t getSize() const;
        void clear();
        std::vector<float> copyRecent(size_t max_samples) const;
        std::vector<float> copyRecentMono(size_t max_frames) const;
        void appendInterleavedInt16(const void* buf, unsigned int frames);
    };

    ALLEGRO_VOICE* voice = nullptr;
    ALLEGRO_MIXER* mixer = nullptr;
    ALLEGRO_AUDIO_STREAM* current_stream = nullptr;
    ALLEGRO_EVENT_QUEUE* event_queue = nullptr;

    double current_time = 0.0;
    double duration = 0.0;
    float current_gain = 1.0f;
    bool is_shutdown = false;
    bool song_finished_fired = false; // Track if we already fired the callback

    SampleCaptureState sample_capture;

    static void mixerPostprocessCallback(void* buf, unsigned int samples, void* data);
};
};