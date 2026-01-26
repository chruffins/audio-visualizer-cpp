#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <csignal>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_acodec.h>

#include "../include/core/app_state.hpp"
#include "../include/core/discord_integration.hpp"
#include "database/library_scanner.hpp"
#include "graphics/views/now_playing.hpp"
#include "graphics/uv.hpp"
#include "graphics/views/album_list.hpp"

// Create a flag to stop the application
std::atomic<bool> running = true;

// Signal handler to stop the application
void signalHandler(int signum) {
  running.store(false);
}

void run_main_loop() {
  auto& appState = core::AppState::instance();
  bool finished = false;

  auto nowPlayingView = ui::NowPlayingView(appState.fontManager, appState.music_engine.progressBarModel, appState.event_dispatcher, &appState.music_engine);
  auto albumListView = ui::AlbumListView(appState.fontManager, appState.library, appState.event_dispatcher);

  // Register a callback so that the NowPlayingView updates automatically when
  // MusicEngine starts a new song (e.g., via playNext or other engine-driven
  // playback). This keeps the UI in sync without polling.
  appState.music_engine.onSongChanged = [&](const music::SongView& s) {
    nowPlayingView.setSongTitle(s.title);
    nowPlayingView.setArtistName(s.artist);
    nowPlayingView.setAlbumName(s.album);

    const music::Album* album = appState.library->getAlbumById(s.album_id);
    if (album) {
      auto albumArt = album->getAlbumArt();
      nowPlayingView.setAlbumArt(albumArt);
      if (!albumArt) {
        std::cerr << "Warning: Failed to load album art for album: " << album->title << "\n";
      }
    }
    nowPlayingView.setDuration(s.duration);
    nowPlayingView.setPosition(0);
    appState.discord_integration.setSongPresence(s);
  };

  // Register a callback to automatically play the next song when current finishes
  appState.music_engine.onSongFinished = [&]() {
    std::cout << "Song finished, playing next...\n";
    appState.music_engine.playNext();
  };

  // Example: Play a sound file (make sure the path is correct)
  std::cout << "Starting playback...\n";

  // lets get a random song now (SongView)
  const music::SongView* song = appState.library->getRandomSong();
  if (song) {
    std::cout << "Playing random song: " << song->title << "\n";
    // Look up the concrete Song for filename using ID from SongView
    if (const music::SongView* songModel = appState.library->getSongById(song->id)) {
      appState.music_engine.playSound(songModel->filename);
    }
  } else {
    std::cerr << "No songs in library.\n";
  }

  // Cache display dimensions for render context (avoids recalculating every frame)
  graphics::RenderContext globalContext{};
  globalContext.screenWidth = al_get_display_width(al_get_current_display());
  globalContext.screenHeight = al_get_display_height(al_get_current_display());

  std::cout << "Starting graphics...\n";
  while (!finished) {
    al_wait_for_event(appState.event_queue, &appState.event);

    switch (appState.event.type)
    {
    case ALLEGRO_EVENT_AUDIO_STREAM_FINISHED:
      // TODO: expose currentStream so that we can verify it's the current stream
      if (appState.music_engine.onSongFinished) {
        appState.music_engine.onSongFinished();
      }
      break;
    case ALLEGRO_EVENT_DISPLAY_CLOSE:
      finished = true;
      break;
    case ALLEGRO_EVENT_TIMER:
      if (appState.event.timer.source == appState.graphics_timer && al_is_event_queue_empty(appState.event_queue)) {
        appState.music_engine.update(); // Update music engine (progress bar, etc.)
        nowPlayingView.setPosition(appState.music_engine.getCurrentTime());

        // Render frame
        al_clear_to_color(al_map_rgb(0, 0, 0));
        //al_hold_bitmap_drawing(true);
        //al_draw_text(appState.default_font, al_map_rgb(255, 255, 255), 200, 150, ALLEGRO_ALIGN_CENTRE, "Hello, Allegro!");

        nowPlayingView.draw(globalContext);
        albumListView.draw(globalContext);
        //al_hold_bitmap_drawing(false);
        al_flip_display();
      } else if (appState.event.timer.source == appState.discord_callback_timer) {
        // Handle Discord callback timer event
        if (!appState.discord_initialized.load()) {
          if (appState.discord_integration.init()) {
            appState.discord_initialized.store(true);
            std::cout << "✅ Discord SDK initialized successfully.\n";
            appState.discord_integration.authorize();
          } else {
            std::cerr << "❌ Failed to initialize Discord SDK.\n";
          }
        }
        appState.discord_integration.run_callbacks();
      }
      break;
    case ALLEGRO_EVENT_KEY_DOWN:
    case ALLEGRO_EVENT_KEY_UP:
    case ALLEGRO_EVENT_KEY_CHAR:
    case ALLEGRO_EVENT_MOUSE_AXES:
    case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
    case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
      appState.event_dispatcher.dispatchEvent(appState.event);
      break;
      /*
      if (appState.event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
        finished = true;
      } else if (appState.event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
        if (appState.music_engine.isPlaying()) {
          appState.music_engine.pauseSound();
        } else {
          appState.music_engine.resumeSound();
        }
      } else if (appState.event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
        // Play random song (onSongChanged callback handles UI updates)
        appState.music_engine.playNext();
        /*
        song = appState.library->getRandomSong();
        if (song) {
          if (const music::SongView* songModel = appState.library->getSongById(song->id)) {
            appState.music_engine.playSound(songModel->filename);
          }
        }
          
      }
      */
    default:
      break;
    }
  }
}

int main() {
  auto& appState = core::AppState::instance();

  std::cout << "Initializing application...\n";

  al_init();
  al_install_mouse();
  al_install_keyboard();
  al_install_audio();

  al_init_font_addon();
  al_init_ttf_addon();

  al_reserve_samples(16);
  al_init_acodec_addon();
  al_init_image_addon();
  al_init_primitives_addon();

  appState.init();

  std::cout << "App state initialized!\n";

  // Import songs from configured directory
  database::LibraryScanner scanner;
  std::string musicDir = appState.config.getMusicDirectory();
  auto result = scanner.scan(appState.db, musicDir);
  std::cout << "Scanned " << result.scanned << " files, imported " << result.imported << " songs, skipped " << result.skipped << " songs.\n";

  run_main_loop();

  std::cout << "Shutting down application...\n";
  appState.shutdown();
  std::cout << "Application shutdown complete.\n";

  // Shutdown Allegro addons in reverse order of initialization
  /*
  al_shutdown_primitives_addon();
  al_shutdown_image_addon();
  al_shutdown_ttf_addon();
  al_shutdown_font_addon();
  al_uninstall_audio();
  al_uninstall_keyboard();
  al_uninstall_mouse();
  */

  return 0;
}