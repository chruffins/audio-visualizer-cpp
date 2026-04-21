#include "util/config.hpp"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <limits.h>
#include <unistd.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>

namespace util {

namespace {

constexpr const char* kAppName = "audiovis";

std::string getEnvOrEmpty(const char* name) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

std::filesystem::path getHomeDir() {
    const std::string home = getEnvOrEmpty("HOME");
    if (!home.empty()) {
        return std::filesystem::path(home);
    }
    return std::filesystem::current_path();
}

std::filesystem::path getExecutableDir() {
    char exePath[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len > 0) {
        exePath[len] = '\0';
        return std::filesystem::path(exePath).parent_path();
    }
    return std::filesystem::current_path();
}

std::filesystem::path appImagePath() {
    const std::string appImage = getEnvOrEmpty("APPIMAGE");
    return appImage.empty() ? std::filesystem::path() : std::filesystem::path(appImage);
}

std::filesystem::path portableConfigHome() {
    const auto appImage = appImagePath();
    if (appImage.empty()) {
        return {};
    }

    const auto configHome = std::filesystem::path(appImage.string() + ".config");
    if (std::filesystem::is_directory(configHome)) {
        return configHome;
    }
    return {};
}

std::filesystem::path portableHome() {
    const auto appImage = appImagePath();
    if (appImage.empty()) {
        return {};
    }

    const auto home = std::filesystem::path(appImage.string() + ".home");
    if (std::filesystem::is_directory(home)) {
        return home;
    }
    return {};
}

std::filesystem::path configHomeDir() {
    const auto portable = portableConfigHome();
    if (!portable.empty()) {
        return portable;
    }

    const std::string xdg = getEnvOrEmpty("XDG_CONFIG_HOME");
    if (!xdg.empty()) {
        return std::filesystem::path(xdg);
    }
    return getHomeDir() / ".config";
}

std::filesystem::path dataHomeDir() {
    const auto portable = portableHome();
    if (!portable.empty()) {
        return portable / ".local" / "share";
    }

    const std::string xdg = getEnvOrEmpty("XDG_DATA_HOME");
    if (!xdg.empty()) {
        return std::filesystem::path(xdg);
    }
    return getHomeDir() / ".local" / "share";
}

std::filesystem::path cacheHomeDir() {
    const auto portable = portableHome();
    if (!portable.empty()) {
        return portable / ".cache";
    }

    const std::string xdg = getEnvOrEmpty("XDG_CACHE_HOME");
    if (!xdg.empty()) {
        return std::filesystem::path(xdg);
    }
    return getHomeDir() / ".cache";
}

std::filesystem::path ensureDir(const std::filesystem::path& p) {
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    return p;
}

bool migrateLegacyFileIfMissing(const std::string& legacyName, const std::filesystem::path& destination) {
    std::error_code ec;
    if (std::filesystem::exists(destination, ec)) {
        return false;
    }

    const std::filesystem::path legacyPath = std::filesystem::current_path() / legacyName;
    if (!std::filesystem::exists(legacyPath, ec)) {
        return false;
    }

    std::filesystem::create_directories(destination.parent_path(), ec);
    if (ec) {
        return false;
    }

    std::filesystem::copy_file(legacyPath, destination, std::filesystem::copy_options::skip_existing, ec);
    if (!ec) {
        std::cout << "Migrated legacy file from " << legacyPath << " to " << destination << "\n";
        return true;
    }

    return false;
}

std::string normalizeAssetRelativePath(std::string relativePath) {
    if (relativePath.rfind("../assets/", 0) == 0) {
        relativePath.erase(0, 10);
    } else if (relativePath.rfind("assets/", 0) == 0) {
        relativePath.erase(0, 7);
    }
    return relativePath;
}

std::vector<std::filesystem::path> assetRoots() {
    std::vector<std::filesystem::path> roots;

    const std::string overrideDir = getEnvOrEmpty("AUDIOVIS_ASSET_DIR");
    if (!overrideDir.empty()) {
        roots.emplace_back(overrideDir);
    }

    const auto exeDir = getExecutableDir();
    roots.emplace_back(exeDir / "../share/audiovis/assets");
    roots.emplace_back(exeDir / "../assets");
    roots.emplace_back(exeDir / "assets");
    roots.emplace_back("/usr/share/audiovis/assets");

    return roots;
}

} // namespace

Config::~Config() {
    if (config) {
        al_destroy_config(config);
    }
}

std::string Config::getConfigDir() {
    return ensureDir(configHomeDir() / kAppName).string();
}

std::string Config::getDataDir() {
    return ensureDir(dataHomeDir() / kAppName).string();
}

std::string Config::getCacheDir() {
    return ensureDir(cacheHomeDir() / kAppName).string();
}

std::string Config::getConfigPath() {
    const std::filesystem::path path = std::filesystem::path(getConfigDir()) / "config.ini";
    migrateLegacyFileIfMissing("config.ini", path);
    return path.string();
}

std::string Config::getDatabasePath() {
    const std::filesystem::path path = std::filesystem::path(getDataDir()) / "music.db";
    migrateLegacyFileIfMissing("music.db", path);
    return path.string();
}

std::string Config::getDiscordTokenPath() {
    const std::filesystem::path path = std::filesystem::path(getConfigDir()) / "discord_tokens.txt";
    migrateLegacyFileIfMissing("discord_tokens.txt", path);
    return path.string();
}

std::string Config::getAssetRoot() {
    for (const auto& root : assetRoots()) {
        std::error_code ec;
        if (std::filesystem::is_directory(root, ec)) {
            return std::filesystem::weakly_canonical(root, ec).string();
        }
    }
    return (getExecutableDir() / "../assets").string();
}

std::string Config::resolveAssetPath(const std::string& relativePath) {
    const std::filesystem::path normalizedRelative = normalizeAssetRelativePath(relativePath);
    for (const auto& root : assetRoots()) {
        const auto candidate = root / normalizedRelative;
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate.string();
        }
    }
    return (std::filesystem::path(getAssetRoot()) / normalizedRelative).string();
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
    al_set_config_value(defaultConfig, "paths", "music_directory", (getHomeDir() / "Music").string().c_str());
    al_set_config_value(defaultConfig, "paths", "icon_path", "logotransparent.png");
    
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
    std::string iconPath = getString("paths", "icon_path", "logotransparent.png");
    if (iconPath.empty()) {
        iconPath = "logotransparent.png";
    }

    const std::filesystem::path asPath(iconPath);
    if (asPath.is_absolute()) {
        return iconPath;
    }

    return resolveAssetPath(iconPath);
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
