#include "graphics/views/audio_vis.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

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

constexpr size_t kPolarWaveformSampleWindow = 1024;
constexpr size_t kMirrorBarsSampleWindow = 768;
constexpr size_t kDualEchoStereoSampleWindow = 1024;
constexpr size_t kDualEchoMonoFallbackWindow = 512;

AudioMetrics computeAudioMetrics(const float* samples, size_t sampleCount) {
    AudioMetrics metrics;
    if (!samples || sampleCount == 0) {
        return metrics;
    }

    float sumSquares = 0.0f;
    float sumAbs = 0.0f;
    float peak = 0.0f;

    for (size_t i = 0; i < sampleCount; ++i) {
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

    ~DualEchoFeedbackState() = default;

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

DualEchoFeedbackState g_dualEchoFeedback;

} // namespace

void AudioVisualizerView::BitmapDeleter::operator()(ALLEGRO_BITMAP* bmp) const {
    if (bmp) {
        al_destroy_bitmap(bmp);
    }
}

AudioVisualizerView::AudioVisualizerView(core::MusicEngine* musicEngine)
    : musicEngine(musicEngine) {
        const std::string vertexSource = util::Config::resolveAssetPath("shaders/vertex.glsl");
        const std::string polarFragmentSource = util::Config::resolveAssetPath("shaders/pixel.glsl");
        const std::string barsFragmentSource = util::Config::resolveAssetPath("shaders/rainbow.glsl");

        const std::size_t polarIndex = visualizationToIndex(VisualizationType::PolarWaveform);
        visualizations[polarIndex].shader = std::make_unique<vis::Shader>(vertexSource.c_str(), polarFragmentSource.c_str(), true);
        visualizations[polarIndex].configureShader = &AudioVisualizerView::configureAudioReactiveShader;
        visualizations[polarIndex].render = &AudioVisualizerView::renderPolarWaveform;

        const std::size_t barsIndex = visualizationToIndex(VisualizationType::MirrorBars);
        visualizations[barsIndex].shader = std::make_unique<vis::Shader>(vertexSource.c_str(), barsFragmentSource.c_str(), true);
        visualizations[barsIndex].configureShader = &AudioVisualizerView::configureTintedShader;
        visualizations[barsIndex].render = &AudioVisualizerView::renderMirrorBars;

        const std::size_t dualEchoIndex = visualizationToIndex(VisualizationType::DualEchoWave);
        visualizations[dualEchoIndex].configureShader = nullptr;
        visualizations[dualEchoIndex].render = &AudioVisualizerView::renderDualEchoWave;
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
    const std::size_t activeIndex = visualizationToIndex(activeVisualization);
    visualizations[activeIndex].shader.reset(newShader);
}

vis::Shader* AudioVisualizerView::getShader() const {
    const std::size_t activeIndex = visualizationToIndex(activeVisualization);
    return visualizations[activeIndex].shader.get();
}

void AudioVisualizerView::setVisualization(VisualizationType visualization) {
    const std::size_t index = visualizationToIndex(visualization);
    if (index >= visualizations.size()) {
        return;
    }

    activeVisualization = visualization;
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

    const std::size_t activeIndex = visualizationToIndex(activeVisualization);
    const bool needsStereoChannels = (activeVisualization == VisualizationType::DualEchoWave);

    std::vector<float> monoSamples;
    std::vector<float> interleavedSamples;
    std::vector<float> leftSamples;
    std::vector<float> rightSamples;
    if (musicEngine) {
        if (needsStereoChannels) {
            interleavedSamples = musicEngine->copyRecentSamples(kDualEchoStereoSampleWindow);

            if (interleavedSamples.size() >= 4 && (interleavedSamples.size() % 2 == 0)) {
                const size_t frameCount = interleavedSamples.size() / 2;
                leftSamples.resize(frameCount);
                rightSamples.resize(frameCount);
                for (size_t i = 0, frame = 0; i + 1 < interleavedSamples.size(); i += 2, ++frame) {
                    leftSamples[frame] = interleavedSamples[i];
                    rightSamples[frame] = interleavedSamples[i + 1];
                }
            } else {
                monoSamples = musicEngine->copyRecentMonoSamples(kDualEchoMonoFallbackWindow);
                leftSamples = monoSamples;
                rightSamples = monoSamples;
            }
        } else {
            const size_t monoWindow = (activeVisualization == VisualizationType::MirrorBars)
                ? kMirrorBarsSampleWindow
                : kPolarWaveformSampleWindow;
            monoSamples = musicEngine->copyRecentMonoSamples(monoWindow);
        }
    }

    if (bitmap) {
        const float srcW = static_cast<float>(al_get_bitmap_width(bitmap.get()));
        const float srcH = static_cast<float>(al_get_bitmap_height(bitmap.get()));
        if (srcW > 0.0f && srcH > 0.0f) {
            al_draw_scaled_bitmap(bitmap.get(), 0.0f, 0.0f, srcW, srcH, x, y, w, h, 0);
        }
    } else {
        al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgb(12, 12, 20));
    }

    Visualization& visualization = visualizations[activeIndex];
    DrawContext drawContext;
    drawContext.x = x;
    drawContext.y = y;
    drawContext.w = w;
    drawContext.h = h;
    drawContext.timeSeconds = static_cast<float>(al_get_time());
    drawContext.monoSamples = &monoSamples;
    drawContext.leftSamples = &leftSamples;
    drawContext.rightSamples = &rightSamples;

    bool shaderActive = false;
    if (visualization.shader && visualization.shader->isLoaded()) {
        visualization.shader->use();
        shaderActive = true;
        visualization.shader->setFloat("time", drawContext.timeSeconds);

        if (visualization.configureShader) {
            visualization.configureShader(*visualization.shader, drawContext);
        }
    }

    if (visualization.render) {
        visualization.render(drawContext);
    }

    if (shaderActive) {
        al_use_shader(nullptr);
    }

}

