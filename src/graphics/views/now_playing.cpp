#include "graphics/views/now_playing.hpp"
#include "util/duration.hpp"
#include <allegro5/allegro.h>

namespace ui {

NowPlayingView::NowPlayingView(std::shared_ptr<util::FontManager> fontManager, std::shared_ptr<ui::ProgressBar> progressBarModel)
    : fontManager(fontManager), progressBar(progressBarModel) {
  
  auto font = fontManager->getFont("courier");

  mainFrame.setPosition(graphics::UV(0.0f, 0.8f, 0.0f, 0.0f));
  mainFrame.setSize(graphics::UV(1.0f, 0.2f, 0.0f, 0.0f));
  mainFrame.setBackgroundColor(al_map_rgba(30, 30, 40, 220));
  mainFrame.setBorderColor(al_map_rgb(80, 80, 100));
  mainFrame.setBorderThickness(2);
  mainFrame.setPadding(20.0f);

  // Song title - large, bold, at top
  songTitleText = TextDrawable(
      "No Song Playing",
      graphics::UV(0.5f, 0.0f, 0.0f, 10.0f),
      graphics::UV(1.0f, 0.0f, -40.0f, 30.0f),
      24
  ).setFont(font)
   .setMultiline(false)
   .setAlignment(TextDrawable::HorizontalAlignment::Center)
   .setVerticalAlignment(graphics::VerticalAlignment::TOP);

  // Artist name - medium size, below title
  artistNameText = TextDrawable(
      "Unknown Artist",
      graphics::UV(0.5f, 0.0f, 0.0f, 50.0f),
      graphics::UV(1.0f, 0.0f, -40.0f, 25.0f),
      18
  ).setFont(font)
   .setMultiline(false)
   .setAlignment(TextDrawable::HorizontalAlignment::Center)
   .setVerticalAlignment(graphics::VerticalAlignment::TOP);

  // Album name - smaller, below artist
  albumNameText = TextDrawable(
      "Unknown Album",
      graphics::UV(0.5f, 0.0f, 0.0f, 85.0f),
      graphics::UV(1.0f, 0.0f, -40.0f, 25.0f),
      14
  ).setFont(font)
   .setMultiline(false)
   .setAlignment(TextDrawable::HorizontalAlignment::Center)
   .setVerticalAlignment(graphics::VerticalAlignment::TOP);

  // Progress bar - positioned in lower third
  progressBar.setPosition(graphics::UV(0.0f, 1.0f, 20.0f, -65.0f));
  progressBar.setSize(graphics::UV(1.0f, 0.0f, -40.0f, 8.0f));

  // Position and duration text below progress bar
  songPositionText = TextDrawable(
      "0:00",
      graphics::UV(0.0f, 1.0f, 20.0f, -45.0f),
      graphics::UV(0.0f, 0.0f, 80.0f, 25.0f),
      14
  ).setFont(font)
   .setAlignment(TextDrawable::HorizontalAlignment::Left)
   .setVerticalAlignment(graphics::VerticalAlignment::TOP);

  songDurationText = TextDrawable(
      "0:00",
      graphics::UV(1.0f, 1.0f, -20.0f, -45.0f),
      graphics::UV(0.0f, 0.0f, 80.0f, 25.0f),
      14
  ).setFont(font)
   .setAlignment(TextDrawable::HorizontalAlignment::Right)
   .setVerticalAlignment(graphics::VerticalAlignment::TOP);

  // Add all elements as children of the main frame
  mainFrame.addChild(std::make_shared<TextDrawable>(songTitleText));
  mainFrame.addChild(std::make_shared<TextDrawable>(artistNameText));
  mainFrame.addChild(std::make_shared<TextDrawable>(albumNameText));
  mainFrame.addChild(std::make_shared<ProgressBarDrawable>(progressBar));
  mainFrame.addChild(std::make_shared<TextDrawable>(songPositionText));
  mainFrame.addChild(std::make_shared<TextDrawable>(songDurationText));
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
  // Draw the main frame, which will automatically draw all its children
  mainFrame.draw(context);
}

}; // namespace ui