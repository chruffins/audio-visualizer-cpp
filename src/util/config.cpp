#include "util/config.hpp"
#include <iostream>

namespace util {

Config::~Config() {
    if (config) {
        al_destroy_config(config);
    }
}

bool Config::load(const std::string& filename) {
    config = al_load_config_file(filename.c_str());
    if (!config) {
        std::cerr << "Failed to load config file: " << filename << "\n";
        return false;
    }
    return true;
}

std::string Config::getString(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    if (!config) return defaultValue;
    const char* value = al_get_config_value(config, section.c_str(), key.c_str());
    return value ? std::string(value) : defaultValue;
}

int Config::getInt(const std::string& section, const std::string& key, int defaultValue) const {
    if (!config) return defaultValue;
    const char* value = al_get_config_value(config, section.c_str(), key.c_str());
    if (!value) return defaultValue;
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

std::string Config::getMusicDirectory() const {
    return getString("paths", "music_directory", "/home/chris/Music");
}

std::string Config::getIconPath() const {
    return getString("paths", "icon_path", "../assets/logotransparent.png");
}

uint64_t Config::getDiscordApplicationId() const {
    if (!config) return 0;
    const char* value = al_get_config_value(config, "discord", "application_id");
    return value ? std::stoull(value) : 0;
}

int Config::getDisplayWidth() const {
    return getInt("display", "width", 800);
}

int Config::getDisplayHeight() const {
    return getInt("display", "height", 300);
}

} // namespace util
