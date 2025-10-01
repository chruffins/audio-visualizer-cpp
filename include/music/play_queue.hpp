#pragma once
#include <string>
#include <queue>

namespace music {
struct PlayQueue {
    std::deque<int> song_ids;
    size_t current_index;
    bool is_shuffled;
    bool is_repeating;
};
} // namespace music