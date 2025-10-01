#include "library_views.hpp"

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

#include <iostream>

using namespace music;

Artwork::Artwork(const std::string& path) {
    bitmap = al_load_bitmap(path.c_str());
    if (!bitmap) {
        // Handle error, e.g., log it
        std::cerr << "Failed to load artwork from path: " << path << std::endl;
    }
}

Artwork::~Artwork() {
    if (bitmap) {
        al_destroy_bitmap(bitmap);
    }
}

std::shared_ptr<Artwork> ArtworkCache::load(const std::string& path) {
    auto it = cache.find(path);
    if (it != cache.end()) {
        if (auto sharedPtr = it->second.lock()) {
            return sharedPtr; // Return existing shared_ptr if still valid
        }
    }
    // Load new artwork and store in cache
    auto artwork = std::make_shared<Artwork>(path);
    cache[path] = artwork;
    return artwork;
}

std::string SongView::toString() const {
    return "SongView(id: " + std::to_string(id) +
           ", title: " + title +
           ", album: " + album +
           ", artist: " + artist +
           ", genre: " + genre +
           ", comment: " + comment +
           ", track_number: " + std::to_string(track_number) +
           ", disc: " + std::to_string(disc) +
           ", year: " + std::to_string(year) +
           ", duration: " + std::to_string(duration) + "s)";
}

