#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include "graphics/drawables/button.hpp"
#include "graphics/uv.hpp"
#include "graphics/drawable.hpp"

struct ALLEGRO_BITMAP;

namespace core {
class MusicEngine;
}

namespace graphics {
class EventDispatcher;
}

namespace vis {
class Shader;
}

namespace util {
class FontManager;
}

namespace ui {

// Release shared visualization resources that outlive AudioVisualizerView instances.
// Must be called before Allegro display shutdown.
void shutdownAudioVisualizerResources();

class AudioVisualizerView {
public:
    enum class VisualizationType : std::size_t {
        PolarWaveform = 0,
        MirrorBars = 1,
        DualEchoWave = 2,
    };

    AudioVisualizerView() = delete;
    explicit AudioVisualizerView(std::shared_ptr<util::FontManager> fontManager, graphics::EventDispatcher& eventDispatcher, core::MusicEngine* musicEngine);
    ~AudioVisualizerView();

    void draw(const graphics::RenderContext& context);

    void setMusicEngine(core::MusicEngine* musicEngine);
    core::MusicEngine* getMusicEngine() const { return musicEngine; }

    void setBounds(const graphics::UV& position, const graphics::UV& size);
    void setVisible(bool visible);
    bool getVisible() const { return isVisible; }

    // Adopts ownership of the passed bitmap pointer.
    void setBitmap(ALLEGRO_BITMAP* bitmap);
    ALLEGRO_BITMAP* getBitmap() const;

    // Adopts ownership of the passed shader pointer.
    void setShader(vis::Shader* shader);
    vis::Shader* getShader() const;

    void setVisualization(VisualizationType visualization);
    VisualizationType getVisualization() const { return activeVisualization; }
    void nextVisualization();
    void previousVisualization();
    const char* getVisualizationName() const;

private:
    static constexpr std::size_t kVisualizationCount = 3;
    static constexpr float kControlStripHeight = 36.0f;
    static constexpr float kControlStripPadding = 10.0f;
    static constexpr float kControlButtonWidth = 64.0f;
    static constexpr float kControlButtonHeight = 22.0f;

    struct DrawContext {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        float timeSeconds = 0.0f;
        const std::vector<float>* monoSamples = nullptr;
        const std::vector<float>* leftSamples = nullptr;
        const std::vector<float>* rightSamples = nullptr;
    };

    struct Visualization {
        std::unique_ptr<vis::Shader> shader;
        void (*configureShader)(vis::Shader& shader, const DrawContext& context) = nullptr;
        void (*render)(const DrawContext& context) = nullptr;
    };

    struct BitmapDeleter {
        void operator()(ALLEGRO_BITMAP* bmp) const;
    };

    static std::size_t visualizationToIndex(VisualizationType visualization);
    static graphics::UV screenToUV(float x, float y, float w, float h, float screenWidth, float screenHeight);
    static const char* visualizationName(VisualizationType visualization);
    static void configureAudioReactiveShader(vis::Shader& shader, const DrawContext& context);
    static void configureTintedShader(vis::Shader& shader, const DrawContext& context);
    static void renderPolarWaveform(const DrawContext& context);
    static void renderMirrorBars(const DrawContext& context);
    static void renderDualEchoWave(const DrawContext& context);
    void layoutControls(const graphics::RenderContext& context, float x, float y, float w, float h);

    bool isVisible = true;
    std::shared_ptr<util::FontManager> fontManager;
    graphics::EventDispatcher& eventDispatcher;
    core::MusicEngine* musicEngine = nullptr; // non-owning dependency
    graphics::UV position{0.0f, 0.0f, 0.0f, 0.0f};
    graphics::UV size{1.0f, 1.0f, 0.0f, 0.0f};
    std::unique_ptr<ALLEGRO_BITMAP, BitmapDeleter> bitmap;
    std::array<Visualization, kVisualizationCount> visualizations;
    VisualizationType activeVisualization = VisualizationType::DualEchoWave;
    std::shared_ptr<ui::ButtonDrawable> previousButton;
    std::shared_ptr<ui::ButtonDrawable> nextButton;
    ALLEGRO_FONT* controlFont = nullptr;

};

};