void shutdownAudioVisualizerResources() {
    g_dualEchoFeedback.releaseResources();
}

std::size_t AudioVisualizerView::visualizationToIndex(VisualizationType visualization) {
    return static_cast<std::size_t>(visualization);
}

void AudioVisualizerView::configureAudioReactiveShader(vis::Shader& shader, const DrawContext& context) {
    const float visCenter[2] = {
        context.x + (context.w * 0.5f),
        context.y + (context.h * 0.5f)
    };
    const float visRadius = (std::min(context.w, context.h) * 0.15f) + (std::min(context.w, context.h) * 0.35f * 0.5f);
    shader.setFloatVector("vis_center", 2, visCenter, 1);
    shader.setFloat("vis_radius", visRadius);

    const std::vector<float>* samples = context.monoSamples;
    if (samples && !samples->empty()) {
        const size_t metricSampleCount = std::min<size_t>(1024, samples->size());
        const float* metricSamples = samples->data() + (samples->size() - metricSampleCount);
        const AudioMetrics metrics = computeAudioMetrics(metricSamples, metricSampleCount);
        shader.setFloat("audio_rms", metrics.rms);
        shader.setFloat("audio_peak", metrics.peak);
        shader.setFloat("audio_transient", metrics.transient);
    } else {
        shader.setFloat("audio_rms", 0.0f);
        shader.setFloat("audio_peak", 0.0f);
        shader.setFloat("audio_transient", 0.0f);
    }
}

