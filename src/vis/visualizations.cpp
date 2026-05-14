#include "vis/visualizations.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "vis/shader.hpp"
#include "util/config.hpp"

namespace {

struct AudioMetrics {
    float rms = 0.0f;
    float peak = 0.0f;
    float transient = 0.0f;
};

constexpr std::size_t kPolarWaveformSampleWindow = 1024;
constexpr std::size_t kMirrorBarsSampleWindow = 768;
constexpr std::size_t kDualEchoStereoSampleWindow = 1024;
constexpr std::size_t kDualEchoMonoFallbackWindow = 512;

AudioMetrics computeAudioMetrics(const float* samples, std::size_t sampleCount) {
    AudioMetrics metrics;
    if (!samples || sampleCount == 0) {
        return metrics;
    }

    float sumSquares = 0.0f;
    float sumAbs = 0.0f;
    float peak = 0.0f;

    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float sample = samples[i];
        const float absSample = std::abs(sample);
        sumSquares += sample * sample;
        sumAbs += absSample;
        peak = std::max(peak, absSample);
    }

    const float count = static_cast<float>(sampleCount);
    metrics.rms = std::sqrt(sumSquares / count);
    metrics.peak = peak;
    metrics.transient = std::max(0.0f, peak - (sumAbs / count));
    return metrics;
}

const float* tailSamples(const std::vector<float>& samples, std::size_t maxSampleCount, std::size_t& sampleCount) {
    if (samples.empty() || maxSampleCount == 0) {
        sampleCount = 0;
        return nullptr;
    }

    sampleCount = std::min(maxSampleCount, samples.size());
    return samples.data() + (samples.size() - sampleCount);
}

AudioMetrics computeTailAudioMetrics(const std::vector<float>& samples, std::size_t maxSampleCount) {
    std::size_t sampleCount = 0;
    const float* tail = tailSamples(samples, maxSampleCount, sampleCount);
    if (!tail || sampleCount == 0) {
        return {};
    }

    return computeAudioMetrics(tail, sampleCount);
}

void splitInterleavedStereoSamples(const std::vector<float>& interleavedSamples, std::vector<float>& leftSamples, std::vector<float>& rightSamples) {
    leftSamples.clear();
    rightSamples.clear();

    if (interleavedSamples.size() < 4 || (interleavedSamples.size() % 2) != 0) {
        return;
    }

    const std::size_t frameCount = interleavedSamples.size() / 2;
    leftSamples.resize(frameCount);
    rightSamples.resize(frameCount);

    for (std::size_t i = 0, frame = 0; i + 1 < interleavedSamples.size(); i += 2, ++frame) {
        leftSamples[frame] = interleavedSamples[i];
        rightSamples[frame] = interleavedSamples[i + 1];
    }
}

struct DualEchoFeedbackState {
    ALLEGRO_BITMAP* historyA = nullptr;
    ALLEGRO_BITMAP* historyB = nullptr;
    ALLEGRO_BITMAP* currentFrame = nullptr;
    int width = 0;
    int height = 0;
    int historyIndex = 0;
    std::unique_ptr<vis::Shader> feedbackShader;
    std::vector<ALLEGRO_VERTEX> topChannelVertices;
    std::vector<ALLEGRO_VERTEX> bottomChannelVertices;

    void releaseResources() {
        if (historyA) {
            al_destroy_bitmap(historyA);
            historyA = nullptr;
        }
        if (historyB) {
            al_destroy_bitmap(historyB);
            historyB = nullptr;
        }
        if (currentFrame) {
            al_destroy_bitmap(currentFrame);
            currentFrame = nullptr;
        }

        width = 0;
        height = 0;
        historyIndex = 0;
        feedbackShader.reset();
        topChannelVertices.clear();
        bottomChannelVertices.clear();
    }

    bool ensureRenderTargets(int newWidth, int newHeight) {
        if (newWidth <= 0 || newHeight <= 0) {
            return false;
        }

        if (width == newWidth && height == newHeight && historyA && historyB && currentFrame) {
            return true;
        }

        if (historyA) {
            al_destroy_bitmap(historyA);
            historyA = nullptr;
        }
        if (historyB) {
            al_destroy_bitmap(historyB);
            historyB = nullptr;
        }
        if (currentFrame) {
            al_destroy_bitmap(currentFrame);
            currentFrame = nullptr;
        }

        width = newWidth;
        height = newHeight;
        historyIndex = 0;

        historyA = al_create_bitmap(width, height);
        historyB = al_create_bitmap(width, height);
        currentFrame = al_create_bitmap(width, height);
        if (!historyA || !historyB || !currentFrame) {
            return false;
        }

        ALLEGRO_BITMAP* previousTarget = al_get_target_bitmap();
        al_set_target_bitmap(historyA);
        al_clear_to_color(al_map_rgba(0, 0, 0, 0));
        al_set_target_bitmap(historyB);
        al_clear_to_color(al_map_rgba(0, 0, 0, 0));
        al_set_target_bitmap(currentFrame);
        al_clear_to_color(al_map_rgba(0, 0, 0, 0));
        al_set_target_bitmap(previousTarget);

        return true;
    }
};

