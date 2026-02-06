#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"
#include <iostream>

#include "core/app_state.hpp"
#include "core/allegro_init.hpp"
#include "core/main_loop.hpp"
#include "database/library_scanner.hpp"

int main() {
  auto& appState = core::AppState::instance();

  std::cout << "Initializing application...\n";

  if (!core::initializeAllegro()) {
    std::cerr << "Failed to initialize Allegro.\n";
    return 1;
  }

  appState.init();

  std::cout << "App state initialized!\n";

  // Import songs from configured directory
  database::LibraryScanner scanner;
  std::string musicDir = appState.config.getMusicDirectory();
  auto result = scanner.scan(appState.db, musicDir);
  std::cout << "Scanned " << result.scanned << " files, imported " << result.imported << " songs, skipped " << result.skipped << " songs.\n";

  core::runMainLoop();

  std::cout << "Shutting down application...\n";
  appState.shutdown();
  std::cout << "Application shutdown complete.\n";

  return 0;
}