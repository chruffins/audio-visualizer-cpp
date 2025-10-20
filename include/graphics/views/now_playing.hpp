#pragma once

#include "graphics/drawables/progress_bar.hpp"
#include "graphics/drawables/text.hpp"
#include "graphics/uv.hpp"

namespace ui {

class NowPlayingView {
public:
    NowPlayingView() = delete;
    NowPlayingView(std::shared_ptr<ui::ProgressBar> progressBarModel);

    void setSongTitle(const std::string& title);
    void setArtistName(const std::string& artist);
    void setAlbumName(const std::string& album);
    void setDuration(int duration);
    void setPosition(int position);

    void draw();
private:
    graphics::UV position;
    ProgressBarDrawable progressBar;
    TextDrawable songTitleText;
    TextDrawable artistNameText;
    TextDrawable albumNameText;
    TextDrawable songPositionText;
    TextDrawable songDurationText;
};

}