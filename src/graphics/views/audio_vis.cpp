#include "graphics/views/audio_vis.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "core/music_engine.hpp"
#include "util/font.hpp"
#include "util/config.hpp"
#include "vis/visualizations.hpp"
#include "vis/shader.hpp"

namespace ui {

// Visualization implementations moved to /src/vis. Use the factory helpers there.

void AudioVisualizerView::SampleFrame::clear() {
    mono.clear();
    interleaved.clear();
    left.clear();
    right.clear();
}

AudioVisualizerView::Visualization::~Visualization() = default;

void AudioVisualizerView::Visualization::update(const FrameContext&) {
}

void AudioVisualizerView::Visualization::setShader(vis::Shader* newShader) {
    shader.reset(newShader);
}

vis::Shader* AudioVisualizerView::Visualization::getShader() const {
    return shader.get();
}

void AudioVisualizerView::BitmapDeleter::operator()(ALLEGRO_BITMAP* bmp) const {
    if (bmp) {
        al_destroy_bitmap(bmp);
    }
}

AudioVisualizerView::AudioVisualizerView(std::shared_ptr<util::FontManager> fontManager, graphics::EventDispatcher& eventDispatcher, core::MusicEngine* musicEngine)
    : fontManager(std::move(fontManager)),
      eventDispatcher(eventDispatcher),
      musicEngine(musicEngine),
      previousButton(std::make_shared<ButtonDrawable>(graphics::UV(), graphics::UV(), "Prev")),
      nextButton(std::make_shared<ButtonDrawable>(graphics::UV(), graphics::UV(), "Next")) {
    auto kanitFont = this->fontManager ? this->fontManager->getFont("kanit") : nullptr;
    controlFont = kanitFont ? kanitFont->getFont(14) : nullptr;
    previousButton->setFont(controlFont);
    nextButton->setFont(controlFont);
    previousButton->setColors(
        al_map_rgb(52, 56, 70),
        al_map_rgb(68, 74, 92),
        al_map_rgb(82, 90, 112)
    );
    nextButton->setColors(
        al_map_rgb(52, 56, 70),
        al_map_rgb(68, 74, 92),
        al_map_rgb(82, 90, 112)
    );
    previousButton->setTextColor(al_map_rgb(245, 247, 250));
    nextButton->setTextColor(al_map_rgb(245, 247, 250));
    previousButton->setOnClick([this]() { previousVisualization(); });
    nextButton->setOnClick([this]() { nextVisualization(); });
    eventDispatcher.addEventTarget(previousButton);
    eventDispatcher.addEventTarget(nextButton);

    const std::string vertexSource = util::Config::resolveAssetPath("shaders/vertex.glsl");
    const std::string polarFragmentSource = util::Config::resolveAssetPath("shaders/pixel.glsl");
    const std::string barsFragmentSource = util::Config::resolveAssetPath("shaders/rainbow.glsl");

    const std::size_t polarIndex = visualizationToIndex(VisualizationType::PolarWaveform);
    visualizations[polarIndex] = vis::createPolarWaveformVisualization();
    visualizations[polarIndex]->setShader(new vis::Shader(vertexSource.c_str(), polarFragmentSource.c_str(), true));

    const std::size_t barsIndex = visualizationToIndex(VisualizationType::MirrorBars);
    visualizations[barsIndex] = vis::createMirrorBarsVisualization();
    visualizations[barsIndex]->setShader(new vis::Shader(vertexSource.c_str(), barsFragmentSource.c_str(), true));

    const std::size_t dualEchoIndex = visualizationToIndex(VisualizationType::DualEchoWave);
    visualizations[dualEchoIndex] = vis::createDualEchoWaveVisualization();
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
    if (previousButton) {
        previousButton->setEnabled(visible);
    }
    if (nextButton) {
        nextButton->setEnabled(visible);
    }
}

void AudioVisualizerView::setBitmap(ALLEGRO_BITMAP* newBitmap) {
    bitmap.reset(newBitmap);
}

ALLEGRO_BITMAP* AudioVisualizerView::getBitmap() const {
    return bitmap.get();
}

void AudioVisualizerView::setShader(vis::Shader* newShader) {
    const std::size_t activeIndex = visualizationToIndex(activeVisualization);
    if (activeIndex >= visualizations.size() || !visualizations[activeIndex]) {
        return;
    }

    visualizations[activeIndex]->setShader(newShader);
}

vis::Shader* AudioVisualizerView::getShader() const {
    const std::size_t activeIndex = visualizationToIndex(activeVisualization);
    if (activeIndex >= visualizations.size() || !visualizations[activeIndex]) {
        return nullptr;
    }

    return visualizations[activeIndex]->getShader();
}

void AudioVisualizerView::setVisualization(VisualizationType visualization) {
    const std::size_t index = visualizationToIndex(visualization);
    if (index >= visualizations.size()) {
        return;
    }

    activeVisualization = visualization;
}

void AudioVisualizerView::nextVisualization() {
    const std::size_t nextIndex = (visualizationToIndex(activeVisualization) + 1) % kVisualizationCount;
    setVisualization(static_cast<VisualizationType>(nextIndex));
}

void AudioVisualizerView::previousVisualization() {
    const std::size_t currentIndex = visualizationToIndex(activeVisualization);
    const std::size_t previousIndex = (currentIndex + kVisualizationCount - 1) % kVisualizationCount;
    setVisualization(static_cast<VisualizationType>(previousIndex));
}

const char* AudioVisualizerView::getVisualizationName() const {
    return visualizationName(activeVisualization);
}

void AudioVisualizerView::layoutControls(const graphics::RenderContext& context, float x, float y, float w, float h) {
    const float stripHeight = std::min(kControlStripHeight, std::max(0.0f, h));
    const float stripY = y + h - stripHeight;
    const float buttonY = stripY + std::max(0.0f, (stripHeight - kControlButtonHeight) * 0.5f);
    const float previousX = x + kControlStripPadding;
    const float nextX = x + w - kControlStripPadding - kControlButtonWidth;

    previousButton->setPosition(screenToUV(previousX, buttonY, 0.0f, 0.0f, context.screenWidth, context.screenHeight));
    previousButton->setSize(screenToUV(0.0f, 0.0f, kControlButtonWidth, kControlButtonHeight, context.screenWidth, context.screenHeight));
    nextButton->setPosition(screenToUV(nextX, buttonY, 0.0f, 0.0f, context.screenWidth, context.screenHeight));
    nextButton->setSize(screenToUV(0.0f, 0.0f, kControlButtonWidth, kControlButtonHeight, context.screenWidth, context.screenHeight));
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

    layoutControls(context, x, y, w, h);

    SampleFrame sampleFrame;
    if (musicEngine) {
        if (activeVisualization == VisualizationType::DualEchoWave) {
            sampleFrame.interleaved = musicEngine->copyRecentSamples(vis::kDualEchoStereoSampleWindow);
            if (sampleFrame.interleaved.size() >= 4 && (sampleFrame.interleaved.size() % 2 == 0)) {
                vis::splitInterleavedStereoSamples(sampleFrame.interleaved, sampleFrame.left, sampleFrame.right);
            } else {
                sampleFrame.mono = musicEngine->copyRecentMonoSamples(vis::kDualEchoMonoFallbackWindow);
                sampleFrame.left = sampleFrame.mono;
                sampleFrame.right = sampleFrame.mono;
            }
        } else {
            const std::size_t monoWindow = (activeVisualization == VisualizationType::MirrorBars)
                ? vis::kMirrorBarsSampleWindow
                : vis::kPolarWaveformSampleWindow;
            sampleFrame.mono = musicEngine->copyRecentMonoSamples(monoWindow);
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

    const std::size_t activeIndex = visualizationToIndex(activeVisualization);
    Visualization* visualization = (activeIndex < visualizations.size()) ? visualizations[activeIndex].get() : nullptr;
    if (visualization) {
        FrameContext frameContext;
        frameContext.x = x;
        frameContext.y = y;
        frameContext.w = w;
        frameContext.h = h;
        frameContext.timeSeconds = static_cast<float>(al_get_time());
        frameContext.samples = &sampleFrame;
        visualization->update(frameContext);
        visualization->draw(frameContext);
    }

    const float stripHeight = std::min(kControlStripHeight, std::max(0.0f, h));
    const float stripY = y + h - stripHeight;
    const float stripRight = x + w;
    al_draw_filled_rectangle(x, stripY, stripRight, y + h, al_map_rgba(18, 20, 28, 220));
    al_draw_rectangle(x, stripY, stripRight, y + h, al_map_rgba(92, 96, 116, 220), 1.0f);

    if (controlFont) {
        const std::string label = std::string("Visualization: ") + getVisualizationName();
        const float textY = stripY + std::max(0.0f, (stripHeight - static_cast<float>(al_get_font_line_height(controlFont))) * 0.5f);
        al_draw_text(controlFont, al_map_rgb(242, 244, 248), x + (w * 0.5f), textY, ALLEGRO_ALIGN_CENTRE, label.c_str());
    }

    if (previousButton) {
        previousButton->draw(context);
    }
    if (nextButton) {
        nextButton->draw(context);
    }
}

// shutdownAudioVisualizerResources implemented in src/vis/visualizations.cpp

std::size_t AudioVisualizerView::visualizationToIndex(VisualizationType visualization) {
    return static_cast<std::size_t>(visualization);
}

graphics::UV AudioVisualizerView::screenToUV(float x, float y, float w, float h, float screenWidth, float screenHeight) {
    return graphics::UV(
        screenWidth > 0.0f ? (x / screenWidth) : 0.0f,
        screenHeight > 0.0f ? (y / screenHeight) : 0.0f,
        w,
        h
    );
}

const char* AudioVisualizerView::visualizationName(VisualizationType visualization) {
    switch (visualization) {
        case VisualizationType::PolarWaveform:
            return "Polar Waveform";
        case VisualizationType::MirrorBars:
            return "Mirror Bars";
        case VisualizationType::DualEchoWave:
            return "Dual Echo Wave";
        default:
            return "Visualization";
    }
}

} // namespace ui

