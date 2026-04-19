#pragma once

#include "graphics/drawables/button.hpp"
#include "graphics/drawables/frame.hpp"
#include "graphics/uv.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace util {
    class FontManager;
}

namespace graphics {
    class EventDispatcher;
}

namespace ui {

enum class LeftPanelView {
    Albums = 0,
    NowPlaying = 1,
    Favorites = 2,
    Search = 3,
    Settings = 4
};

class SidebarView {
public:
    SidebarView() = delete;
    SidebarView(std::shared_ptr<util::FontManager> fontManager,
                graphics::EventDispatcher& eventDispatcher);

    void draw(const graphics::RenderContext& context);
    void setBounds(const graphics::UV& position, const graphics::UV& size);
    void setVisible(bool visible);

    void setSelectedView(LeftPanelView view);
    LeftPanelView getSelectedView() const { return selectedView; }

    void setOnSelectionChanged(std::function<void(LeftPanelView)> callback) {
        onSelectionChanged = std::move(callback);
    }

private:
    void rebuildButtons();
    void updateButtonStyles();

    std::shared_ptr<util::FontManager> fontManager;
    std::shared_ptr<FrameDrawable> mainFrame;
    std::vector<std::unique_ptr<ButtonDrawable>> buttons;
    std::vector<LeftPanelView> buttonViews;
    LeftPanelView selectedView = LeftPanelView::Albums;
    bool isVisible = true;

    std::function<void(LeftPanelView)> onSelectionChanged;
};

} // namespace ui
