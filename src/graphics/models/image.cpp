#include "graphics/models/image.hpp"

using namespace ui;

ImageModel::~ImageModel() {
    clear();
}

ImageModel::ImageModel(ImageModel&& other) noexcept 
    : cache(std::move(other.cache)) {
    other.cache.clear();
}

ImageModel& ImageModel::operator=(ImageModel&& other) noexcept {
    if (this != &other) {
        clear();
        cache = std::move(other.cache);
        other.cache.clear();
    }
    return *this;
}

ALLEGRO_BITMAP* ImageModel::load(const std::string& filepath, bool doCache) {
    // Check if already cached
    if (doCache) {
        auto it = cache.find(filepath);
        if (it != cache.end()) {
            return it->second;
        }
    }

    // Load the bitmap
    ALLEGRO_BITMAP* bitmap = al_load_bitmap(filepath.c_str());
    
    // Cache if requested and successful
    if (bitmap && doCache) {
        cache[filepath] = bitmap;
    }

    return bitmap;
}

ALLEGRO_BITMAP* ImageModel::get(const std::string& filepath) const {
    auto it = cache.find(filepath);
    return (it != cache.end()) ? it->second : nullptr;
}

bool ImageModel::isCached(const std::string& filepath) const {
    return cache.find(filepath) != cache.end();
}

void ImageModel::unload(const std::string& filepath) {
    auto it = cache.find(filepath);
    if (it != cache.end()) {
        if (it->second) {
            al_destroy_bitmap(it->second);
        }
        cache.erase(it);
    }
}

void ImageModel::clear() {
    for (auto& pair : cache) {
        if (pair.second) {
            al_destroy_bitmap(pair.second);
        }
    }
    cache.clear();
}

ALLEGRO_BITMAP* ImageModel::createBlank(int width, int height) {
    if (width <= 0 || height <= 0) {
        return nullptr;
    }
    return al_create_bitmap(width, height);
}

ALLEGRO_BITMAP* ImageModel::createSubBitmap(ALLEGRO_BITMAP* source, int x, int y, int width, int height) {
    if (!source || width <= 0 || height <= 0) {
        return nullptr;
    }

    int srcWidth = al_get_bitmap_width(source);
    int srcHeight = al_get_bitmap_height(source);

    // Clamp to source bitmap bounds
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width > srcWidth) {
        width = srcWidth - x;
    }
    if (y + height > srcHeight) {
        height = srcHeight - y;
    }

    if (width <= 0 || height <= 0) {
        return nullptr;
    }

    return al_create_sub_bitmap(source, x, y, width, height);
}
