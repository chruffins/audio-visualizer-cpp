#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "graphics/uv.hpp"
#include "graphics/drawable.hpp"

struct ALLEGRO_BITMAP;

namespace core {
class MusicEngine;
}

namespace vis {
class Shader;
}

namespace ui {

class AudioVisualizerView {
public:
    enum class VisualizationType : std::size_t {
        PolarWaveform = 0,
        MirrorBars = 1,
        DualEchoWave = 2,
    };

    AudioVisualizerView() = delete;
    explicit AudioVisualizerView(core::MusicEngine* musicEngine);
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

private:
    static constexpr std::size_t kVisualizationCount = 3;

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
    static void configureAudioReactiveShader(vis::Shader& shader, const DrawContext& context);
    static void configureTintedShader(vis::Shader& shader, const DrawContext& context);
    static void renderPolarWaveform(const DrawContext& context);
    static void renderMirrorBars(const DrawContext& context);
    static void renderDualEchoWave(const DrawContext& context);

    bool isVisible = true;
    core::MusicEngine* musicEngine = nullptr; // non-owning dependency
    graphics::UV position{0.0f, 0.0f, 0.0f, 0.0f};
    graphics::UV size{1.0f, 1.0f, 0.0f, 0.0f};
    std::unique_ptr<ALLEGRO_BITMAP, BitmapDeleter> bitmap;
    std::array<Visualization, kVisualizationCount> visualizations;
    VisualizationType activeVisualization = VisualizationType::DualEchoWave;

};

};