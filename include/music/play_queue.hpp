#pragma once
#include <string>
#include <queue>
#include <deque>
#include <vector>
#include <mutex>
#include <algorithm>
#include <random>

namespace music {
struct PlayQueue {
    PlayQueue() : current_index(0), is_shuffled(false), is_repeating(false), song_ids() {}

    // Enqueue a single song id
    void enqueue(int song_id) {
        song_ids.push_back(song_id);
    }

    // Enqueue multiple song ids
    void enqueue_many(const std::vector<int>& ids) {
        for (int id : ids) song_ids.push_back(id);
    }

    // Get current song id (does not advance). Returns -1 if empty.
    int current() const {
        if (song_ids.empty() || current_index >= song_ids.size()) return -1;
        return song_ids[current_index];
    }

    // Advance to next song and return its id. Returns -1 if there is no next song.
    int next() {
        if (song_ids.empty()) return -1;
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

    // Move to previous song and return its id. Returns -1 if none.
    int previous() {
        if (song_ids.empty()) return -1;
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
        current_index = 0;
    }

    size_t size() const {
        return song_ids.size();
    }

    void shuffle(bool on) {
        is_shuffled = on;
    }

    void repeat(bool on) {
        is_repeating = on;
        is_repeating = on;
    }

    std::deque<int> song_ids;
    size_t current_index;
    bool is_shuffled;
    bool is_repeating;
};
} // namespace music
 