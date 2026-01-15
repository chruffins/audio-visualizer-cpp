#include "graphics/views/now_playing.hpp"
#include "util/duration.hpp"
#include <allegro5/allegro.h>
#include <algorithm>

namespace ui {

NowPlayingView::NowPlayingView(std::shared_ptr<util::FontManager> fontManager, std::shared_ptr<ui::ProgressBar> progressBarModel)
    : fontManager(fontManager), progressBar(progressBarModel) {
  
  auto font = fontManager->getFont("courier");

  mainFrame.setPosition(graphics::UV(0.0f, 1.0f, 0.0f, -101.0f));
  mainFrame.setSize(graphics::UV(1.0f, 0.0f, 0.0f, 101.0f));
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
  mainFrame.addChild(&songTitleText);
  mainFrame.addChild(&artistNameText);
  mainFrame.addChild(&albumNameText);
  mainFrame.addChild(&progressBar);
  mainFrame.addChild(&songPositionText);
  mainFrame.addChild(&songDurationText);
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

void NowPlayingView::recalculateLayout(const graphics::RenderContext& context) {
  // Only recalculate if display size has changed
  if (context.screenWidth == lastDisplayWidth && context.screenHeight == lastDisplayHeight) {
    return;
  }
  lastDisplayWidth = context.screenWidth;
  lastDisplayHeight = context.screenHeight;

  // Scale font sizes responsively based on display height
  // Title: ~6-8% of display height, clamped to reasonable range
  int titleFontSize = static_cast<int>(std::clamp(context.screenHeight * 0.08f, 16.0f, 48.0f));
  // Artist: ~4-5% of display height
  int artistFontSize = static_cast<int>(std::clamp(context.screenHeight * 0.08f, 12.0f, 32.0f));
  // Album: ~3-4% of display height
  int albumFontSize = static_cast<int>(std::clamp(context.screenHeight * 0.04f, 10.0f, 24.0f));
  // Time labels: ~2-3% of display height
  int timeFontSize = static_cast<int>(std::clamp(context.screenHeight * 0.05f, 8.0f, 18.0f));

  // Update font sizes
  songTitleText.setFontSize(titleFontSize);
  artistNameText.setFontSize(artistFontSize);
  albumNameText.setFontSize(albumFontSize);
  songPositionText.setFontSize(timeFontSize);
  songDurationText.setFontSize(timeFontSize);

  // Scale padding based on display width (2% of width, clamped)
  float scaledPadding = std::clamp(context.screenWidth * 0.02f, 10.0f, 40.0f);
  mainFrame.setPadding(scaledPadding);
}

void NowPlayingView::draw(const graphics::RenderContext& context) {
  // Recalculate layout if display size has changed (responsive font sizing)
  recalculateLayout(context);
  // Draw the main frame, which will automatically draw all its children
  mainFrame.draw(context);
}

}; // namespace ui