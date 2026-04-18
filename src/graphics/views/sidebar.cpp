#include "graphics/views/sidebar.hpp"

#include "graphics/event_handler.hpp"
#include "util/font.hpp"
#include <utility>

namespace ui {

SidebarView::SidebarView(std::shared_ptr<util::FontManager> fontManager,
                         graphics::EventDispatcher& eventDispatcher)
    : fontManager(std::move(fontManager)),
      mainFrame(std::make_shared<FrameDrawable>()) {
    mainFrame->setPosition(graphics::UV(0.0f, 0.0f, 0.0f, 0.0f));
    mainFrame->setSize(graphics::UV(0.0f, 1.0f, 220.0f, -100.0f));
    mainFrame->setBackgroundColor(al_map_rgba(18, 18, 26, 240));
    mainFrame->setBorderColor(al_map_rgb(70, 70, 90));
    mainFrame->setBorderThickness(2);
    mainFrame->setPadding(10.0f);
    mainFrame->m_isEnabled = [this]() { return isVisible; };

    eventDispatcher.addEventTarget(mainFrame);
    rebuildButtons();
}

void SidebarView::draw(const graphics::RenderContext& context) {
    if (!isVisible) {
        return;
    }

    mainFrame->draw(context);
}

void SidebarView::setBounds(const graphics::UV& position, const graphics::UV& size) {
    mainFrame->setPosition(position);
    mainFrame->setSize(size);
}

void SidebarView::setVisible(bool visible) {
    isVisible = visible;
}

void SidebarView::setSelectedView(LeftPanelView view) {
    selectedView = view;
    updateButtonStyles();
}

void SidebarView::rebuildButtons() {
    mainFrame->clearChildren();
    buttons.clear();
    buttonViews.clear();

    struct SidebarItem {
        const char* label;
        LeftPanelView view;
    };

    const SidebarItem items[] = {
        {"Albums", LeftPanelView::Albums},
        {"Favorites", LeftPanelView::Favorites},
        {"Search", LeftPanelView::Search},
    };

    ALLEGRO_FONT* buttonFont = fontManager->getFont("kanit")->getFont(16);

    float yOffset = 0.0f;
    const float buttonHeight = 42.0f;
    const float spacing = 10.0f;

    for (const auto& item : items) {
        auto button = std::make_unique<ButtonDrawable>(
            graphics::UV(0.0f, 0.0f, 0.0f, yOffset),
            graphics::UV(1.0f, 0.0f, 0.0f, buttonHeight),
            item.label
        );

        button->setFont(buttonFont);
        button->setOnClick([this, view = item.view]() {
            this->setSelectedView(view);
            if (this->onSelectionChanged) {
                this->onSelectionChanged(view);
            }
        });

        mainFrame->addChild(button.get());
        buttons.push_back(std::move(button));
        buttonViews.push_back(item.view);

        yOffset += buttonHeight + spacing;
    }

    updateButtonStyles();
}

void SidebarView::updateButtonStyles() {
    const ALLEGRO_COLOR activeNormal = al_map_rgb(78, 116, 180);
    const ALLEGRO_COLOR activeHover = al_map_rgb(98, 136, 200);
    const ALLEGRO_COLOR activePressed = al_map_rgb(58, 96, 160);

    const ALLEGRO_COLOR idleNormal = al_map_rgb(45, 45, 60);
    const ALLEGRO_COLOR idleHover = al_map_rgb(65, 65, 85);
    const ALLEGRO_COLOR idlePressed = al_map_rgb(35, 35, 50);

    for (size_t i = 0; i < buttons.size(); ++i) {
        if (!buttons[i]) {
            continue;
        }

        if (buttonViews[i] == selectedView) {
            buttons[i]->setColors(activeNormal, activeHover, activePressed);
        } else {
            buttons[i]->setColors(idleNormal, idleHover, idlePressed);
        }
    }
}

} // namespace ui
