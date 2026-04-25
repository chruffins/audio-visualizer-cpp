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
    mainFrame->setSize(graphics::UV(0.75f, 1.0f, 0.0f, -100.0f));
    mainFrame->setBackgroundColor(al_map_rgba(20, 20, 30, 240));
    mainFrame->setBorderColor(al_map_rgb(80, 80, 100));
    mainFrame->setBorderThickness(2);
    mainFrame->setPadding(5.0f);
    mainFrame->setScrollbarColor(al_map_rgb(120, 120, 140));
    mainFrame->setScrollbarWidth(8.0f);
    mainFrame->enableScrollbar(true);
    mainFrame->m_isEnabled = [this]() { return isVisible; };

    eventDispatcher.addEventTarget(mainFrame);

    refresh();
}

void AlbumListView::refresh() {
    selectedIndex = -1;
    rebuildItemList();
}

void AlbumListView::rebuildSortControls() {
    if (!sortByNameButton) {
        sortByNameButton = std::make_unique<ButtonDrawable>(
            graphics::UV(0.0f, 0.0f, 0.0f, 0.0f),
            graphics::UV(0.333f, 0.0f, -4.0f, SORT_BUTTON_HEIGHT),
            "Name"
        );
        sortByNameButton->setOnClick([this]() {
            currentSortMode = SortMode::ByTitle;
            sortItems(currentSortMode);
            relayoutItemPositions();
            updateSortButtonStyles();
        });
    }

    if (!sortByArtistButton) {
        sortByArtistButton = std::make_unique<ButtonDrawable>(
            graphics::UV(0.333f, 0.0f, 2.0f, 0.0f),
            graphics::UV(0.333f, 0.0f, -4.0f, SORT_BUTTON_HEIGHT),
            "Artist"
        );
        sortByArtistButton->setOnClick([this]() {
            currentSortMode = SortMode::ByArtist;
            sortItems(currentSortMode);
            relayoutItemPositions();
            updateSortButtonStyles();
        });
    }

    if (!sortByYearButton) {
        sortByYearButton = std::make_unique<ButtonDrawable>(
            graphics::UV(0.666f, 0.0f, 4.0f, 0.0f),
            graphics::UV(0.334f, 0.0f, -4.0f, SORT_BUTTON_HEIGHT),
            "Release"
        );
        sortByYearButton->setOnClick([this]() {
            currentSortMode = SortMode::ByYear;
            sortItems(currentSortMode);
            relayoutItemPositions();
            updateSortButtonStyles();
        });
    }

    ALLEGRO_FONT* buttonFont = fontManager->getFont("kanit")->getFont(16);
    sortByNameButton->setFont(buttonFont);
    sortByArtistButton->setFont(buttonFont);
    sortByYearButton->setFont(buttonFont);

    mainFrame->addChild(sortByNameButton.get());
    mainFrame->addChild(sortByArtistButton.get());
    mainFrame->addChild(sortByYearButton.get());
    updateSortButtonStyles();
}

void AlbumListView::updateSortButtonStyles() {
    const ALLEGRO_COLOR activeNormal = al_map_rgb(78, 116, 180);
    const ALLEGRO_COLOR activeHover = al_map_rgb(98, 136, 200);
    const ALLEGRO_COLOR activePressed = al_map_rgb(58, 96, 160);

    const ALLEGRO_COLOR idleNormal = al_map_rgb(45, 45, 60);
    const ALLEGRO_COLOR idleHover = al_map_rgb(65, 65, 85);
    const ALLEGRO_COLOR idlePressed = al_map_rgb(35, 35, 50);

    if (sortByNameButton) {
        const bool isActive = currentSortMode == SortMode::ByTitle;
        sortByNameButton->setColors(
            isActive ? activeNormal : idleNormal,
            isActive ? activeHover : idleHover,
            isActive ? activePressed : idlePressed
        );
    }

    if (sortByArtistButton) {
        const bool isActive = currentSortMode == SortMode::ByArtist;
        sortByArtistButton->setColors(
            isActive ? activeNormal : idleNormal,
            isActive ? activeHover : idleHover,
            isActive ? activePressed : idlePressed
        );
    }

    if (sortByYearButton) {
        const bool isActive = currentSortMode == SortMode::ByYear;
        sortByYearButton->setColors(
            isActive ? activeNormal : idleNormal,
            isActive ? activeHover : idleHover,
            isActive ? activePressed : idlePressed
        );
    }
}

void AlbumListView::relayoutItemPositions() {
    for (size_t i = 0; i < items.size(); ++i) {
        const float yPos = SORT_SECTION_HEIGHT + static_cast<float>(i) * (ITEM_HEIGHT + ITEM_SPACING);
        items[i].frame.setPosition(graphics::UV(0.0f, 0.0f, 0.0f, yPos));
    }
}

void AlbumListView::rebuildItemList() {
    // Remove old children so we don't keep pointers to destroyed items
    mainFrame->clearChildren();
    rebuildSortControls();

    items.clear();
    if (library) {
        const auto& albums = library->getAllAlbums();
        items.reserve(albums.size());
    }
    
    auto kanitFont = fontManager->getFont("kanit");
    auto gothicFont = fontManager->getFont("gothic");
    
    if (!library) {
        mainFrame->setContentHeight(0.0f);
        lastDisplayWidth = 0;
        lastDisplayHeight = 0;
        return;
    }

    const auto& albums = library->getAllAlbums();
    size_t idx = 0;
    for (const auto& [id, albumVal] : albums) {
        const music::Album* album = &albumVal;
        if (!album) continue;
        
        items.emplace_back();
        AlbumListItem& item = items.back();
        configureItem(item, album, idx, kanitFont, gothicFont);
        
        ++idx;
    }

    sortItems(currentSortMode);
    relayoutItemPositions();

    for (auto& item : items) {
        // Add frame as child of the main frame (contains all UI elements)
        mainFrame->addChild(&item.frame);
    }
    
    // Update content height for scrolling
    float contentHeight = calculateContentHeight();
    mainFrame->setContentHeight(contentHeight);
    
    // Need to rebuild layout on next draw
    lastDisplayWidth = 0;
    lastDisplayHeight = 0;
}

