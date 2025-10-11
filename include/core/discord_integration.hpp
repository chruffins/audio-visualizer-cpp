#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <iostream>
#include "discordpp.h"
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

    void set_new_song(std::string &title, std::string &artist);

    void run_callbacks() {
		discordpp::RunCallbacks();
	}

	void update_presence(std::string state);

    std::shared_ptr<discordpp::Client> client;

private:
	DiscordIntegration() = default;
	~DiscordIntegration() = default;

	const uint64_t APPLICATION_ID = 1362371550275174512;
};

} // namespace core
