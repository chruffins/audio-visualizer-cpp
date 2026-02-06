#include "graphics/views/album_list.hpp"
#include "music/album.hpp"
#include "music/library.hpp"
#include "core/music_engine.hpp"
#include "util/font.hpp"
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <algorithm>

namespace ui {

AlbumListView::AlbumListView(std::shared_ptr<util::FontManager> fontManager,
                             std::shared_ptr<music::Library> library,
                             core::MusicEngine* musicEngine,
                             graphics::EventDispatcher& eventDispatcher)
    : fontManager(std::move(fontManager)), library(std::move(library)),
      musicEngine(musicEngine),
      mainFrame(std::make_shared<ScrollableFrameDrawable>()) {
    
    // Configure main scrollable frame
    mainFrame->setPosition(graphics::UV(0.0f, 0.0f, 0.0f, 00.0f));
    mainFrame->setSize(graphics::UV(1.0f, 1.0f, 0.0f, -100.0f));
    mainFrame->setBackgroundColor(al_map_rgba(20, 20, 30, 240));
    mainFrame->setBorderColor(al_map_rgb(80, 80, 100));
    mainFrame->setBorderThickness(2);
    mainFrame->setPadding(15.0f);
    mainFrame->setScrollbarColor(al_map_rgb(120, 120, 140));
    mainFrame->setScrollbarWidth(8.0f);
    mainFrame->enableScrollbar(true);

    eventDispatcher.addEventTarget(mainFrame);

    refresh();
}

void AlbumListView::refresh() {
    selectedIndex = -1;
    rebuildItemList();
}

void AlbumListView::rebuildItemList() {
    // Remove old children so we don't keep pointers to destroyed items
    mainFrame->clearChildren();

    items.clear();
    if (library) {
        items.reserve(library->getAllAlbums().size());
    }
    
    auto font = fontManager->getFont("courier");
    
    if (!library) {
        mainFrame->setContentHeight(0.0f);
        lastDisplayWidth = 0;
        lastDisplayHeight = 0;
        return;
    }

    size_t idx = 0;
    for (const auto& [id, albumVal] : library->getAllAlbums()) {
        const music::Album* album = &albumVal;
        if (!album) continue;
        
        items.emplace_back();
        AlbumListItem& item = items.back();
        item.album = album;

        float yPos = static_cast<float>(idx) * (ITEM_HEIGHT + ITEM_SPACING);
        
        // Set up frame with margin to prevent overlap
        item.frame.setPosition(graphics::UV(0.0f, 0.0f, 0.0f, yPos));
        item.frame.setSize(graphics::UV(1.0f, 0.0f, 0.0f, ITEM_HEIGHT - ITEM_SPACING / 2.0f));
        item.frame.setPadding(5.0f);
        item.frame.setBackgroundColor(al_map_rgba(30, 30, 40, 200));
        item.frame.setBorderColor(al_map_rgba(60, 60, 80, 150));
        item.frame.setBorderThickness(1);
        
        // Album art
        item.albumArt->setPosition(graphics::UV(0.0f, 0.0f, 0.0f, 0.0f));
        item.albumArt->setSize(graphics::UV(0.0f, 0.0f, ALBUM_ART_SIZE, ALBUM_ART_SIZE));
        item.albumArt->setScaleMode(ImageDrawable::ScaleMode::STRETCH);
        item.albumArt->setImageModel(album->cover_art_model);
        
        // Title text
        *item.titleText = TextDrawable(
            album->title,
            graphics::UV(0.0f, 0.0f, TEXT_OFFSET_X, 5.0f),
            graphics::UV(1.0f, 0.0f, -TEXT_OFFSET_X - 20.0f, 24.0f),
            16
        ).setFont(font)
         .setMultiline(false)
         .setAlignment(TextDrawable::HorizontalAlignment::Left)
         .setVerticalAlignment(graphics::VerticalAlignment::TOP);
        
        // Artist text (we'd need to look up artist name from library)
        std::string artistText = library->getArtistById(album->artist_id) ? 
                                 library->getArtistById(album->artist_id)->name : "Unknown Artist";
        *item.artistText = TextDrawable(
            artistText,
            graphics::UV(0.0f, 0.0f, TEXT_OFFSET_X, 30.0f),
            graphics::UV(1.0f, 0.0f, -TEXT_OFFSET_X - 20.0f, 20.0f),
            14
        ).setFont(font)
         .setMultiline(false)
         .setAlignment(TextDrawable::HorizontalAlignment::Left)
         .setVerticalAlignment(graphics::VerticalAlignment::TOP);
        
        // Year text
        std::string yearStr = album->year > 0 ? std::to_string(album->year) : "Unknown";
        *item.yearText = TextDrawable(
            yearStr,
            graphics::UV(0.0f, 0.0f, TEXT_OFFSET_X, 50.0f),
            graphics::UV(1.0f, 0.0f, -TEXT_OFFSET_X - 20.0f, 20.0f),
            12
        ).setFont(font)
         .setMultiline(false)
         .setAlignment(TextDrawable::HorizontalAlignment::Left)
         .setVerticalAlignment(graphics::VerticalAlignment::TOP)
         .setColor(al_map_rgb(180, 180, 180));
        
        // Action button (right side)
        item.actionButton->setPosition(graphics::UV(1.0f, 0.0f, -75.0f, 25.0f));
        item.actionButton->setSize(graphics::UV(0.0f, 0.0f, 70.0f, 30.0f));
        item.actionButton->setColors(
            al_map_rgb(80, 120, 180),
            al_map_rgb(100, 140, 200),
            al_map_rgb(60, 100, 160)
        );
        item.actionButton->setLabel("Play");
        item.actionButton->setOnClick([this, album]() {
            if (musicEngine && album) {
                musicEngine->playAlbum(album->id);
            }
        });
        
        // Add frame as child of the main frame (contains all UI elements)
        mainFrame->addChild(&item.frame);
        ++idx;
    }
    
    // Update content height for scrolling
    float contentHeight = calculateContentHeight();
    mainFrame->setContentHeight(contentHeight);
    
    // Need to rebuild layout on next draw
    lastDisplayWidth = 0;
    lastDisplayHeight = 0;
}

float AlbumListView::calculateContentHeight() const {
    if (items.empty()) {
        return 0.0f;
    }
    return items.size() * (ITEM_HEIGHT + ITEM_SPACING) - ITEM_SPACING;
}

void AlbumListView::scrollBy(float delta) {
    mainFrame->scrollBy(delta);
}

void AlbumListView::scrollTo(float offset) {
    mainFrame->setScrollOffset(offset);
}

float AlbumListView::getScrollOffset() const {
    return mainFrame->getScrollOffset();
}

void AlbumListView::setSelectedIndex(int index) {
    if (index >= -1 && index < static_cast<int>(items.size())) {
        selectedIndex = index;
    }
}

const music::Album* AlbumListView::getSelectedAlbum() const {
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size())) {
        return items[selectedIndex].album;
    }
    return nullptr;
}

