#include "graphics/views/play_queue.hpp"
#include "core/music_engine.hpp"
#include "music/library.hpp"
#include "music/library_views.hpp"
#include "util/duration.hpp"
#include "util/font.hpp"
#include <allegro5/allegro.h>
#include <algorithm>
#include <sstream>

namespace ui {

PlayQueueView::PlayQueueView(std::shared_ptr<util::FontManager> fontManager,
                             graphics::EventDispatcher& eventDispatcher,
                             core::MusicEngine* musicEngine,
                             music::Library* library)
    : fontManager(fontManager),
      musicEngine(musicEngine),
      library(library),
      eventDispatcher(eventDispatcher),
      mainFrame(std::make_shared<FrameDrawable>()),
      scrollableFrame(std::make_shared<ScrollableFrameDrawable>()),
      contextLabelText(std::make_unique<TextDrawable>()) {
    
    // Main frame setup
    mainFrame->setPosition(graphics::UV(0.75f, 0.0f, 0.0f, 0.0f));
    mainFrame->setSize(graphics::UV(0.25f, 1.0f, 0.0f, -100.0f));
    mainFrame->setBackgroundColor(al_map_rgba(25, 25, 35, 230));
    mainFrame->setBorderColor(al_map_rgb(70, 70, 90));
    mainFrame->setBorderThickness(2);
    mainFrame->setPadding(5.0f);

    // Scrollable frame for songs
    scrollableFrame->setPosition(graphics::UV(0.0f, 0.0f, 0.0f, 10.0f));
    scrollableFrame->setSize(graphics::UV(1.0f, 1.0f, 0.0f, -10.0f));
    scrollableFrame->setBackgroundColor(al_map_rgba(20, 20, 30, 200));
    scrollableFrame->setBorderColor(al_map_rgb(60, 60, 80));
    scrollableFrame->setBorderThickness(1);
    scrollableFrame->setPadding(0.0f);
    scrollableFrame->setScrollStep(30.0f);
    scrollableFrame->enableScrollbar(true);
    
    // Context label
    auto font = fontManager->getFont("courier");
    *contextLabelText = TextDrawable(
        "Queue",
        graphics::UV(0.0f, 0.0f, 0.0f, 5.0f),
        graphics::UV(1.0f, 0.0f, -10.0f, 30.0f),
        14
    );
    contextLabelText->setFont(font)
                     .setMultiline(false)
                     .setAlignment(TextDrawable::HorizontalAlignment::Left)
                     .setVerticalAlignment(graphics::VerticalAlignment::TOP);

    mainFrame->addChild(scrollableFrame.get());
    mainFrame->addChild(contextLabelText.get());
    
    // Register mainFrame with event dispatcher to receive scroll events
    eventDispatcher.addEventTarget(mainFrame);
    
    refresh();
}

std::string PlayQueueView::getContextLabel() const {
    if (!musicEngine) return "Queue";
    
    auto contextType = musicEngine->getCurrentContextType();
    int contextId = musicEngine->getCurrentContextId();
    
    switch (contextType) {
        case music::PlaybackContextType::Individual:
            return "Now Playing";
        case music::PlaybackContextType::Album: {
            if (library && contextId >= 0) {
                const auto* album = library->getAlbumById(contextId);
                if (album) {
                    return "Album: " + album->title;
                }
            }
            return "Album";
        }
        case music::PlaybackContextType::Playlist: {
            if (library && contextId >= 0) {
                const auto* playlist = library->getPlaylistById(contextId);
                if (playlist) {
                    return "Playlist: " + playlist->name;
                }
            }
            return "Playlist";
        }
        default:
            return "Queue";
    }
}

void PlayQueueView::buildQueueDisplay() {
    scrollableFrame->clearChildren();
    queueItems.clear();
    totalContentHeight = 0.0f;
    
    if (!musicEngine || !musicEngine->playQueueModel || !library) {
        return;
    }
    
    const auto& playQueue = musicEngine->playQueueModel;
    auto font = fontManager->getFont("courier");
    
    const float itemHeight = 40.0f;
    const float padding = 5.0f;
    
    int currentIndex = playQueue->current_index;
    
    for (size_t i = 0; i < playQueue->song_ids.size(); ++i) {
        int song_id = playQueue->song_ids[i];
        const auto* songView = library->getSongById(song_id);
        
        if (!songView) continue;
        
        QueueSongItem item;
        item.song_id = song_id;
        item.is_current = (i == currentIndex);
        
        // Item frame
        item.frame.setPosition(graphics::UV(0.0f, 0.0f, 0.0f, totalContentHeight));
        item.frame.setSize(graphics::UV(1.0f, 0.0f, 0.0f, itemHeight));
        
        ALLEGRO_COLOR bgColor = item.is_current 
            ? al_map_rgba(60, 100, 150, 180)
            : al_map_rgba(40, 40, 55, 160);
        item.frame.setBackgroundColor(bgColor);
        item.frame.setBorderColor(al_map_rgb(70, 70, 90));
        item.frame.setBorderThickness(1);
        item.frame.setPadding(5.0f);
        
        // Track number
        item.trackNumberText->setText(std::to_string(i + 1));
        item.trackNumberText->setPosition(graphics::UV(0.0f, 0.0f, 0.0f, 5.0f));
        item.trackNumberText->setSize(graphics::UV(0.0f, 0.0f, 25.0f, 20.0f));
        item.trackNumberText->setFontSize(12);
        item.trackNumberText->setFont(font);
        item.trackNumberText->setAlignment(TextDrawable::HorizontalAlignment::Left);
        
        // Title
        item.titleText->setText(songView->title);
        item.titleText->setPosition(graphics::UV(0.0f, 0.0f, 25.0f, 5.0f));
        item.titleText->setSize(graphics::UV(1.0f, 0.0f, -70.0f, 20.0f));
        item.titleText->setFontSize(12);
        item.titleText->setFont(font);
        item.titleText->setAlignment(TextDrawable::HorizontalAlignment::Left);
        
        // Artist
        item.artistText->setText(songView->artist);
        item.artistText->setPosition(graphics::UV(0.0f, 0.0f, 25.0f, 22.0f));
        item.artistText->setSize(graphics::UV(1.0f, 0.0f, -70.0f, 15.0f));
        item.artistText->setFontSize(10);
        item.artistText->setFont(font);
        item.artistText->setAlignment(TextDrawable::HorizontalAlignment::Left);
        
        // Duration
        std::string durationStr = util::format_mm_ss(songView->duration);
        item.durationText->setText(durationStr);
        item.durationText->setPosition(graphics::UV(1.0f, 0.0f, -10.0f, 20.0f));
        item.durationText->setSize(graphics::UV(0.0f, 0.0f, 50.0f, 20.0f));
        item.durationText->setFontSize(10);
        item.durationText->setFont(font);
        item.durationText->setAlignment(TextDrawable::HorizontalAlignment::Right);
        
        // add texts to frame
        item.frame.addChild(item.trackNumberText.get());
        item.frame.addChild(item.titleText.get());
        item.frame.addChild(item.artistText.get());
        item.frame.addChild(item.durationText.get());
        
        queueItems.push_back(std::move(item));
        totalContentHeight += itemHeight + padding;

        // Add to scrollableFrame, not mainFrame
        scrollableFrame->addChild(&(queueItems.back().frame));
    }
    
    // Adjust total content height to not include final padding
    if (totalContentHeight > 0.0f) {
        totalContentHeight -= padding;
    }
}

void PlayQueueView::refresh() {
    buildQueueDisplay();
    if (scrollableFrame) {
        scrollableFrame->setScrollOffset(0.0f); // reset scroll to top on refresh
        scrollableFrame->setContentHeight(totalContentHeight);
    }
    if (contextLabelText) {
        contextLabelText->setText(getContextLabel());
    }
}

void PlayQueueView::recalculateLayout(const graphics::RenderContext& context) {
    if (lastDisplayWidth == context.screenWidth && 
        lastDisplayHeight == context.screenHeight) {
        return;
    }
    
    lastDisplayWidth = context.screenWidth;
    lastDisplayHeight = context.screenHeight;
    
    buildQueueDisplay();
    if (scrollableFrame) {
        scrollableFrame->setContentHeight(totalContentHeight);
    }
}

void PlayQueueView::draw(const graphics::RenderContext& context) {
    if (!isVisible) return;
    
    recalculateLayout(context);
    
    // Draw main frame background/border (includes children via FrameDrawable::draw)
    mainFrame->draw(context);
}

} // namespace ui
