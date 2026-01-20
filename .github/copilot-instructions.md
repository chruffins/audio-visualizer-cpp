# Copilot Instructions

## Architecture Overview
Desktop audio player using **Allegro 5** (rendering/audio/IO), **SQLite3** (library DB), **TagLib** (metadata), and **Discord SDK** (rich presence). Entry point in `src/main.cpp` orchestrates initialization → scanning → event loop → shutdown.

## Critical Build/Run Workflow
**Windows (PowerShell)**:
```powershell
cmake -S . -B build; cmake --build build
cd build; ./audiovis  # Run from build dir - config.ini and discord_partner_sdk.dll copied here
```
- **Dependencies**: Allegro5, TagLib, SQLite3 via pkg-config/vcpkg; Discord SDK bundled in `external/discord-sdk/`
- **Platform gotchas**: Windows skips AddressSanitizer (line 13-16 in CMakeLists). Discord DLL auto-copied post-build (line 85-89)
- **MP3 support**: minimp3 header-only lib added via `mp3streaming::addMP3Support()` in `AppState::init()`

## Global State: The Singleton Pattern
**All** app resources live in `core::AppState::instance()` (Meyers singleton, thread-safe C++11). Never construct `MusicEngine`, `MusicDatabase`, `Library`, etc. directly.
- **Owned objects**: `display`, `event_queue`, timers (graphics 60Hz + Discord 1Hz), `MusicEngine`, `MusicDatabase`, `Library`, `PlayQueue`, `FontManager`, `DiscordIntegration`
- **Init flow** (`src/core/app_state.cpp:7-92`): 
  1. Load `config.ini`, create Allegro resources, start timers
  2. Initialize `MusicEngine` (voice→mixer→stream pipeline)
  3. Open DB (`music.db`), load `Library` into memory, populate `PlayQueue` with all song IDs
  4. Load fonts (`../assets/CourierPrime-Regular.ttf`)
  5. Auto-play first song if queue non-empty
- **Shutdown**: Destroy in reverse order (line 95-126); call `shutdown()` multiple times safely

## Data Model: Two-Tier Architecture
**Persistent layer** (`MusicDatabase`): Raw SQLite CRUD ops. Returns `optional<int64_t>` IDs or vectors of structs (`Song`, `Album`, `Artist`, `Genre`, `Playlist`). Junction tables: `song_artists`, `song_genres`, `playlist_songs`. Transactions **opt-in** (caller must `beginTransaction()`/`commit()`).

**In-memory layer** (`Library`): Maps `id → struct` caching all DB rows. Exposes **read-only views** (`SongView`, `AlbumView`) joining related data. Critical: `recreateViews()` must be called after any DB mutation outside `loadFromDatabase()` (line 51 in `src/music/library.cpp`).

**PlayQueue** (`include/music/play_queue.hpp`): Simple deque of song IDs with `next()`/`previous()`, shuffle, repeat. No playback logic—just index management. `MusicEngine` reads from it via `playQueueModel`.

## Audio Engine: Stream-Based Playback
`MusicEngine` (`src/core/music_engine.cpp`) wraps Allegro audio chain: `voice` (hw output) → `mixer` → `current_stream` (file). 
- `playSound(filepath)`: Destroys old stream, loads new via `al_load_audio_stream`, attaches to mixer, looks up `Song` in library by filename to fire `onSongChanged` callback (line 68-91)
- `update()`: Called every frame to update `current_time` from stream position and sync `ProgressBar` model (line 134-149)
- `playNext()`: Reads `playQueueModel->next()`, looks up `Song`, calls `playSound` (line 150-161)
- **Gotcha**: Filename matching is string-equality; if scanner normalizes paths differently, `onSongChanged` may not fire

## Library Scanning: Batch Import by Album
`LibraryScanner::scan()` (`src/database/library_scanner.cpp:96-341`):
1. Walk directory via `al_for_each_fs_entry`, filter audio with `al_identify_sample`
2. Read metadata (`TagLib::FileRef`), extract cover art from ID3v2/FLAC/MP4 (line 217-290)
3. **Group files by album key** (`album_title|album_artist`) to minimize DB artist/album duplication
4. Batch insert in single transaction if `opts.transactional` (default true)
- **Cover art**: Extracted once per album (first track), stored as BLOB in `albums.cover_image`

## UI: Declarative Hierarchy with UV Coordinates
`NowPlayingView` (`src/graphics/views/now_playing.cpp`) uses **UV positioning** (0.0-1.0 relative + pixel offsets):
- `FrameDrawable` (container) at bottom (`UV(0.0f, 1.0f, 0.0f, -101.0f)` = anchor to bottom-left, 101px tall)
- Children: `TextDrawable` (title/artist/album), `ProgressBarDrawable`, `ImageDrawable` (album art)
- **Sync pattern**: `MusicEngine::onSongChanged` lambda (line 41 in `src/main.cpp`) updates view when playback starts

## Event Loop: Multi-Source Allegro Queue
`main.cpp:run_main_loop()` (line 27-146):
- **Graphics timer** (60Hz): Clears screen, calls `music_engine.update()`, draws `NowPlayingView`, flips display
- **Discord timer** (1Hz): Lazy-inits SDK on first tick, then calls `run_callbacks()`
- **Keyboard**: `ESC` quit, `SPACE` pause/resume, `RIGHT` random song (via `Library::getRandomSong()`)

## Discord Integration: Lazy Init Pattern
`DiscordIntegration` (`src/core/discord_integration.cpp`) is a **singleton** like `AppState`. First Discord timer tick checks `discord_initialized` atomic flag, calls `init()` once (line 109-118 in `src/main.cpp`). Tokens saved/loaded from `discord_tokens.txt` (line 6-41). SDK requires callbacks polled regularly—never block in handler.

## Configuration: Allegro INI Format
`config.ini` (Windows paths use backslashes):
```ini
[paths]
music_directory = C:\Users\chris\Music
icon_path = ../assets/logotransparent.png
[discord]
application_id = 123456789012345678
```
Loaded via `al_load_config_file` in `Config::load()`. Defaults hardcoded if missing (see `src/util/config.cpp`).

## Adding Features: The Update Cycle
1. **DB changes**: Use `MusicDatabase` methods (return `optional<int64_t>` on success). If transactional, wrap in `beginTransaction()`/`commit()`
2. **In-memory sync**: Call `Library::recreateViews()` to rebuild `SongView`/`AlbumView` from updated maps
3. **Playback**: Modify `PlayQueue` (enqueue/shuffle), then `MusicEngine::playNext()` or `playSound(filepath)` directly
4. **UI updates**: Either poll in event loop or hook `MusicEngine::onSongChanged` callback (preferred for reactive updates)

## Common Pitfalls
- **Don't initialize singletons manually**: Always use `::instance()` for `AppState`, `DiscordIntegration`
- **Allegro lifecycle**: Destroy in reverse order of creation. If crashes on shutdown, check timer/event source cleanup order
- **Path handling**: Scanner stores filepaths as-is; `playSound` expects absolute paths. Mismatches break `onSongChanged` lookups
- **Missing views**: After DB insert, `Library` maps are updated but views are stale until `recreateViews()`
