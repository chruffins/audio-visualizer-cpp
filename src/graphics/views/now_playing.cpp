#include "graphics/views/now_playing.hpp"
#include "util/duration.hpp"

namespace ui {

NowPlayingView::NowPlayingView(std::shared_ptr<util::FontManager> fontManager, std::shared_ptr<ui::ProgressBar> progressBarModel)
    : fontManager(fontManager), progressBar(progressBarModel) {
  float progressBarStart = 0.3f;
  float progressBarWidth = 0.4f;
  float textOffsets = 10.0f;

  auto font = fontManager->getFont("courier");

  progressBar.setPosition(graphics::UV(progressBarStart, 0.9f, 0.0f, 0.0f));
  progressBar.setSize(graphics::UV(progressBarWidth, 0.05f, 0.0f, 0.0f));

  songTitleText =
      TextDrawable("Title", graphics::UV(0.0f, 0.85f, textOffsets, 0.0f), graphics::UV(0.5f, 0.1f, 0.0f, 0.0f), 16)
          .setFont(font)
          .setMultiline(false)
          .setAlignment(TextDrawable::HorizontalAlignment::Left);

  artistNameText =
      TextDrawable("Artist", graphics::UV(0.0f, 0.85f, textOffsets, 20.0f), graphics::UV(0.5f, 0.1f, 0.0f, 0.0f), 16)
          .setFont(font)
          .setMultiline(false)
          .setAlignment(TextDrawable::HorizontalAlignment::Left);
  albumNameText =
      TextDrawable("Album", graphics::UV(0.5f, 0.3f, 0.0f, 0.0f), graphics::UV(0.0f, 0.0f, 200.0f, 0.0f), 20);
  songPositionText =
      TextDrawable("0:00", graphics::UV(progressBarStart, 0.9f, -textOffsets, 0.0f), graphics::UV(0.15f, 0.05f, 0.0f, 0.0f), 16)
          .setVerticalAlignment(graphics::VerticalAlignment::CENTER)
          .setAlignment(TextDrawable::HorizontalAlignment::Right);
  songDurationText =
      TextDrawable("0:00", graphics::UV(progressBarStart + progressBarWidth, 0.9f, textOffsets, 0.0f), graphics::UV(0.15f, 0.05f, 0.0f, 0.0f), 16)
          .setVerticalAlignment(graphics::VerticalAlignment::CENTER)
          .setAlignment(TextDrawable::HorizontalAlignment::Left);
}

void NowPlayingView::setSongTitle(const std::string &title) {
  songTitleText.setText(title);
}

void NowPlayingView::setArtistName(const std::string &artist) {
  artistNameText.setText(artist);
}

void NowPlayingView::setAlbumName(const std::string &album) {
  albumNameText.setText(album);
}

void NowPlayingView::setDuration(int duration) {
  songDurationText.setText(util::format_mm_ss(duration));
}

void NowPlayingView::setPosition(int position) {
  songPositionText.setText(util::format_mm_ss(position));
}

void NowPlayingView::draw(const graphics::RenderContext& context) {
  // Draw the UI elements using the provided rendering context. The
  // context must be populated by the caller (e.g., the main loop) so that
  // Allegro global queries are centralized.
  songTitleText.draw(context);
  artistNameText.draw(context);
  albumNameText.draw(context);
  songPositionText.draw(context);
  songDurationText.draw(context);
  progressBar.draw(context);
}

}; // namespace ui