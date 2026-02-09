#pragma once

#include "graphics/drawables/progress_bar.hpp"
#include "graphics/drawables/text.hpp"
#include "graphics/drawables/frame.hpp"
#include "graphics/drawables/image.hpp"
#include "graphics/drawables/button.hpp"
#include "graphics/event_handler.hpp"
#include "graphics/uv.hpp"
#include <memory>

namespace util {
    class FontManager;
}

namespace core {
    class MusicEngine;
}

namespace ui {

class NowPlayingView {
public:
    NowPlayingView() = delete;
    NowPlayingView(std::shared_ptr<util::FontManager> fontManager, 
        std::shared_ptr<ui::ProgressBar> progressBarModel,
        graphics::EventDispatcher& eventDispatcher,
        core::MusicEngine* musicEngine);

    void setSongTitle(const std::string& title);
    void setArtistName(const std::string& artist);
    void setAlbumName(const std::string& album);
    void setDuration(int duration);
    void setPosition(int position);
    void setAlbumArt(ALLEGRO_BITMAP* bitmap);

    void draw(const graphics::RenderContext& context);
    void updatePlayPauseButton();
    void updateLoopButton();
private:
    void recalculateLayout(const graphics::RenderContext& context);
    
    std::shared_ptr<util::FontManager> fontManager;
    core::MusicEngine* musicEngine;
    int lastDisplayWidth = 0;
    int lastDisplayHeight = 0;

    ButtonDrawable playPauseButton;
    ButtonDrawable rewindButton;
    ButtonDrawable skipButton;
    ButtonDrawable loopButton;
    std::shared_ptr<FrameDrawable> mainFrame;
    
    // Text drawables
    TextDrawable songTitleText;
    TextDrawable artistNameText;
    TextDrawable albumNameText;
    TextDrawable songPositionText;
    TextDrawable songDurationText;

    // Album drawable
    ImageDrawable albumArtImage;
    
    // Progress bar
    ProgressBarDrawable progressBar;
};

}