#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <csignal>

#include <allegro5/allegro.h>

#include "../include/core/app_state.hpp"
#include "../include/core/discord_integration.hpp"

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
  al_init_font_addon();

  appState.init();

  std::cout << "App state initialized!\n";

  run_main_loop();

  appState.shutdown();

  return 0;
}