#pragma once

#include <memory>

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

private:
    struct BitmapDeleter {
        void operator()(ALLEGRO_BITMAP* bmp) const;
    };

    bool isVisible = true;
    core::MusicEngine* musicEngine = nullptr; // non-owning dependency
    graphics::UV position{0.0f, 0.0f, 0.0f, 0.0f};
    graphics::UV size{1.0f, 1.0f, 0.0f, 0.0f};
    std::unique_ptr<ALLEGRO_BITMAP, BitmapDeleter> bitmap;
    std::unique_ptr<vis::Shader> shader;

};

};