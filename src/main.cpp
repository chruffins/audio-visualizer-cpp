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

#include "../include/core/app_state.hpp"
#include "../include/core/discord_integration.hpp"
#include "database/library_scanner.hpp"

// Replace with your Discord Application ID
const uint64_t APPLICATION_ID = 123456789012345678;

// Create a flag to stop the application
std::atomic<bool> running = true;

// Signal handler to stop the application
void signalHandler(int signum) {
  running.store(false);
}

void run_main_loop() {
  auto& appState = core::AppState::instance();
  bool finished = false;

  appState.music_engine.playSound("C:\\Users\\chris\\Music\\downloaded\\complete\\GotMedieval\\[2001] Humanistic\\01 - The Remedy.mp3");
 
  while (!finished) {
    al_wait_for_event(appState.event_queue, &appState.event);

    switch (appState.event.type)
    {
    case ALLEGRO_EVENT_DISPLAY_CLOSE:
      finished = true;
      break;
    case ALLEGRO_EVENT_TIMER:
      if (appState.event.timer.source == appState.graphics_timer) {
        // Handle graphics timer event
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_draw_text(appState.default_font, al_map_rgb(255, 255, 255), 200, 150, ALLEGRO_ALIGN_CENTRE, "Hello, Allegro!");
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

  al_reserve_samples(16);
  al_init_acodec_addon();

  appState.init();

  std::cout << "App state initialized!\n";

  // Import songs from a directory
  database::LibraryScanner scanner;
  auto result = scanner.scan(appState.db, "C:\\Users\\chris\\Music");
  std::cout << "Scanned " << result.scanned << " files, imported " << result.imported << " songs, skipped " << result.skipped << " songs.\n";

  run_main_loop();

  appState.shutdown();

  return 0;
}