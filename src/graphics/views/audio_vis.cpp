#include "graphics/views/audio_vis.hpp"

#include <algorithm>
#include <cmath>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "core/music_engine.hpp"
#include "util/config.hpp"
#include "vis/shader.hpp"

namespace ui {

namespace {

struct AudioMetrics {
    float rms = 0.0f;
    float peak = 0.0f;
    float transient = 0.0f;
};

AudioMetrics computeAudioMetrics(const std::vector<float>& samples) {
    AudioMetrics metrics;
    if (samples.empty()) {
        return metrics;
    }

    float sumSquares = 0.0f;
    float sumAbs = 0.0f;
    float peak = 0.0f;

    for (float sample : samples) {
        const float absSample = std::abs(sample);
        sumSquares += sample * sample;
        sumAbs += absSample;
        peak = std::max(peak, absSample);
    }

    const float count = static_cast<float>(samples.size());
    metrics.rms = std::sqrt(sumSquares / count);
    metrics.peak = peak;
    metrics.transient = std::max(0.0f, peak - (sumAbs / count));
    return metrics;
}

} // namespace

void AudioVisualizerView::BitmapDeleter::operator()(ALLEGRO_BITMAP* bmp) const {
    if (bmp) {
        al_destroy_bitmap(bmp);
    }
}

AudioVisualizerView::AudioVisualizerView(core::MusicEngine* musicEngine)
    : musicEngine(musicEngine) {
        const std::string vertexSource = util::Config::resolveAssetPath("shaders/vertex.glsl");
        const std::string pixelSource = util::Config::resolveAssetPath("shaders/pixel.glsl");
        shader = std::make_unique<vis::Shader>(vertexSource.c_str(), pixelSource.c_str(), true);
        // shader = std::make_unique<vis::Shader>(); // use default shader for now
    }

AudioVisualizerView::~AudioVisualizerView() = default;

void AudioVisualizerView::setMusicEngine(core::MusicEngine* engine) {
    musicEngine = engine;
}

void AudioVisualizerView::setBounds(const graphics::UV& newPosition, const graphics::UV& newSize) {
    position = newPosition;
    size = newSize;
}

void AudioVisualizerView::setVisible(bool visible) {
    isVisible = visible;
}

void AudioVisualizerView::setBitmap(ALLEGRO_BITMAP* newBitmap) {
    bitmap.reset(newBitmap);
}

ALLEGRO_BITMAP* AudioVisualizerView::getBitmap() const {
    return bitmap.get();
}

void AudioVisualizerView::setShader(vis::Shader* newShader) {
    shader.reset(newShader);
}

vis::Shader* AudioVisualizerView::getShader() const {
    return shader.get();
}

void AudioVisualizerView::draw(const graphics::RenderContext& context) {
    if (!isVisible) {
        return;
    }

    auto [x, y] = position.toScreenPos(
        static_cast<float>(context.screenWidth),
        static_cast<float>(context.screenHeight)
    );
    auto [w, h] = size.toScreenPos(
        static_cast<float>(context.screenWidth),
        static_cast<float>(context.screenHeight)
    );

    x += context.offsetX;
    y += context.offsetY;

    if (w <= 0.0f || h <= 0.0f) {
        return;
    }

    if (bitmap) {
        const float srcW = static_cast<float>(al_get_bitmap_width(bitmap.get()));
        const float srcH = static_cast<float>(al_get_bitmap_height(bitmap.get()));
        if (srcW > 0.0f && srcH > 0.0f) {
            if (shader && shader->isLoaded()) {
                shader->use();
                shader->setFloat("time", static_cast<float>(al_get_time()));

                if (musicEngine) {
                    const auto samples = musicEngine->copyRecentMonoSamples(1024);
                    const AudioMetrics metrics = computeAudioMetrics(samples);
                    shader->setFloat("audio_rms", metrics.rms);
                    shader->setFloat("audio_peak", metrics.peak);
                    shader->setFloat("audio_transient", metrics.transient);
                } else {
                    shader->setFloat("audio_rms", 0.0f);
                    shader->setFloat("audio_peak", 0.0f);
                    shader->setFloat("audio_transient", 0.0f);
                }
            }

            al_draw_scaled_bitmap(bitmap.get(), 0.0f, 0.0f, srcW, srcH, x, y, w, h, 0);

            if (shader) {
                al_use_shader(nullptr);
            }
        }
    } else {
        al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgb(12, 12, 20));
    }

    if (musicEngine) {
        const auto samples = musicEngine->copyRecentMonoSamples(2048);
        if (samples.size() >= 2) {
            const float midY = y + (h * 0.5f);
            const float amplitude = h * 0.65f;
            const float widthDenom = static_cast<float>(samples.size() - 1);

            for (size_t i = 1; i < samples.size(); ++i) {
                const float x1 = x + (static_cast<float>(i - 1) / widthDenom) * w;
                const float x2 = x + (static_cast<float>(i) / widthDenom) * w;
                const float y1 = midY - std::clamp(samples[i - 1], -1.0f, 1.0f) * amplitude;
                const float y2 = midY - std::clamp(samples[i], -1.0f, 1.0f) * amplitude;
                al_draw_line(x1, y1, x2, y2, al_map_rgba(255, 255, 255, 220), 1.0f);
            }
        }
    }

}

} // namespace ui