class PolarWaveformVisualization final : public ui::AudioVisualizerView::Visualization {
public:
    void update(const ui::AudioVisualizerView::FrameContext& context) override {
        const std::vector<float>* samples = context.samples ? &context.samples->mono : nullptr;
        if (samples && !samples->empty()) {
            metrics = computeTailAudioMetrics(*samples, kPolarWaveformSampleWindow);
        } else {
            metrics = {};
        }
    }

    void draw(const ui::AudioVisualizerView::FrameContext& context) override {
        const std::vector<float>* audioSamples = context.samples ? &context.samples->mono : nullptr;
        if (!audioSamples || audioSamples->size() < 2) {
            return;
        }

        const float centerX = context.x + (context.w * 0.5f);
        const float centerY = context.y + (context.h * 0.5f);
        const float baseRadius = std::min(context.w, context.h) * 0.15f;
        const float maxRadius = std::min(context.w, context.h) * 0.35f;
        const ALLEGRO_COLOR waveformColor = al_map_rgba(255, 255, 255, 220);
        const float twoPi = 2.0f * 3.14159265359f;

        bool shaderActive = false;
        if (shader && shader->isLoaded()) {
            shader->use();
            shaderActive = true;
            shader->setFloat("time", context.timeSeconds);

            const float visCenter[2] = {
                context.x + (context.w * 0.5f),
                context.y + (context.h * 0.5f)
            };
            const float visRadius = (std::min(context.w, context.h) * 0.15f) + (std::min(context.w, context.h) * 0.35f * 0.5f);
            shader->setFloatVector("vis_center", 2, visCenter, 1);
            shader->setFloat("vis_radius", visRadius);
            shader->setFloat("audio_rms", metrics.rms);
            shader->setFloat("audio_peak", metrics.peak);
            shader->setFloat("audio_transient", metrics.transient);
        }

        std::vector<ALLEGRO_VERTEX> vertices;
        vertices.reserve(audioSamples->size() * 2);

        for (std::size_t i = 0; i < audioSamples->size(); ++i) {
            const std::size_t nextIdx = (i + 1) % audioSamples->size();

            const float angle1 = (static_cast<float>(i) / audioSamples->size()) * twoPi;
            const float angle2 = (static_cast<float>(nextIdx) / audioSamples->size()) * twoPi;

            const float sample1 = std::clamp((*audioSamples)[i], -1.0f, 1.0f);
            const float sample2 = std::clamp((*audioSamples)[nextIdx], -1.0f, 1.0f);
            const float radius1 = baseRadius + std::abs(sample1) * maxRadius;
            const float radius2 = baseRadius + std::abs(sample2) * maxRadius;

            const float px1 = centerX + radius1 * std::cos(angle1);
            const float py1 = centerY + radius1 * std::sin(angle1);
            const float px2 = centerX + radius2 * std::cos(angle2);
            const float py2 = centerY + radius2 * std::sin(angle2);

            ALLEGRO_VERTEX v1 = {px1, py1, 0.0f, 0.0f, 0.0f, waveformColor};
            ALLEGRO_VERTEX v2 = {px2, py2, 0.0f, 0.0f, 0.0f, waveformColor};
            vertices.push_back(v1);
            vertices.push_back(v2);
        }

        if (!vertices.empty()) {
            al_draw_prim(vertices.data(), nullptr, nullptr, 0, static_cast<int>(vertices.size()), ALLEGRO_PRIM_LINE_LIST);
        }

        if (shaderActive) {
            al_use_shader(nullptr);
        }
    }

private:
    AudioMetrics metrics;
};

class MirrorBarsVisualization final : public ui::AudioVisualizerView::Visualization {
public:
    void update(const ui::AudioVisualizerView::FrameContext& context) override {
        const std::vector<float>* samples = context.samples ? &context.samples->mono : nullptr;
        if (samples && !samples->empty()) {
            const AudioMetrics metrics = computeTailAudioMetrics(*samples, kDualEchoMonoFallbackWindow);
            intensity = std::clamp(metrics.peak * 0.75f + metrics.rms * 0.25f, 0.0f, 1.0f);
        } else {
            intensity = 0.0f;
        }
    }

