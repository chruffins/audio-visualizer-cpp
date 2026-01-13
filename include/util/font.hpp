#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include<map>
#include<string>
#include<memory>

// Utility function to load a TTF font with error handling
namespace util {
class Font {
public:
    Font() {
        filename = "";
    }

    Font(std::string filename) : filename(std::move(filename)) {}
    ~Font() {
        cleanup();
    }

    void cleanup() {
        if (is_shutdown) {
            return; // Already cleaned up
        }
        for (auto& pair : fonts) {
            if (pair.second) {
                al_destroy_font(pair.second);
            }
        }
        fonts.clear();
        is_shutdown = true;
    }

    ALLEGRO_FONT* getFont(int size) {
        if (filename == "") return getFallbackFont();

        auto it = fonts.find(size);
        if (it != fonts.end()) {
            return it->second; // Return cached font
        }

        ALLEGRO_FONT* font = al_load_ttf_font(filename.c_str(), size, 0);
        if (!font) {
            // Handle error: font loading failed
            fprintf(stderr, "Failed to load font: %s\n", filename.c_str());
            return nullptr;
        }

        fonts[size] = font; // Cache the loaded font
        return font;
    }

    ALLEGRO_FONT* getFallbackFont() {
        return al_create_builtin_font();
    }
private:
    std::string filename;
    std::map<int, ALLEGRO_FONT*> fonts;
    bool is_shutdown = false;
};

class FontManager {
public:
    FontManager() {
        defaultFont = std::make_shared<Font>();
    }

    std::shared_ptr<Font> getFont(const std::string& name) {
        auto it = fonts.find(name);
        if (it != fonts.end()) {
            return it->second;
        }
        return defaultFont;
    }

    std::shared_ptr<Font> getDefaultFont() {
        return defaultFont;
    }

    std::shared_ptr<Font> loadFont(const std::string& name, const std::string& filename) {
        auto font = std::make_shared<Font>(filename);
        fonts[name] = font;
        return font;
    }

    void cleanup() {
        for (auto& pair : fonts) {
            if (pair.second) {
                pair.second->cleanup();
            }
        }
        if (defaultFont) {
            defaultFont->cleanup();
        }
        fonts.clear();
    }

private:
    std::map<std::string, std::shared_ptr<Font>> fonts;
    std::shared_ptr<Font> defaultFont;
};
}; // namespace util