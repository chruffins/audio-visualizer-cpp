#include "util/config.hpp"
#include <iostream>
#include <algorithm>
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>

namespace util {

Config::~Config() {
    if (config) {
        al_destroy_config(config);
    }
}

bool Config::generateDefaultConfig(const std::string& filename) {
    // Check if file already exists
    ALLEGRO_FILE* file = al_fopen(filename.c_str(), "r");
    if (file) {
        al_fclose(file);
        std::cout << "Config file already exists: " << filename << "\n";
        return true;
    }

    // Create a new config
    ALLEGRO_CONFIG* defaultConfig = al_create_config();
    if (!defaultConfig) {
        std::cerr << "Failed to create default config\n";
        return false;
    }

    // Set default values
    al_set_config_value(defaultConfig, "paths", "music_directory", "/home/chris/Music");
    al_set_config_value(defaultConfig, "paths", "icon_path", "../assets/logotransparent.png");
    
    al_set_config_value(defaultConfig, "display", "width", "1024");
    al_set_config_value(defaultConfig, "display", "height", "300");
    
    al_set_config_value(defaultConfig, "discord", "application_id", "0");
    al_set_config_value(defaultConfig, "audio", "volume_percent", "100");

    // Save the config file
    bool success = al_save_config_file(filename.c_str(), defaultConfig);
    al_destroy_config(defaultConfig);

    if (success) {
        std::cout << "Generated default config file: " << filename << "\n";
    } else {
        std::cerr << "Failed to save default config file: " << filename << "\n";
    }

    return success;
}

bool Config::load(const std::string& filename) {
    // Try to generate default config if it doesn't exist
    if (!al_filename_exists(filename.c_str())) {
        std::cout << "Config file not found, generating default...\n";
        generateDefaultConfig(filename);
    }

    config = al_load_config_file(filename.c_str());
    if (!config) {
        std::cerr << "Failed to load config file: " << filename << "\n";
        return false;
    }
    loadedFilename = filename;
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

void Config::setString(const std::string& section, const std::string& key, const std::string& value) {
    if (!config) return;
    al_set_config_value(config, section.c_str(), key.c_str(), value.c_str());
}

void Config::setInt(const std::string& section, const std::string& key, int value) {
    setString(section, key, std::to_string(value));
}

bool Config::save() const {
    if (!config || loadedFilename.empty()) {
        return false;
    }
    return al_save_config_file(loadedFilename.c_str(), config);
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

int Config::getVolumePercent() const {
    const int value = getInt("audio", "volume_percent", 100);
    return std::clamp(value, 0, 100);
}

void Config::setVolumePercent(int percent) {
    setInt("audio", "volume_percent", std::clamp(percent, 0, 100));
}

} // namespace util