    void draw(const ui::AudioVisualizerView::FrameContext& context) override {
        const std::vector<float>* audioSamples = context.samples ? &context.samples->mono : nullptr;
        if (!audioSamples || audioSamples->empty()) {
            return;
        }

        bool shaderActive = false;
        if (shader && shader->isLoaded()) {
            shader->use();
            shaderActive = true;
            shader->setFloat("time", context.timeSeconds);
            const float tint[3] = {
                0.30f + (0.55f * intensity),
                0.55f + (0.35f * intensity),
                0.95f
            };
            shader->setFloatVector("tint", 3, tint, 1);
        }

        const float centerY = context.y + (context.h * 0.5f);
        const float columnWidth = std::max(1.0f, context.w / 160.0f);
        const float spacing = std::max(1.0f, columnWidth * 1.35f);
        const int barCount = static_cast<int>(std::max(8.0f, std::floor(context.w / spacing)));
        const int sampleStride = std::max<int>(1, static_cast<int>(audioSamples->size() / static_cast<std::size_t>(barCount)));
        const ALLEGRO_COLOR barColor = al_map_rgba(246, 250, 255, 210);

        for (int i = 0; i < barCount; ++i) {
            const std::size_t sampleIndex = std::min(audioSamples->size() - 1, static_cast<std::size_t>(i * sampleStride));
            const float magnitude = std::abs(std::clamp((*audioSamples)[sampleIndex], -1.0f, 1.0f));
            const float halfHeight = std::max(1.0f, magnitude * context.h * 0.45f);
            const float left = context.x + (i * spacing);
            const float right = left + columnWidth;

            al_draw_filled_rectangle(left, centerY - halfHeight, right, centerY + halfHeight, barColor);
        }

        if (shaderActive) {
            al_use_shader(nullptr);
        }
    }

private:
    float intensity = 0.0f;
};

