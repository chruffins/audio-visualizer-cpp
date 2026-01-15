#pragma once

#include "graphics/drawables/progress_bar.hpp"
#include "graphics/drawables/text.hpp"
#include "graphics/drawables/frame.hpp"
#include "graphics/uv.hpp"
#include <memory>

namespace util {
    class FontManager;
}

namespace ui {

class NowPlayingView {
public:
    NowPlayingView() = delete;
    NowPlayingView(std::shared_ptr<util::FontManager> fontManager, std::shared_ptr<ui::ProgressBar> progressBarModel);

    void setSongTitle(const std::string& title);
    void setArtistName(const std::string& artist);
    void setAlbumName(const std::string& album);
    void setDuration(int duration);
    void setPosition(int position);

    void draw(const graphics::RenderContext& context);
private:
    std::shared_ptr<util::FontManager> fontManager;

    // Main container frame
    FrameDrawable mainFrame;
    
    // Text drawables
    TextDrawable songTitleText;
    TextDrawable artistNameText;
    TextDrawable albumNameText;
    TextDrawable songPositionText;
    TextDrawable songDurationText;
    
    // Progress bar
    ProgressBarDrawable progressBar;
};

}