int AlbumListView::handleClick(float x, float y, const graphics::RenderContext& context) {
    // Calculate which item was clicked based on y position
    // Need to account for scroll offset and padding
    auto sizePx = mainFrame->getSize().toScreenPos(
        static_cast<float>(context.screenWidth), 
        static_cast<float>(context.screenHeight)
    );
    auto posPx = mainFrame->getPosition().toScreenPos(
        static_cast<float>(context.screenWidth), 
        static_cast<float>(context.screenHeight)
    );
    
    float frameX = posPx.first + context.offsetX;
    float frameY = posPx.second + context.offsetY;
    float frameWidth = sizePx.first;
    float frameHeight = sizePx.second;
    
    // Check if click is within frame bounds
    if (x < frameX || x > frameX + frameWidth || y < frameY || y > frameY + frameHeight) {
        return -1;
    }
    
    // Adjust for padding and scroll offset
    float padding = mainFrame->getPadding();
    float relativeY = y - frameY - padding + mainFrame->getScrollOffset();
    
    // Calculate which item
    int clickedIndex = static_cast<int>(relativeY / (ITEM_HEIGHT + ITEM_SPACING));
    
    if (clickedIndex >= 0 && clickedIndex < static_cast<int>(items.size())) {
        selectedIndex = clickedIndex;
        if (onAlbumSelected && items[clickedIndex].album) {
            onAlbumSelected(items[clickedIndex].album);
        }
        return clickedIndex;
    }
    
    return -1;
}

void AlbumListView::recalculateLayout(const graphics::RenderContext& context) {
    // Currently layout is fixed, but this could be extended for responsive design
    lastDisplayWidth = context.screenWidth;
    lastDisplayHeight = context.screenHeight;
}

void AlbumListView::draw(const graphics::RenderContext& context) {
    // Check if we need to recalculate layout
    if (context.screenWidth != lastDisplayWidth || context.screenHeight != lastDisplayHeight) {
        recalculateLayout(context);
    }
    
    // Draw the main frame with all children
    mainFrame->draw(context);
}

} // namespace ui