class DualEchoWaveVisualization final : public ui::AudioVisualizerView::Visualization {
public:
    void draw(const ui::AudioVisualizerView::FrameContext& context) override {
        const std::vector<float>* topSamples = context.samples ? &context.samples->left : nullptr;
        const std::vector<float>* bottomSamples = context.samples ? &context.samples->right : nullptr;
        if ((!topSamples || topSamples->size() < 2) && (!bottomSamples || bottomSamples->size() < 2)) {
            return;
        }

        const int texW = std::max(1, static_cast<int>(std::round(context.w)));
        const int texH = std::max(1, static_cast<int>(std::round(context.h)));
        if (!feedbackState.ensureRenderTargets(texW, texH)) {
            return;
        }

        if (!feedbackState.feedbackShader) {
            const std::string vertexSource = util::Config::resolveAssetPath("shaders/vertex.glsl");
            const std::string fragmentSource = util::Config::resolveAssetPath("shaders/dual_echo_feedback.glsl");
            feedbackState.feedbackShader = std::make_unique<vis::Shader>(vertexSource.c_str(), fragmentSource.c_str(), true);
        }

        const float topBaseline = texH * 0.28f;
        const float bottomBaseline = texH * 0.72f;
        const float amplitude = texH * 0.25f;

        auto drawChannel = [&](const std::vector<float>* samples,
                               float baselineY,
                               ALLEGRO_COLOR color,
                               std::vector<ALLEGRO_VERTEX>& vertices) {
            if (!samples || samples->size() < 2) {
                vertices.clear();
                return;
            }

            const std::size_t segmentCount = samples->size() - 1;
            vertices.resize(segmentCount * 2);
            const float invWidthDenom = 1.0f / static_cast<float>(segmentCount);

            for (std::size_t i = 1, vertexIndex = 0; i < samples->size(); ++i, vertexIndex += 2) {
                const float x1 = (static_cast<float>(i - 1) * invWidthDenom) * texW;
                const float x2 = (static_cast<float>(i) * invWidthDenom) * texW;

                const float y1 = baselineY - std::clamp((*samples)[i - 1], -1.0f, 1.0f) * amplitude;
                const float y2 = baselineY - std::clamp((*samples)[i], -1.0f, 1.0f) * amplitude;

                vertices[vertexIndex] = {x1, y1, 0.0f, 0.0f, 0.0f, color};
                vertices[vertexIndex + 1] = {x2, y2, 0.0f, 0.0f, 0.0f, color};
            }

            al_draw_prim(vertices.data(), nullptr, nullptr, 0, static_cast<int>(vertices.size()), ALLEGRO_PRIM_LINE_LIST);
        };

        ALLEGRO_BITMAP* previousTarget = al_get_target_bitmap();
        al_set_target_bitmap(feedbackState.currentFrame);
        al_clear_to_color(al_map_rgba(0, 0, 0, 0));

        const ALLEGRO_COLOR topColor = al_map_rgba_f(1.0f, 0.15f, 0.15f, 0.95f);
        const ALLEGRO_COLOR bottomColor = al_map_rgba_f(0.15f, 0.15f, 1.0f, 0.95f);
        drawChannel(topSamples, topBaseline, topColor, feedbackState.topChannelVertices);
        drawChannel(bottomSamples, bottomBaseline, bottomColor, feedbackState.bottomChannelVertices);

        ALLEGRO_BITMAP* historySrc = (feedbackState.historyIndex == 0) ? feedbackState.historyA : feedbackState.historyB;
        ALLEGRO_BITMAP* historyDst = (feedbackState.historyIndex == 0) ? feedbackState.historyB : feedbackState.historyA;

        al_set_target_bitmap(historyDst);
        al_clear_to_color(al_map_rgba(0, 0, 0, 0));

        if (feedbackState.feedbackShader && feedbackState.feedbackShader->isLoaded()) {
            feedbackState.feedbackShader->use();

            float peak = 0.0f;
            float transient = 0.0f;
            const std::vector<float>* metricSource = context.samples ? &context.samples->mono : nullptr;
            if ((!metricSource || metricSource->empty()) && topSamples && !topSamples->empty()) {
                metricSource = topSamples;
            }

            if (metricSource && !metricSource->empty()) {
                const AudioMetrics metrics = computeTailAudioMetrics(*metricSource, kPolarWaveformSampleWindow);
                peak = metrics.peak;
                transient = metrics.transient;
            }

            feedbackState.feedbackShader->setFloat("audio_peak", peak);
            feedbackState.feedbackShader->setFloat("audio_transient", transient);

            const float decay = std::clamp(0.955f + (peak * 0.03f), 0.955f, 0.992f);
            feedbackState.feedbackShader->setFloat("echo_decay", decay);
            feedbackState.feedbackShader->setFloat("echo_speed", 0.0018f + (peak * 0.0034f));

            const float centerUv[2] = {0.5f, 0.5f};
            const float texelSize[2] = {1.0f / static_cast<float>(texW), 1.0f / static_cast<float>(texH)};
            feedbackState.feedbackShader->setFloatVector("echo_center", 2, centerUv, 1);
            feedbackState.feedbackShader->setFloatVector("texel_size", 2, texelSize, 1);
            feedbackState.feedbackShader->setTexture("history_tex", historySrc, 1);
        }

        al_draw_scaled_bitmap(feedbackState.currentFrame, 0.0f, 0.0f, texW, texH, 0.0f, 0.0f, texW, texH, 0);
        al_use_shader(nullptr);

        feedbackState.historyIndex = 1 - feedbackState.historyIndex;

        al_set_target_bitmap(previousTarget);
        al_draw_scaled_bitmap(historyDst, 0.0f, 0.0f, texW, texH, context.x, context.y, context.w, context.h, 0);
    }

private:
    DualEchoFeedbackState feedbackState;
};

} // namespace

// Factories (top-level vis namespace)
namespace vis {

std::unique_ptr<ui::AudioVisualizerView::Visualization> createPolarWaveformVisualization() {
    return std::make_unique<PolarWaveformVisualization>();
}

std::unique_ptr<ui::AudioVisualizerView::Visualization> createMirrorBarsVisualization() {
    return std::make_unique<MirrorBarsVisualization>();
}

std::unique_ptr<ui::AudioVisualizerView::Visualization> createDualEchoWaveVisualization() {
    return std::make_unique<DualEchoWaveVisualization>();
}

void splitInterleavedStereoSamples(const std::vector<float>& interleavedSamples, std::vector<float>& leftSamples, std::vector<float>& rightSamples) {
    leftSamples.clear();
    rightSamples.clear();

    if (interleavedSamples.size() < 4 || (interleavedSamples.size() % 2) != 0) {
        return;
    }

    const std::size_t frameCount = interleavedSamples.size() / 2;
    leftSamples.resize(frameCount);
    rightSamples.resize(frameCount);

    for (std::size_t i = 0, frame = 0; i + 1 < interleavedSamples.size(); i += 2, ++frame) {
        leftSamples[frame] = interleavedSamples[i];
        rightSamples[frame] = interleavedSamples[i + 1];
    }
}

} // namespace vis

namespace ui {
void shutdownAudioVisualizerResources() {
    // Nothing global to release currently; resources are owned per-visualization.
}
} // namespace ui
