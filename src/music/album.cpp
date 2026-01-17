#include "music/album.hpp"
#include <sstream>

namespace music {

ALLEGRO_BITMAP* Album::getAlbumArt() const {
    // If we have an ImageModel with embedded cover art, use it
    if (cover_art_model) {
        // Check if already loaded in cache
        if (cover_art_model->isCached(EMBEDDED_IMAGE_PATH)) {
            return cover_art_model->get(EMBEDDED_IMAGE_PATH);
        }
        // If not cached but we have the model, it should have been loaded during construction
        // This shouldn't normally happen, but return nullptr if the bitmap isn't available
    }
    
    // Fall back to loading from file path if available
    if (!cover_image_path.empty()) {
        // Try to load from the external file
        // Note: This creates a temporary ImageModel just for loading
        // Consider caching this if needed
        ui::ImageModel tempModel;
        return tempModel.load(cover_image_path, false);
    }
    
    return nullptr;
}

bool Album::hasCoverArt() const {
    return (cover_art_model != nullptr) || (!cover_image_path.empty());
}

std::string Album::toString() const {
    std::ostringstream oss;
    oss << "Album{id=" << id 
        << ", title=\"" << title << "\""
        << ", year=" << year
        << ", artist_id=" << artist_id;
    
    if (hasCoverArt()) {
        oss << ", has_cover_art=true";
        if (!cover_image_path.empty()) {
            oss << ", cover_path=\"" << cover_image_path << "\"";
        }
        if (cover_art_model) {
            oss << ", embedded_cover=true";
        }
    } else {
        oss << ", has_cover_art=false";
    }
    
    oss << "}";
    return oss.str();
}

} // namespace music
