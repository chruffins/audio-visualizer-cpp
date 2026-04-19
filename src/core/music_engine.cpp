#include "core/music_engine.hpp"
#include <allegro5/allegro.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_audio.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include "core/app_state.hpp"

namespace core {
namespace {
constexpr size_t kDefaultSampleBufferCapacity = 44100 * 2 * 4; // ~4s stereo at 44.1kHz
}

void MusicEngine::SampleCaptureState::setEnabled(bool value) {
    std::lock_guard<std::mutex> lock(ring_mutex);
    enabled = value;
}

bool MusicEngine::SampleCaptureState::isEnabled() const {
    std::lock_guard<std::mutex> lock(ring_mutex);
    return enabled;
}

void MusicEngine::SampleCaptureState::setCapacity(size_t sample_count) {
    std::lock_guard<std::mutex> lock(ring_mutex);
    ring_capacity = sample_count;
    ring_buffer.assign(ring_capacity, 0.0f);
    ring_write_pos = 0;
    ring_size = 0;
}

size_t MusicEngine::SampleCaptureState::getCapacity() const {
    std::lock_guard<std::mutex> lock(ring_mutex);
    return ring_capacity;
}

size_t MusicEngine::SampleCaptureState::getSize() const {
    std::lock_guard<std::mutex> lock(ring_mutex);
    return ring_size;
}

void MusicEngine::SampleCaptureState::clear() {
    std::lock_guard<std::mutex> lock(ring_mutex);
    ring_write_pos = 0;
    ring_size = 0;
}

std::vector<float> MusicEngine::SampleCaptureState::copyRecent(size_t max_samples) const {
    std::lock_guard<std::mutex> lock(ring_mutex);
    if (ring_size == 0 || max_samples == 0 || ring_capacity == 0) {
        return {};
    }

    const size_t sample_count = std::min(max_samples, ring_size);
    std::vector<float> out(sample_count);
    const size_t start = (ring_write_pos + ring_capacity - sample_count) % ring_capacity;

    for (size_t i = 0; i < sample_count; ++i) {
        out[i] = ring_buffer[(start + i) % ring_capacity];
    }

    return out;
}

std::vector<float> MusicEngine::SampleCaptureState::copyRecentMono(size_t max_frames) const {
    std::lock_guard<std::mutex> lock(ring_mutex);
    if (ring_size == 0 || max_frames == 0 || ring_capacity == 0) {
        return {};
    }

    const size_t channel_count = std::max<size_t>(1, channels);
    const size_t available_frames = ring_size / channel_count;
    const size_t frame_count = std::min(max_frames, available_frames);
    if (frame_count == 0) {
        return {};
    }

    std::vector<float> out(frame_count);
    const size_t sample_count = frame_count * channel_count;
    const size_t start = (ring_write_pos + ring_capacity - sample_count) % ring_capacity;

    for (size_t frame = 0; frame < frame_count; ++frame) {
        float sum = 0.0f;
        const size_t frame_start = (start + frame * channel_count) % ring_capacity;

        for (size_t channel = 0; channel < channel_count; ++channel) {
            sum += ring_buffer[(frame_start + channel) % ring_capacity];
        }

        out[frame] = sum / static_cast<float>(channel_count);
    }

    return out;
}

void MusicEngine::SampleCaptureState::appendInterleavedInt16(const void* buf, unsigned int frames) {
    std::lock_guard<std::mutex> lock(ring_mutex);
    if (!enabled || ring_capacity == 0 || !buf || frames == 0) {
        return;
    }

    const auto* interleaved = static_cast<const int16_t*>(buf);
    const size_t total_samples = static_cast<size_t>(frames) * channels;

    for (size_t i = 0; i < total_samples; ++i) {
        ring_buffer[ring_write_pos] = static_cast<float>(interleaved[i]) / 32768.0f;
        ring_write_pos = (ring_write_pos + 1) % ring_capacity;
        if (ring_size < ring_capacity) {
            ++ring_size;
        }
    }
}

MusicEngine::MusicEngine() {
    // Constructor
    voice = nullptr;
    mixer = nullptr;
    current_stream = nullptr;
    is_shutdown = false;
    song_finished_fired = false;
    sample_capture.channels = 2;
    sample_capture.setCapacity(kDefaultSampleBufferCapacity);
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

    sample_capture.channels = std::max<size_t>(
        1,
        al_get_channel_count(al_get_mixer_channels(mixer))
    );

    if (!al_set_mixer_postprocess_callback(mixer, &MusicEngine::mixerPostprocessCallback, this)) {
        std::cerr << "Failed to register mixer postprocess callback\n";
        al_destroy_mixer(mixer);
        al_destroy_voice(voice);
        mixer = nullptr;
        voice = nullptr;
        return false;
    }

    al_set_mixer_gain(mixer, current_gain);

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
        al_set_mixer_postprocess_callback(mixer, nullptr, nullptr);
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
        
        // If not playing from a queue context (album/playlist), mark as individual song
        if (playQueueModel && playQueueModel->context_type != music::PlaybackContextType::Album &&
            playQueueModel->context_type != music::PlaybackContextType::Playlist) {
            playQueueModel->context_type = music::PlaybackContextType::Individual;
            playQueueModel->context_id = -1;
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
    current_gain = std::clamp(gain, 0.0f, 1.0f);
    if (mixer) {
        al_set_mixer_gain(mixer, current_gain);
    }
}

float MusicEngine::getGain() const {
    return current_gain;
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

void MusicEngine::setSampleCaptureEnabled(bool enabled) {
    sample_capture.setEnabled(enabled);
}

bool MusicEngine::isSampleCaptureEnabled() const {
    return sample_capture.isEnabled();
}

void MusicEngine::setSampleBufferCapacity(size_t sample_count) {
    sample_capture.setCapacity(sample_count);
}

size_t MusicEngine::getSampleBufferCapacity() const {
    return sample_capture.getCapacity();
}

size_t MusicEngine::getBufferedSampleCount() const {
    return sample_capture.getSize();
}

void MusicEngine::clearSampleBuffer() {
    sample_capture.clear();
}

std::vector<float> MusicEngine::copyRecentSamples(size_t max_samples) const {
    return sample_capture.copyRecent(max_samples);
}

std::vector<float> MusicEngine::copyRecentMonoSamples(size_t max_frames) const {
    return sample_capture.copyRecentMono(max_frames);
}

void MusicEngine::mixerPostprocessCallback(void* buf, unsigned int samples, void* data) {
    if (!data || !buf || samples == 0) {
        return;
    }

    auto* engine = static_cast<MusicEngine*>(data);
    engine->sample_capture.appendInterleavedInt16(buf, samples);
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
        // Set context to Album with the album ID
        playQueueModel->context_type = music::PlaybackContextType::Album;
        playQueueModel->context_id = album_id;
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

void MusicEngine::playPlaylist(int playlist_id) {
    if (!library) {
        std::cerr << "MusicEngine::playPlaylist: library not set\n";
        return;
    }

    // TODO: Fetch playlist songs from database when playlist support is fully implemented
    // For now, this is a stub that sets up the context
    if (playQueueModel) {
        playQueueModel->clear();
        playQueueModel->context_type = music::PlaybackContextType::Playlist;
        playQueueModel->context_id = playlist_id;
    }
}

music::PlaybackContextType MusicEngine::getCurrentContextType() const {
    if (!playQueueModel) {
        return music::PlaybackContextType::Individual;
    }
    return playQueueModel->context_type;
}

int MusicEngine::getCurrentContextId() const {
    if (!playQueueModel) {
        return -1;
    }
    return playQueueModel->context_id;
}

void MusicEngine::setRepeat(bool enabled) {
    if (playQueueModel) {
        playQueueModel->repeat(enabled);
    }
}

bool MusicEngine::isRepeating() const {
    if (!playQueueModel) {
        return false;
    }
    return playQueueModel->is_repeating;
}

};