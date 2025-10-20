#include "graphics/views/now_playing.hpp"
#include "util/duration.hpp"

namespace ui {

NowPlayingView::NowPlayingView(std::shared_ptr<ui::ProgressBar> progressBarModel) : progressBar(progressBarModel) {
    progressBar.setPosition(graphics::UV(0.2f, 0.5f, 0.0f, 0.0f));
    progressBar.setSize(graphics::UV(0.6f, 0.05f, 0.0f, 0.0f));

    songTitleText = TextDrawable("Title", graphics::UV(0.5f, 0.1f, 0.0f, 0.0f), 24);
    artistNameText = TextDrawable("Artist", graphics::UV(0.5f, 0.2f, 0.0f, 0.0f), 20);
    albumNameText = TextDrawable("Album", graphics::UV(0.5f, 0.3f, 0.0f, 0.0f), 20);
    songPositionText = TextDrawable("0:00", graphics::UV(0.15f, 0.6f, 0.0f, 0.0f), 16);
    songDurationText = TextDrawable("0:00", graphics::UV(0.8f, 0.6f, 0.0f, 0.0f), 16);
}

void NowPlayingView::setSongTitle(const std::string& title) {
    songTitleText.setText(title);
}

void NowPlayingView::setArtistName(const std::string& artist) {
    artistNameText.setText(artist);
}

void NowPlayingView::setAlbumName(const std::string& album) {
    albumNameText.setText(album);
}

void NowPlayingView::setDuration(int duration) {
    songDurationText.setText(util::format_mm_ss(duration));
}

void NowPlayingView::setPosition(int position) {
    songPositionText.setText(util::format_mm_ss(position));
}

void NowPlayingView::draw() {
    // Draw the UI elements
    songTitleText.draw();
    artistNameText.draw();
    albumNameText.draw();
    songPositionText.draw();
    songDurationText.draw();
    progressBar.draw();
}

};