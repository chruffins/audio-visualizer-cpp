#pragma once

#include <allegro5/allegro.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace ui {

/**
 * @brief Optional model for managing ALLEGRO_BITMAP resources
 * 
 * Provides centralized bitmap loading, caching, and lifecycle management.
 * Multiple ImageDrawables can share the same ImageModel to avoid duplicate loading.
 */
class ImageModel {
public:
    ImageModel() = default;
    ~ImageModel();

    // Disable copy
    ImageModel(const ImageModel&) = delete;
    ImageModel& operator=(const ImageModel&) = delete;

    // Enable move
    ImageModel(ImageModel&& other) noexcept;
    ImageModel& operator=(ImageModel&& other) noexcept;

    /**
     * @brief Load a bitmap from file
     * @param filepath Path to the image file
     * @param cache If true, cache the bitmap for reuse
     * @return Pointer to the loaded bitmap, or nullptr on failure
     */
    ALLEGRO_BITMAP* load(const std::string& filepath, bool cache = true);

    /**
     * @brief Get a cached bitmap
     * @param filepath Path that was used to load the bitmap
     * @return Pointer to the cached bitmap, or nullptr if not found
     */
    ALLEGRO_BITMAP* get(const std::string& filepath) const;

    /**
     * @brief Check if a bitmap is cached
     */
    bool isCached(const std::string& filepath) const;

    /**
     * @brief Unload a specific cached bitmap
     */
    void unload(const std::string& filepath);

    /**
     * @brief Unload all cached bitmaps
     */
    void clear();

    /**
     * @brief Create a blank bitmap
     * @param width Width in pixels
     * @param height Height in pixels
     * @return Pointer to the created bitmap, or nullptr on failure
     */
    ALLEGRO_BITMAP* createBlank(int width, int height);

    /**
     * @brief Create a bitmap from a sub-region of another bitmap
     * @param source Source bitmap
     * @param x X coordinate of the sub-region
     * @param y Y coordinate of the sub-region
     * @param width Width of the sub-region
     * @param height Height of the sub-region
     * @return Pointer to the created sub-bitmap, or nullptr on failure
     */
    ALLEGRO_BITMAP* createSubBitmap(ALLEGRO_BITMAP* source, int x, int y, int width, int height);

    /**
     * @brief Get the number of cached bitmaps
     */
    size_t getCachedCount() const { return cache.size(); }

private:
    std::unordered_map<std::string, ALLEGRO_BITMAP*> cache;
};

} // namespace ui
