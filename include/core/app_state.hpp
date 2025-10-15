// AppState: simple application-wide singleton to hold global state.
// Header-only singleton skeleton. Thread-safe (C++11 magic statics) and
// non-copyable/non-movable.

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <sqlite3.h>
#include "discord_integration.hpp"
#include "music_engine.hpp"
#include "database/database.hpp"
#include "music/library.hpp"

namespace core {

class AppState {
public:
	// Retrieve the singleton instance (Meyers singleton - thread safe in C++11+)
	static AppState& instance() {
		static AppState s;
		return s;
	}

	// Disallow copy/move
	AppState(const AppState&) = delete;
	AppState& operator=(const AppState&) = delete;
	AppState(AppState&&) = delete;
	AppState& operator=(AppState&&) = delete;

	// Initialize application-wide state. Safe to call multiple times; returns true
	bool init();

	// Shutdown/cleanup. Safe to call multiple times.
	void shutdown();

    // Some really basic Allegro resources
    ALLEGRO_DISPLAY* display;
    ALLEGRO_FONT* default_font;
	ALLEGRO_EVENT_QUEUE* event_queue;
	ALLEGRO_TIMER* graphics_timer;
	ALLEGRO_TIMER* discord_callback_timer;
	ALLEGRO_EVENT event;

    // Database
    database::MusicDatabase db;

	// Discord
	std::atomic<bool> discord_initialized;
	DiscordIntegration& discord_integration;

	// Music engine
	MusicEngine music_engine;

	// Library (in-memory)
	music::Library library;

private:
	AppState()
		: display(nullptr)
		, default_font(nullptr)
		, db("music.db")
		, discord_initialized(false)
		, discord_integration(DiscordIntegration::instance())
		, music_engine()
		, library() {}

	~AppState() = default;
};

} // namespace core
