#pragma once

#include "graphics/drawables/scrollable_frame.hpp"
#include "graphics/drawables/frame.hpp"
#include "graphics/drawables/text.hpp"
#include "graphics/drawables/button.hpp"
#include "graphics/uv.hpp"
#include "graphics/event_handler.hpp"
#include "music/play_queue.hpp"
#include <memory>
#include <vector>

namespace util {
    class FontManager;
}

namespace core {
    class MusicEngine;
}

namespace music {
    class Library;
}

namespace ui {

// Individual song item in the queue
struct QueueSongItem {
    FrameDrawable frame;
    std::unique_ptr<TextDrawable> trackNumberText;
    std::unique_ptr<TextDrawable> titleText;
    std::unique_ptr<TextDrawable> artistText;
    std::unique_ptr<TextDrawable> durationText;
    std::unique_ptr<ButtonDrawable> playButton;
    
    int song_id = -1;
    bool is_current = false;
    
    QueueSongItem() {
        trackNumberText = std::make_unique<TextDrawable>();
        titleText = std::make_unique<TextDrawable>();
        artistText = std::make_unique<TextDrawable>();
        durationText = std::make_unique<TextDrawable>();
        playButton = std::make_unique<ButtonDrawable>(
            graphics::UV(0.0f, 0.0f, 0.0f, 0.0f),
            graphics::UV(0.0f, 0.0f, 30.0f, 30.0f),
            "►"
        );
    }
};

// Main play queue view - displays current context and queued songs
// 
// Usage example:
//   auto queueView = ui::PlayQueueView(
//       appState.fontManager,
//       appState.event_dispatcher,
//       &appState.music_engine,
//       appState.library
//   );
//   
//   // In your main loop, call refresh when context changes:
//   // queueView.refresh();
//   
//   // Draw the view each frame:
//   // queueView.draw(globalContext);
//
class PlayQueueView {
public:
    PlayQueueView() = delete;
    PlayQueueView(std::shared_ptr<util::FontManager> fontManager,
                  graphics::EventDispatcher& eventDispatcher,
                  core::MusicEngine* musicEngine,
                  music::Library* library);

    void refresh();  // Rebuild queue display from current music engine state
    void draw(const graphics::RenderContext& context);
    void setVisible(bool visible) { isVisible = visible; }
    bool getVisible() const { return isVisible; }

private:
    void recalculateLayout(const graphics::RenderContext& context);
    void buildQueueDisplay();
    std::string getContextLabel() const;
    
    std::shared_ptr<util::FontManager> fontManager;
    core::MusicEngine* musicEngine;
    music::Library* library;
    graphics::EventDispatcher& eventDispatcher;
    
    std::shared_ptr<FrameDrawable> mainFrame;
    std::shared_ptr<ScrollableFrameDrawable> scrollableFrame;
    std::unique_ptr<TextDrawable> contextLabelText;
    
    std::vector<QueueSongItem> queueItems;
    
    bool isVisible = true;
    int lastDisplayWidth = 0;
    int lastDisplayHeight = 0;
    float totalContentHeight = 0.0f;
};

} // namespace ui
