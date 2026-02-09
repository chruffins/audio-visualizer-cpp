#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <iostream>
#include "discordpp.h"

namespace music {
	class SongView;
}

// Forward declaration for Allegro timer
struct ALLEGRO_TIMER;

namespace core {

class DiscordIntegration {
public:
	// Retrieve the singleton instance (Meyers singleton - thread safe in C++11+)
	static DiscordIntegration& instance() {
		static DiscordIntegration s;
		return s;
	}

	// Disallow copy/move
	DiscordIntegration(const DiscordIntegration&) = delete;
	DiscordIntegration& operator=(const DiscordIntegration&) = delete;
	DiscordIntegration(DiscordIntegration&&) = delete;
	DiscordIntegration& operator=(DiscordIntegration&&) = delete;

	// Initialize application-wide state. Safe to call multiple times; returns true
	bool init();

	bool authorize();

	// Shutdown/cleanup. Safe to call multiple times.
	void shutdown();

	void setSongPresence(const music::SongView& song);

    void run_callbacks() {
		discordpp::RunCallbacks();
	}

	void update_presence(std::string state);

	// Inject the callback timer so Discord integration can stop it when ready
	void setCallbackTimer(ALLEGRO_TIMER* timer) {
		callback_timer = timer;
	}

    std::shared_ptr<discordpp::Client> client;

private:
	DiscordIntegration() = default;
	~DiscordIntegration() = default;

	const uint64_t APPLICATION_ID = 1362371550275174512;
	std::chrono::steady_clock::time_point last_update_time = std::chrono::steady_clock::now();
	ALLEGRO_TIMER* callback_timer = nullptr;
};

} // namespace core
