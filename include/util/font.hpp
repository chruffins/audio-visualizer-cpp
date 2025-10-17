#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include<map>
#include<string>

// Utility function to load a TTF font with error handling
namespace util {
class Font {
public:
    Font() {
        filename = "";
    }

    Font(std::string filename) : filename(std::move(filename)) {}
    ~Font() {
        for (auto& pair : fonts) {
            al_destroy_font(pair.second);
        }
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
};
}; // namespace util