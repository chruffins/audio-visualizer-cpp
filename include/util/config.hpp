#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <string>
#include <cstdint>

namespace util {

class Config {
public:
    Config() : config(nullptr) {}
    ~Config();

    bool load(const std::string& filename);
    
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    
    // Convenience methods for common config values
    std::string getMusicDirectory() const;
    std::string getIconPath() const;
    uint64_t getDiscordApplicationId() const;
    int getDisplayWidth() const;
    int getDisplayHeight() const;

private:
    ALLEGRO_CONFIG* config;
    
    // Disallow copy/move
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;
};

} // namespace util