void AudioVisualizerView::configureTintedShader(vis::Shader& shader, const DrawContext& context) {
    float intensity = 0.0f;
    const std::vector<float>* samples = context.monoSamples;
    if (samples && !samples->empty()) {
        const size_t metricSampleCount = std::min<size_t>(512, samples->size());
        const float* metricSamples = samples->data() + (samples->size() - metricSampleCount);
        const AudioMetrics metrics = computeAudioMetrics(metricSamples, metricSampleCount);
        intensity = std::clamp(metrics.peak * 0.75f + metrics.rms * 0.25f, 0.0f, 1.0f);
    }

    const float tint[3] = {
        0.30f + (0.55f * intensity),
        0.55f + (0.35f * intensity),
        0.95f
    };
    shader.setFloatVector("tint", 3, tint, 1);
}

void AudioVisualizerView::renderPolarWaveform(const DrawContext& context) {
    const std::vector<float>* audioSamples = context.monoSamples;
    if (!audioSamples || audioSamples->size() < 2) {
        return;
    }

    const float centerX = context.x + (context.w * 0.5f);
    const float centerY = context.y + (context.h * 0.5f);
    const float baseRadius = std::min(context.w, context.h) * 0.15f;
    const float maxRadius = std::min(context.w, context.h) * 0.35f;
    const ALLEGRO_COLOR waveformColor = al_map_rgba(255, 255, 255, 220);
    const float twoPi = 2.0f * 3.14159265359f;

    std::vector<ALLEGRO_VERTEX> vertices;
    vertices.reserve(audioSamples->size() * 2);

    for (size_t i = 0; i < audioSamples->size(); ++i) {
        const size_t nextIdx = (i + 1) % audioSamples->size();

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
        al_draw_prim(&vertices[0], nullptr, nullptr, 0, vertices.size(), ALLEGRO_PRIM_LINE_LIST);
    }
}

void AudioVisualizerView::renderMirrorBars(const DrawContext& context) {
    const std::vector<float>* audioSamples = context.monoSamples;
    if (!audioSamples || audioSamples->empty()) {
        return;
    }

    const float centerY = context.y + (context.h * 0.5f);
    const float columnWidth = std::max(1.0f, context.w / 160.0f);
    const float spacing = std::max(1.0f, columnWidth * 1.35f);
    const int barCount = static_cast<int>(std::max(8.0f, std::floor(context.w / spacing)));
    const int sampleStride = std::max<int>(1, static_cast<int>(audioSamples->size() / static_cast<size_t>(barCount)));
    const ALLEGRO_COLOR barColor = al_map_rgba(246, 250, 255, 210);

    for (int i = 0; i < barCount; ++i) {
        const size_t sampleIndex = std::min(audioSamples->size() - 1, static_cast<size_t>(i * sampleStride));
        const float magnitude = std::abs(std::clamp((*audioSamples)[sampleIndex], -1.0f, 1.0f));
        const float halfHeight = std::max(1.0f, magnitude * context.h * 0.45f);
        const float left = context.x + (i * spacing);
        const float right = left + columnWidth;

        al_draw_filled_rectangle(left, centerY - halfHeight, right, centerY + halfHeight, barColor);
    }
}

