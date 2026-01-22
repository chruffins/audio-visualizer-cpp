#pragma once

#include "graphics/drawables/scrollable_frame.hpp"
#include "graphics/drawables/text.hpp"
#include "graphics/drawables/image.hpp"
#include "graphics/uv.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace util {
    class FontManager;
}

namespace music {
    struct Album;
    class Library;
}

namespace ui {

// Individual album item in the list
struct AlbumListItem {
    ImageDrawable albumArt;
    TextDrawable titleText;
    TextDrawable artistText;
    TextDrawable yearText;
    
    // Reference to the album data
    const music::Album* album = nullptr;
};

// View for displaying a scrollable list of albums
class AlbumListView {
public:
    AlbumListView() = delete;
    AlbumListView(std::shared_ptr<util::FontManager> fontManager,
                  std::shared_ptr<music::Library> library);

    // Reload albums from the library
    void refresh();
    
    // Scroll control
    void scrollBy(float delta);
    void scrollTo(float offset);
    float getScrollOffset() const;
    
    // Selection/interaction
    void setSelectedIndex(int index);
    int getSelectedIndex() const { return selectedIndex; }
    const music::Album* getSelectedAlbum() const;
    
    // Click handling (returns index of clicked album or -1)
    int handleClick(float x, float y, const graphics::RenderContext& context);
    
    // Callback when an album is selected/clicked
    void setOnAlbumSelected(std::function<void(const music::Album*)> callback) {
        onAlbumSelected = callback;
    }

    void draw(const graphics::RenderContext& context);

private:
    void rebuildItemList();
    void recalculateLayout(const graphics::RenderContext& context);
    float calculateContentHeight() const;
    
    std::shared_ptr<util::FontManager> fontManager;
    std::shared_ptr<music::Library> library;
    int lastDisplayWidth = 0;
    int lastDisplayHeight = 0;

    // Main scrollable container
    ScrollableFrameDrawable mainFrame;
    
    // Album items
    std::vector<AlbumListItem> items;
    
    // Layout constants
    static constexpr float ITEM_HEIGHT = 80.0f;
    static constexpr float ITEM_SPACING = 10.0f;
    static constexpr float ALBUM_ART_SIZE = 64.0f;
    static constexpr float TEXT_OFFSET_X = 74.0f;
    
    // Selection state
    int selectedIndex = -1;
    ALLEGRO_COLOR selectedColor = al_map_rgb(80, 120, 180);
    ALLEGRO_COLOR hoverColor = al_map_rgb(60, 60, 80);
    
    // Callback
    std::function<void(const music::Album*)> onAlbumSelected;
};

} // namespace ui
