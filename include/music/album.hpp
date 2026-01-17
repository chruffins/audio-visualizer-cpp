#pragma once
#include <string>
#include <memory>
#include "graphics/models/image.hpp"

namespace music {
struct Album {
    int id;                         // Unique identifier for the album
    std::string title;              // Title of the album
    int year;                       // Release year
    std::string cover_image_path;   // Path to the cover image file (if external)
    int artist_id;                  // Foreign key to the artist
    
    // Album art management
    std::shared_ptr<ui::ImageModel> cover_art_model; // Manages album art bitmap loading/caching
    std::string cover_art_mime;                      // MIME type of embedded cover art

    // Constructor with path only (for backward compatibility)
    Album(int id, const std::string& title, int year, const std::string& cover_image_path, int artist_id)
        : id(id), title(title), year(year), cover_image_path(cover_image_path), artist_id(artist_id),
          cover_art_model(nullptr) {}

    // Full constructor with embedded cover art
    Album(int id, const std::string& title, int year, const std::string& cover_image_path, int artist_id,
          ui::ImageModel cover_art_model, const std::string& cover_art_mime)
        : id(id), title(title), year(year), cover_image_path(cover_image_path), artist_id(artist_id),
          cover_art_model(std::make_shared<ui::ImageModel>(std::move(cover_art_model))), cover_art_mime(cover_art_mime) {}

    // Get the album art bitmap (loads if necessary)
    ALLEGRO_BITMAP* getAlbumArt() const;
    
    // Check if album has cover art available (either embedded or as file)
    bool hasCoverArt() const;
    
    // Function to use for printing album information
    std::string toString() const;

    // Comparison operator for sorting albums by year, then title
    bool operator<(const Album& other) const {
        if (year != other.year) {
            return year < other.year;
        }
        return title < other.title;
    }
};
} // namespace music