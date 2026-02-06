#pragma once

namespace core {

/**
 * Initialize all Allegro subsystems and addons.
 * Must be called before any other Allegro functions.
 * Returns true on success, false on failure.
 */
bool initializeAllegro();

} // namespace core
