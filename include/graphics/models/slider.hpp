#pragma once

#include <algorithm>

namespace ui {

class Slider {
public:
    Slider() = default;
    Slider(float minValue, float maxValue, float initialValue)
        : minValue(minValue), maxValue(maxValue), value(initialValue) {
        if (this->maxValue <= this->minValue) {
            this->maxValue = this->minValue + 1.0f;
        }
        this->value = std::clamp(this->value, this->minValue, this->maxValue);
    }

    void setRange(float min, float max) {
        minValue = min;
        maxValue = max;
        if (maxValue <= minValue) {
            maxValue = minValue + 1.0f;
        }
        value = std::clamp(value, minValue, maxValue);
    }

    float getMin() const { return minValue; }
    float getMax() const { return maxValue; }

    void setValue(float newValue) {
        value = std::clamp(newValue, minValue, maxValue);
    }

    float getValue() const { return value; }

    float getNormalizedValue() const {
        const float range = maxValue - minValue;
        if (range <= 0.0f) return 0.0f;
        return (value - minValue) / range;
    }

    void setNormalizedValue(float normalized) {
        const float t = std::clamp(normalized, 0.0f, 1.0f);
        value = minValue + (maxValue - minValue) * t;
    }

private:
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float value = 0.0f;
};

} // namespace ui