void AudioVisualizerView::renderDualEchoWave(const DrawContext& context) {
    const std::vector<float>* topSamples = context.leftSamples;
    const std::vector<float>* bottomSamples = context.rightSamples;
    if ((!topSamples || topSamples->size() < 2) && (!bottomSamples || bottomSamples->size() < 2)) {
        return;
    }

    const int texW = std::max(1, static_cast<int>(std::round(context.w)));
    const int texH = std::max(1, static_cast<int>(std::round(context.h)));
    if (!g_dualEchoFeedback.ensureRenderTargets(texW, texH)) {
        return;
    }

    if (!g_dualEchoFeedback.feedbackShader) {
        const std::string vertexSource = util::Config::resolveAssetPath("shaders/vertex.glsl");
        const std::string fragmentSource = util::Config::resolveAssetPath("shaders/dual_echo_feedback.glsl");
        g_dualEchoFeedback.feedbackShader = std::make_unique<vis::Shader>(vertexSource.c_str(), fragmentSource.c_str(), true);
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

        const size_t segmentCount = samples->size() - 1;
        vertices.resize(segmentCount * 2);
        const float invWidthDenom = 1.0f / static_cast<float>(segmentCount);

        for (size_t i = 1, vertexIndex = 0; i < samples->size(); ++i, vertexIndex += 2) {
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
    al_set_target_bitmap(g_dualEchoFeedback.currentFrame);
    al_clear_to_color(al_map_rgba(0, 0, 0, 0));

    const ALLEGRO_COLOR topColor = al_map_rgba_f(1.0f, 0.15f, 0.15f, 0.95f); // gb: 0.22, 0.28
    const ALLEGRO_COLOR bottomColor = al_map_rgba_f(0.15f, 0.15f, 1.0f, 0.95f); // rg: 0.24, 0.54
    drawChannel(topSamples, topBaseline, topColor, g_dualEchoFeedback.topChannelVertices);
    drawChannel(bottomSamples, bottomBaseline, bottomColor, g_dualEchoFeedback.bottomChannelVertices);

    ALLEGRO_BITMAP* historySrc = (g_dualEchoFeedback.historyIndex == 0) ? g_dualEchoFeedback.historyA : g_dualEchoFeedback.historyB;
    ALLEGRO_BITMAP* historyDst = (g_dualEchoFeedback.historyIndex == 0) ? g_dualEchoFeedback.historyB : g_dualEchoFeedback.historyA;

    al_set_target_bitmap(historyDst);
    al_clear_to_color(al_map_rgba(0, 0, 0, 0));

    if (g_dualEchoFeedback.feedbackShader && g_dualEchoFeedback.feedbackShader->isLoaded()) {
        g_dualEchoFeedback.feedbackShader->use();

        float peak = 0.0f;
        float transient = 0.0f;
        const std::vector<float>* metricSource = context.monoSamples;
        if ((!metricSource || metricSource->empty()) && topSamples && !topSamples->empty()) {
            metricSource = topSamples;
        }

        if (metricSource && !metricSource->empty()) {
            const size_t metricSampleCount = std::min<size_t>(1024, metricSource->size());
            const float* metricSamples = metricSource->data() + (metricSource->size() - metricSampleCount);
            const AudioMetrics metrics = computeAudioMetrics(metricSamples, metricSampleCount);
            peak = metrics.peak;
            transient = metrics.transient;
        }

        g_dualEchoFeedback.feedbackShader->setFloat("audio_peak", peak);
        g_dualEchoFeedback.feedbackShader->setFloat("audio_transient", transient);

        const float decay = std::clamp(0.955f + (peak * 0.03f), 0.955f, 0.992f);
        g_dualEchoFeedback.feedbackShader->setFloat("echo_decay", decay);
        g_dualEchoFeedback.feedbackShader->setFloat("echo_speed", 0.0018f + (peak * 0.0034f));

        const float centerUv[2] = {0.5f, 0.5f};
        const float texelSize[2] = {1.0f / static_cast<float>(texW), 1.0f / static_cast<float>(texH)};
        g_dualEchoFeedback.feedbackShader->setFloatVector("echo_center", 2, centerUv, 1);
        g_dualEchoFeedback.feedbackShader->setFloatVector("texel_size", 2, texelSize, 1);
        g_dualEchoFeedback.feedbackShader->setTexture("history_tex", historySrc, 1);
    }

    al_draw_scaled_bitmap(g_dualEchoFeedback.currentFrame, 0.0f, 0.0f, texW, texH, 0.0f, 0.0f, texW, texH, 0);
    al_use_shader(nullptr);

    g_dualEchoFeedback.historyIndex = 1 - g_dualEchoFeedback.historyIndex;

    al_set_target_bitmap(previousTarget);
    al_draw_scaled_bitmap(historyDst, 0.0f, 0.0f, texW, texH, context.x, context.y, context.w, context.h, 0);
}

} // namespace ui
