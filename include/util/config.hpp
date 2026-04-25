#pragma once

#include <string>
#include <cstdint>

struct ALLEGRO_CONFIG;

namespace util {

class Config {
public:
    Config() : config(nullptr) {}
    ~Config();

    // Global path policy for runtime files and packaged assets.
    static std::string getConfigDir();
    static std::string getDataDir();
    static std::string getCacheDir();
    static std::string getConfigPath();
    static std::string getDatabasePath();
    static std::string getDiscordTokenPath();
    static std::string getAssetRoot();
    static std::string resolveAssetPath(const std::string& relativePath);

    bool load(const std::string& filename);
    bool generateDefaultConfig(const std::string& filename);
    
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    void setString(const std::string& section, const std::string& key, const std::string& value);
    void setInt(const std::string& section, const std::string& key, int value);
    bool save() const;
    
    // Convenience methods for common config values
    std::string getMusicDirectory() const;
    std::string getIconPath() const;
    uint64_t getDiscordApplicationId() const;
    int getDisplayWidth() const;
    int getDisplayHeight() const;
    int getVolumePercent() const;
    void setVolumePercent(int percent);

private:
    ALLEGRO_CONFIG* config;
    std::string loadedFilename;
    
    // Disallow copy/move
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;
};

} // namespace util
