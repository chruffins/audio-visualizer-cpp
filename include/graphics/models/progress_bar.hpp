#pragma once

#include <utility>

#include "graphics/uv.hpp"

namespace ui {
class ProgressBar {
public:
    ProgressBar() = default;
    ~ProgressBar() = default;

    void setProgress(float progress) {
        if (progress < 0.0f) progress = 0.0f;
        this->progress = progress;
    }

    // Get current progress
    float getProgress() const {
        return progress;
    }

    // Get progress in comparison to finishesAt
    float getProgressRelative() const {
        return progress / finishesAt;
    }

    // get finishesAt value
    float getFinishesAt() const {
        return finishesAt;
    }

    // Set finishesAt value
    void setFinishesAt(float value) {
        if (value <= 0.0f) value = 1.0f;
        finishesAt = value;
    }
private:
    // progress is a non-negative value
    float progress = 0.0f;
    // finishesAt is a positive value
    float finishesAt = 1.0f;
};
}; // namespace ui