void AlbumListView::configureItem(AlbumListItem& item,
                                  const music::Album* album,
                                  size_t index,
                                  const std::shared_ptr<util::Font>& kanitFont,
                                  const std::shared_ptr<util::Font>& gothicFont) {
    item.album = album;

    const float yPos = SORT_SECTION_HEIGHT + static_cast<float>(index) * (ITEM_HEIGHT + ITEM_SPACING);

    item.frame.setPosition(graphics::UV(0.0f, 0.0f, 0.0f, yPos));
    item.frame.setSize(graphics::UV(1.0f, 0.0f, 0.0f, ITEM_HEIGHT - ITEM_SPACING / 2.0f));
    item.frame.setPadding(5.0f);
    item.frame.setBackgroundColor(al_map_rgba(30, 30, 40, 200));
    item.frame.setBorderColor(al_map_rgba(60, 60, 80, 150));
    item.frame.setBorderThickness(1);

    item.albumArt->setPosition(graphics::UV(0.0f, 0.0f, 0.0f, 0.0f));
    item.albumArt->setSize(graphics::UV(0.0f, 0.0f, ALBUM_ART_SIZE, ALBUM_ART_SIZE));
    item.albumArt->setScaleMode(ImageDrawable::ScaleMode::STRETCH);
    item.albumArt->setImageModel(album->cover_art_model);

    const auto* artist = library->getArtistById(album->artist_id);
    item.artistName = artist ? artist->name : "Unknown Artist";

    *item.titleText = TextDrawable(
        album->title,
        graphics::UV(0.0f, 0.0f, TEXT_OFFSET_X, 5.0f),
        graphics::UV(1.0f, 0.0f, -TEXT_OFFSET_X - 20.0f, 24.0f),
        16
    ).setFont(gothicFont)
     .setMultiline(false)
     .setAlignment(TextDrawable::HorizontalAlignment::Left)
     .setVerticalAlignment(graphics::VerticalAlignment::TOP);

    *item.artistText = TextDrawable(
        item.artistName,
        graphics::UV(0.0f, 0.0f, TEXT_OFFSET_X, 30.0f),
        graphics::UV(1.0f, 0.0f, -TEXT_OFFSET_X - 20.0f, 20.0f),
        14
    ).setFont(kanitFont)
     .setMultiline(false)
     .setAlignment(TextDrawable::HorizontalAlignment::Left)
     .setVerticalAlignment(graphics::VerticalAlignment::TOP);

    const std::string yearStr = album->year > 0 ? std::to_string(album->year) : "Unknown";
    *item.yearText = TextDrawable(
        yearStr,
        graphics::UV(0.0f, 0.0f, TEXT_OFFSET_X, 50.0f),
        graphics::UV(1.0f, 0.0f, -TEXT_OFFSET_X - 20.0f, 20.0f),
        12
    ).setFont(kanitFont)
     .setMultiline(false)
     .setAlignment(TextDrawable::HorizontalAlignment::Left)
     .setVerticalAlignment(graphics::VerticalAlignment::TOP)
     .setColor(al_map_rgb(180, 180, 180));

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
}

void AlbumListView::sortItems(SortMode mode) {
    switch (mode) {
        case SortMode::ByYear:
            std::sort(items.begin(), items.end(), [](const AlbumListItem& a, const AlbumListItem& b) {
                if (!a.album || !b.album) return false;
                return a.album->year < b.album->year;
            });
            break;
        case SortMode::ByTitle:
            std::sort(items.begin(), items.end(), [](const AlbumListItem& a, const AlbumListItem& b) {
                if (!a.album || !b.album) return false;
                return a.album->title < b.album->title;
            });
            break;
        case SortMode::ByArtist:
            std::sort(items.begin(), items.end(), [](const AlbumListItem& a, const AlbumListItem& b) {
                if (!a.album || !b.album) return false;
                if (a.artistName != b.artistName) {
                    return a.artistName < b.artistName;
                }
                if (a.album->year != b.album->year) {
                    return a.album->year < b.album->year;
                }
                return a.album->title < b.album->title;
            });
            break;
        default:
            break;
    }
}

float AlbumListView::calculateContentHeight() const {
    if (items.empty()) {
        return SORT_SECTION_HEIGHT;
    }
    return SORT_SECTION_HEIGHT + items.size() * (ITEM_HEIGHT + ITEM_SPACING) - ITEM_SPACING;
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
    
    // Ignore click area occupied by sort controls.
    if (relativeY < SORT_SECTION_HEIGHT) {
        return -1;
    }

    // Calculate which item
    int clickedIndex = static_cast<int>((relativeY - SORT_SECTION_HEIGHT) / (ITEM_HEIGHT + ITEM_SPACING));
    
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
    if (!isVisible) {
        return;
    }

    // Check if we need to recalculate layout
    if (context.screenWidth != lastDisplayWidth || context.screenHeight != lastDisplayHeight) {
        recalculateLayout(context);
    }
    
    // Draw the main frame with all children
    mainFrame->draw(context);
}

void AlbumListView::setBounds(const graphics::UV& position, const graphics::UV& size) {
    mainFrame->setPosition(position);
    mainFrame->setSize(size);
    lastDisplayWidth = 0;
    lastDisplayHeight = 0;
}

void AlbumListView::setVisible(bool visible) {
    isVisible = visible;
}

} // namespace ui
