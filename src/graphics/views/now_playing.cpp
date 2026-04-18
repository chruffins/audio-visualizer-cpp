#include "graphics/views/now_playing.hpp"
#include "core/music_engine.hpp"
#include "util/duration.hpp"
#include <allegro5/allegro.h>

namespace ui {

NowPlayingView::NowPlayingView(std::shared_ptr<util::FontManager> fontManager, 
  std::shared_ptr<ui::ProgressBar> progressBarModel,
  graphics::EventDispatcher& eventDispatcher,
  core::MusicEngine* musicEngine)
    : fontManager(fontManager), musicEngine(musicEngine), 
      playPauseButton(graphics::UV(0.5f, 1.0f, -20.0f, -50.0f),
      graphics::UV(0.0f, 0.0f, 40.0f, 40.0f)),
      rewindButton(graphics::UV(0.5f, 1.0f, -70.0f, -50.0f),
      graphics::UV(0.0f, 0.0f, 40.0f, 40.0f)),
      skipButton(graphics::UV(0.5f, 1.0f, 30.0f, -50.0f),
      graphics::UV(0.0f, 0.0f, 40.0f, 40.0f)),
      loopButton(graphics::UV(0.5f, 1.0f, 80.0f, -50.0f),
      graphics::UV(0.0f, 0.0f, 40.0f, 40.0f)),
      mainFrame(std::make_shared<FrameDrawable>()),
        progressBar(progressBarModel),
        volumeSliderModel(std::make_shared<ui::Slider>(
          0.0f,
          100.0f,
          musicEngine ? (musicEngine->getGain() * 100.0f) : 100.0f
        )),
        volumeSlider(volumeSliderModel) {
  
  // Cache fonts (optimization)
  auto courierFont = fontManager->getFont("courier");
  auto kanitFont = fontManager->getFont("kanit");
  auto kanitLightFont = fontManager->getFont("kanitLight");
  auto gothicFont = fontManager->getFont("gothic");
  auto plexSansLightFont = fontManager->getFont("plexSansLight");
  m_fontTitle = fontManager->getFont("courier")->getFont(16);
  m_fontMetadata = fontManager->getFont("courier")->getFont(14);
  m_fontTime = fontManager->getFont("courier")->getFont(12);

  mainFrame->setPosition(graphics::UV(0.0f, 1.0f, 0.0f, -101.0f));
  mainFrame->setSize(graphics::UV(1.0f, 0.0f, 0.0f, 101.0f));
  mainFrame->setBackgroundColor(al_map_rgba(30, 30, 40, 220));
  mainFrame->setBorderColor(al_map_rgb(80, 80, 100));
  mainFrame->setBorderThickness(2);
  mainFrame->setPadding(15.0f);

  songTitleText = TextDrawable(
      "No Song Playing",
      graphics::UV(0.0f, 0.0f, 69.0f, 17.0f),
      graphics::UV(0.9f, 0.0f, 0.0f, 30.0f),
      16
  ).setFont(kanitFont)
   .setMultiline(false)
   .setAlignment(TextDrawable::HorizontalAlignment::Left)
   .setVerticalAlignment(graphics::VerticalAlignment::TOP);

  // Artist name - medium size, below title
  artistNameText = TextDrawable(
      "Unknown Artist",
      graphics::UV(0.0f, 0.0f, 69.0f, 32.0f),
      graphics::UV(0.9f, 0.0f, 0.0f, 25.0f),
      16
  ).setFont(kanitLightFont)
   .setMultiline(false)
   .setAlignment(TextDrawable::HorizontalAlignment::Left)
   .setVerticalAlignment(graphics::VerticalAlignment::TOP);

  // Album name - smaller, below artist
  albumNameText = TextDrawable(
      "Unknown Album",
      graphics::UV(0.0f, 0.0f, 64.0f, 60.0f),
      graphics::UV(0.9f, 0.0f, 0.0f, 25.0f),
      16
  ).setFont(kanitFont)
   .setMultiline(false)
   .setAlignment(TextDrawable::HorizontalAlignment::Left)
   .setVerticalAlignment(graphics::VerticalAlignment::TOP)
   .setVisible(false); // uhh dont need this anymore i guess

  // Progress bar - positioned in lower third
  progressBar.setPosition(graphics::UV(0.3f, 1.0f, 10.0f, 0.0f));
  progressBar.setSize(graphics::UV(0.4f, 0.0f, 0.0f, 8.0f));

  // Album art image - left side
  albumArtImage.setPosition(graphics::UV(0.0f, 0.0f, 0.0f, 0.0f))
  .setSize(graphics::UV(0.0f, 0.0f, 64.0f, 64.0f))
  .setScaleMode(ImageDrawable::ScaleMode::STRETCH);

  // Position and duration text below progress bar
  songPositionText = TextDrawable(
      "0:00",
      graphics::UV(0.3f, 1.0f, -0.0f, -10.0f),
      graphics::UV(0.0f, 0.0f, 80.0f, 25.0f),
      12
  ).setFont(plexSansLightFont)
   .setAlignment(TextDrawable::HorizontalAlignment::Right)
   .setVerticalAlignment(graphics::VerticalAlignment::CENTER);

  songDurationText = TextDrawable(
      "0:00",
      graphics::UV(0.7f, 1.0f, 20.0f, -10.0f),
      graphics::UV(0.0f, 0.0f, 80.0f, 25.0f),
      12
  ).setFont(plexSansLightFont)
   .setAlignment(TextDrawable::HorizontalAlignment::Left)
   .setVerticalAlignment(graphics::VerticalAlignment::CENTER);

  // Volume slider (0-100 mapped to engine gain 0.0-1.0)
  volumeSlider.setPosition(graphics::UV(1.0f, 1.0f, -160.0f, -7.0f));
  volumeSlider.setSize(graphics::UV(0.0f, 0.0f, 140.0f, 20.0f));
  volumeSlider.setOnValueChanged([this](float value) {
    if (this->musicEngine) {
      this->musicEngine->setGain(value / 100.0f);
    }
  });

  // Setup play/pause button
  playPauseButton.loadImageFromFile("../assets/icons/play.png");
  playPauseButton.setDrawBorder(false);
  playPauseButton.setDrawBackground(true);
  playPauseButton.setBackgroundColors(
    al_map_rgb(60, 60, 80),   // normal
    al_map_rgb(80, 80, 100),  // hover
    al_map_rgb(100, 100, 120), // pressed
    al_map_rgb(50, 50, 60)    // disabled
  );
  playPauseButton.setImageScaleMode(ImageDrawable::ScaleMode::FIT);
  playPauseButton.setImagePadding(4.0f);
  playPauseButton.setOnClick([this]() {
    if (this->musicEngine) {
      if (this->musicEngine->isPlaying()) {
        this->musicEngine->pauseSound();
      } else {
        this->musicEngine->resumeSound();
      }
      updatePlayPauseButton();
    }
  });
  
  // Setup rewind button
  rewindButton.loadImageFromFile("../assets/icons/skip_prev.png");
  rewindButton.setDrawBorder(false);
  rewindButton.setDrawBackground(true);
  rewindButton.setBackgroundColors(
    al_map_rgb(60, 60, 80),   // normal
    al_map_rgb(80, 80, 100),  // hover
    al_map_rgb(100, 100, 120), // pressed
    al_map_rgb(50, 50, 60)    // disabled
  );
  rewindButton.setImageScaleMode(ImageDrawable::ScaleMode::FIT);
  rewindButton.setImagePadding(4.0f);
  rewindButton.setOnClick([this]() {
    if (this->musicEngine) {
      // smarter rewind
      constexpr double REWIND_THRESHOLD = 3.0; // seconds
      if (this->musicEngine->getCurrentTime() < REWIND_THRESHOLD) {
        this->musicEngine->playPrevious();
      } else {
        this->musicEngine->setProgress(0.0);
      }
    }
  });
  
  // Setup skip button
  skipButton.loadImageFromFile("../assets/icons/skip_next.png");
  skipButton.setDrawBorder(false);
  skipButton.setDrawBackground(true);
  skipButton.setBackgroundColors(
    al_map_rgb(60, 60, 80),   // normal
    al_map_rgb(80, 80, 100),  // hover
    al_map_rgb(100, 100, 120), // pressed
    al_map_rgb(50, 50, 60)    // disabled
  );
  skipButton.setImageScaleMode(ImageDrawable::ScaleMode::FIT);
  skipButton.setImagePadding(4.0f);
  skipButton.setOnClick([this]() {
    if (this->musicEngine) {
      this->musicEngine->playNext();
    }
  });
  
  // Setup loop button
  loopButton.loadImageFromFile("../assets/icons/loop.png");
  loopButton.setDrawBorder(false);
  loopButton.setDrawBackground(true);
  loopButton.setBackgroundColors(
    al_map_rgb(60, 60, 80),   // normal
    al_map_rgb(80, 80, 100),  // hover
    al_map_rgb(100, 100, 120), // pressed
    al_map_rgb(50, 50, 60)    // disabled
  );
  loopButton.setImageScaleMode(ImageDrawable::ScaleMode::FIT);
  loopButton.setImagePadding(4.0f);
  loopButton.setOnClick([this]() {
    if (this->musicEngine) {
      this->musicEngine->setRepeat(!this->musicEngine->isRepeating());
      updateLoopButton();
    }
  });
  
  // Add all elements as children of the main frame
  mainFrame->addChild(&songTitleText);
  mainFrame->addChild(&artistNameText);
  mainFrame->addChild(&albumNameText);
  mainFrame->addChild(&progressBar);
  mainFrame->addChild(&songPositionText);
  mainFrame->addChild(&songDurationText);
  mainFrame->addChild(&albumArtImage);
  mainFrame->addChild(&rewindButton);
  mainFrame->addChild(&playPauseButton);
  mainFrame->addChild(&skipButton);
  mainFrame->addChild(&loopButton);
  mainFrame->addChild(&volumeSlider);
  
  // Register button for events
  eventDispatcher.addEventTarget(mainFrame);
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

void NowPlayingView::setAlbumArt(ALLEGRO_BITMAP *bitmap) {
  albumArtImage.setBitmap(bitmap);
}

void NowPlayingView::setBounds(const graphics::UV& position, const graphics::UV& size) {
  mainFrame->setPosition(position);
  mainFrame->setSize(size);
  lastDisplayWidth = 0;
  lastDisplayHeight = 0;
}

void NowPlayingView::recalculateLayout(const graphics::RenderContext& context) {
  // Only recalculate if display size has changed
  if (context.screenWidth == lastDisplayWidth && context.screenHeight == lastDisplayHeight) {
    return;
  }
  lastDisplayWidth = context.screenWidth;
  lastDisplayHeight = context.screenHeight;
}

void NowPlayingView::updatePlayPauseButton() {
  if (musicEngine) {
    playPauseButton.loadImageFromFile(musicEngine->isPlaying() ? "../assets/icons/pause.png" : "../assets/icons/play.png");
  }
}

void NowPlayingView::updateLoopButton() {
  if (musicEngine) {
    // Swap icon based on repeat state
    if (musicEngine->isRepeating()) {
      loopButton.setImageTint(al_map_rgb(100, 200, 100)); // green tint when active
    } else {
      loopButton.setImageTint(al_map_rgb(255, 255, 255)); // normal tint when inactive
    }
  }
}

void NowPlayingView::draw(const graphics::RenderContext& context) {
  // Recalculate layout if display size has changed (responsive font sizing)
  recalculateLayout(context);
  // Update button states
  updatePlayPauseButton();
  updateLoopButton();
  // Draw the main frame, which will automatically draw all its children
  mainFrame->draw(context);
}

}; // namespace ui