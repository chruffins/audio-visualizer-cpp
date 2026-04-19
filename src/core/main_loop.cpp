#include "core/main_loop.hpp"
#include "core/app_state.hpp"
#include "graphics/views/now_playing.hpp"
#include "graphics/views/album_list.hpp"
#include "graphics/views/play_queue.hpp"
#include "graphics/views/sidebar.hpp"
#include "graphics/drawables/frame.hpp"
#include "graphics/drawables/text.hpp"
#include "graphics/uv.hpp"
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <iostream>

#include "scrobble.h"

namespace core {

void runMainLoop() {
    auto& appState = AppState::instance();
    bool finished = false;

    auto nowPlayingView = ui::NowPlayingView(appState.fontManager, appState.music_engine.progressBarModel, appState.event_dispatcher, &appState.music_engine);
    auto albumListView = ui::AlbumListView(appState.fontManager, appState.library, &appState.music_engine, appState.event_dispatcher);
    auto playQueueView = ui::PlayQueueView(appState.fontManager, appState.event_dispatcher, &appState.music_engine, appState.library.get());
    auto sidebarView = ui::SidebarView(appState.fontManager, appState.event_dispatcher);

    ui::LeftPanelView activeLeftView = ui::LeftPanelView::Albums;

    ui::FrameDrawable leftPlaceholderFrame;
    leftPlaceholderFrame.setBackgroundColor(al_map_rgba(20, 20, 30, 240));
    leftPlaceholderFrame.setBorderColor(al_map_rgb(80, 80, 100));
    leftPlaceholderFrame.setBorderThickness(2);
    leftPlaceholderFrame.setPadding(12.0f);

    ui::TextDrawable leftPlaceholderTitle(
        "",
        graphics::UV(0.0f, 0.0f, 0.0f, 10.0f),
        graphics::UV(1.0f, 0.0f, 0.0f, 30.0f),
        18
    );
    leftPlaceholderTitle.setFont(appState.fontManager->getFont("kanit"));
    leftPlaceholderTitle.setAlignment(ui::TextDrawable::HorizontalAlignment::Left);
    leftPlaceholderTitle.setVerticalAlignment(graphics::VerticalAlignment::TOP);
    leftPlaceholderFrame.addChild(&leftPlaceholderTitle);

    const float footerHeight = 101.0f;
    const float sidebarWidth = 150.0f;
    const float queueWidth = 200.0f;

    auto applyLayout = [&]() {
        nowPlayingView.setBounds(
            graphics::UV(0.0f, 1.0f, 0.0f, -footerHeight),
            graphics::UV(1.0f, 0.0f, 0.0f, footerHeight)
        );

        sidebarView.setBounds(
            graphics::UV(0.0f, 0.0f, 0.0f, 0.0f),
            graphics::UV(0.0f, 1.0f, sidebarWidth, -footerHeight)
        );

        albumListView.setBounds(
            graphics::UV(0.0f, 0.0f, sidebarWidth, 0.0f),
            graphics::UV(1.0f, 1.0f, -(sidebarWidth + queueWidth), -footerHeight)
        );

        playQueueView.setBounds(
            graphics::UV(1.0f, 0.0f, -queueWidth, 0.0f),
            graphics::UV(0.0f, 1.0f, queueWidth, -footerHeight)
        );

        leftPlaceholderFrame.setPosition(graphics::UV(0.0f, 0.0f, sidebarWidth, 0.0f));
        leftPlaceholderFrame.setSize(graphics::UV(1.0f, 1.0f, -(sidebarWidth + queueWidth), -footerHeight));
    };

    applyLayout();

    auto applyLeftPanelSelection = [&](ui::LeftPanelView selection) {
        activeLeftView = selection;
        albumListView.setVisible(activeLeftView == ui::LeftPanelView::Albums);

        if (activeLeftView == ui::LeftPanelView::Favorites) {
            leftPlaceholderTitle.setText("Favorites view is coming soon.");
        } else if (activeLeftView == ui::LeftPanelView::Search) {
            leftPlaceholderTitle.setText("Search view is coming soon.");
        } else if (activeLeftView == ui::LeftPanelView::Settings) {
            leftPlaceholderTitle.setText("Settings view is coming soon.");
        } else if (activeLeftView == ui::LeftPanelView::NowPlaying) {
            leftPlaceholderTitle.setText("");
        } else {
            leftPlaceholderTitle.setText("");
        }
    };

    sidebarView.setOnSelectionChanged([&](ui::LeftPanelView selection) {
        applyLeftPanelSelection(selection);
    });

    applyLeftPanelSelection(activeLeftView);

    // Register a callback so that the NowPlayingView updates automatically when
    // MusicEngine starts a new song (e.g., via playNext or other engine-driven
    // playback). This keeps the UI in sync without polling.
    appState.music_engine.onSongChanged = [&](const music::SongView& s) {
        nowPlayingView.setSongTitle(s.title);
        nowPlayingView.setArtistName(s.artist);
        nowPlayingView.setAlbumName(s.album);

        const music::Album* album = appState.library->getAlbumById(s.album_id);
        if (album) {
            auto albumArt = album->getAlbumArt();
            nowPlayingView.setAlbumArt(albumArt);
            if (!albumArt) {
                std::cerr << "Warning: Failed to load album art for album: " << album->title << "\n";
            }
        }
        nowPlayingView.setDuration(s.duration);
        nowPlayingView.setPosition(0);
        playQueueView.refresh(); // Refresh the queue view to highlight the current song

        appState.discord_integration.setSongPresence(s);
    };

    // Register a callback to automatically play the next song when current finishes
    appState.music_engine.onSongFinished = [&]() {
        std::cout << "Song finished, playing next...\n";
        // Last.fm scrobble goes here...
        auto songId = appState.music_engine.playQueueModel ? appState.music_engine.playQueueModel->current() : -1;
        if (songId >= 0) {
            const music::SongView* song = appState.library->getSongById(songId);
            if (song && appState.lastfm_enabled) {
                int result = scrob_easy_scrobble(appState.scrobClient, song->artist.c_str(), song->title.c_str(), song->album.c_str(), time(nullptr) - song->duration);
                if (result != 0) {
                    std::cerr << "Failed to scrobble song to last.fm...\n";
                }
            }
        }

        appState.music_engine.playNext();
    };

    // Example: Play a sound file (make sure the path is correct)
    std::cout << "Starting playback...\n";

    // lets get a random song now (SongView)
    const music::SongView* song = appState.library->getRandomSong();
    if (song) {
        std::cout << "Playing random song: " << song->title << "\n";
        // Look up the concrete Song for filename using ID from SongView
        if (const music::SongView* songModel = appState.library->getSongById(song->id)) {
            appState.music_engine.playSound(songModel->filename);
        }
    } else {
        std::cerr << "No songs in library.\n";
    }

    // Cache display dimensions for render context (avoids recalculating every frame)
    graphics::RenderContext globalContext{};
    globalContext.screenWidth = al_get_display_width(al_get_current_display());
    globalContext.screenHeight = al_get_display_height(al_get_current_display());

    std::cout << "Starting graphics...\n";
    while (!finished) {
        al_wait_for_event(appState.event_queue, &appState.event);

        switch (appState.event.type)
        {
        case ALLEGRO_EVENT_AUDIO_STREAM_FINISHED:
            // TODO: expose currentStream so that we can verify it's the current stream
            if (appState.music_engine.onSongFinished) {
                appState.music_engine.onSongFinished();
            }
            break;
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            finished = true;
            break;
        case ALLEGRO_EVENT_TIMER:
            if (appState.event.timer.source == appState.graphics_timer && al_is_event_queue_empty(appState.event_queue)) {
                appState.music_engine.update(); // Update music engine (progress bar, etc.)
                nowPlayingView.setPosition(appState.music_engine.getCurrentTime());

                // Render frame
                al_clear_to_color(al_map_rgb(0, 0, 0));
                //al_hold_bitmap_drawing(true);
                //al_draw_text(appState.default_font, al_map_rgb(255, 255, 255), 200, 150, ALLEGRO_ALIGN_CENTRE, "Hello, Allegro!");

                sidebarView.draw(globalContext);
                if (activeLeftView == ui::LeftPanelView::Albums) {
                    albumListView.draw(globalContext);
                } else {
                    leftPlaceholderFrame.draw(globalContext);
                }
                playQueueView.draw(globalContext);
                nowPlayingView.draw(globalContext);
                //al_hold_bitmap_drawing(false);
                al_flip_display();
            } else if (appState.event.timer.source == appState.discord_callback_timer) {
                // Handle Discord callback timer event (used for initial setup only)
                if (!appState.discord_initialized.load()) {
                    if (appState.discord_integration.init()) {
                        appState.discord_initialized.store(true);
                        std::cout << "✅ Discord SDK initialized successfully.\n";
                        appState.discord_integration.authorize();
                    } else {
                        std::cerr << "❌ Failed to initialize Discord SDK.\n";
                    }
                }
                appState.discord_integration.run_callbacks();
            }
            break;
        case ALLEGRO_EVENT_KEY_DOWN:
        case ALLEGRO_EVENT_KEY_UP:
        case ALLEGRO_EVENT_KEY_CHAR:
        case ALLEGRO_EVENT_MOUSE_AXES:
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            appState.event_dispatcher.dispatchEvent(appState.event);
            break;
            /*
            if (appState.event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                finished = true;
            } else if (appState.event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                if (appState.music_engine.isPlaying()) {
                    appState.music_engine.pauseSound();
                } else {
                    appState.music_engine.resumeSound();
                }
            } else if (appState.event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
                // Play random song (onSongChanged callback handles UI updates)
                appState.music_engine.playNext();
                /*
                song = appState.library->getRandomSong();
                if (song) {
                    if (const music::SongView* songModel = appState.library->getSongById(song->id)) {
                        appState.music_engine.playSound(songModel->filename);
                    }
                }
                  
            }
            */
        case ALLEGRO_EVENT_UI:
            if (appState.event.user.data1 == ALLEGRO_EVENT_UI_PLAY_QUEUE_JUMP) {
                auto& pq = appState.music_engine.playQueueModel;
                size_t idx = static_cast<size_t>(appState.event.user.data2);

                if (pq && idx < pq->song_ids.size()) {
                    pq->current_index = idx;

                    const auto* song = appState.library->getSongById(pq->song_ids[idx]);
                    if (song) {
                        appState.music_engine.playSound(song->filename);
                    }
                }
            }
            break;
        case ALLEGRO_EVENT_DISPLAY_RESIZE:
            // Update cached display dimensions for render context
            al_acknowledge_resize(al_get_current_display());
            globalContext.screenWidth = al_get_display_width(al_get_current_display());
            globalContext.screenHeight = al_get_display_height(al_get_current_display());
            applyLayout(); // Re-apply layout to adjust to new size
            break;
        default:
            break;
        }
    }
}

} // namespace core
