#pragma once

#include <deque>
#include <random>
#include <vector>

namespace music {
enum class PlaybackContextType {
    Individual,  // Playing a single song directly
    Album,       // Playing from an album
    Playlist     // Playing from a playlist
};

struct PlayQueue {
    PlayQueue() : current_index(0), is_shuffled(false), is_repeating(false), 
                  context_type(PlaybackContextType::Individual), context_id(-1),
                  song_ids(), played_indices() {}

    // enqueue a single song id
    void enqueue(int song_id) {
        song_ids.push_back(song_id);
    }

    // enqueue multiple song ids
    void enqueue_many(const std::vector<int>& ids) {
        for (int id : ids) song_ids.push_back(id);
    }

    // get current song id (does not advance). Returns -1 if empty.
    int current() const {
        if (song_ids.empty() || current_index >= song_ids.size()) return -1;
        return song_ids[current_index];
    }

    // advance to next song and return its id. Returns -1 if there is no next song.
    int next() {
        if (song_ids.empty()) return -1;
        
        // record current index in play history before advancing
        played_indices.push_back(current_index);
        
        if (is_shuffled) {
            // pick a random index
            std::uniform_int_distribution<size_t> dist(0, song_ids.size() - 1);
            static thread_local std::mt19937 rng(std::random_device{}());
            current_index = dist(rng);
            return song_ids[current_index];
        }
        if (current_index + 1 < song_ids.size()) {
            ++current_index;
            return song_ids[current_index];
        }
        if (is_repeating) {
            current_index = 0;
            return song_ids[current_index];
        }
        return -1;
    }

    // move to previous song and return its id. Returns -1 if none.
    // uses play history to return to previously played songs.
    int previous() {
        if (song_ids.empty()) return -1;
        
        // if we have play history, go back to the last played index
        if (!played_indices.empty()) {
            current_index = played_indices.back();
            played_indices.pop_back();
            return song_ids[current_index];
        }
        
        // fallback: no play history, try to move back in queue
        if (current_index == 0) {
            if (is_repeating) {
                current_index = song_ids.size() - 1;
                return song_ids[current_index];
            }
            return -1;
        }
        --current_index;
        return song_ids[current_index];
    }

    void clear() {
        song_ids.clear();
        played_indices.clear();
        current_index = 0;
        context_type = PlaybackContextType::Individual;
        context_id = -1;
    }

    size_t size() const {
        return song_ids.size();
    }

    void shuffle(bool on) {
        is_shuffled = on;
    }

    void repeat(bool on) {
        is_repeating = on;
    }

    std::deque<int> song_ids;
    std::deque<size_t> played_indices;  // track indices that were actually played for reverse navigation
    size_t current_index;
    bool is_shuffled;
    bool is_repeating;
    PlaybackContextType context_type;   // Track what kind of playback context we're in
    int context_id;                      // Track the album/playlist ID (or -1 for individual songs)
};
} // namespace music
 