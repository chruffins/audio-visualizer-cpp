# Audio Visualizer

A desktop audio player built with C++ featuring music library management, album artwork display, and Discord Rich Presence integration. Built on top of the game programming library Allegro.

## Features

- **Music Library Management**: Automatic scanning and importing of audio files with metadata extraction
- **Album Artwork**: Extracts and displays embedded cover art from audio files
- **Playback Controls**: Play, pause, skip, shuffle, and progress tracking
- **Discord Rich Presence**: Display currently playing track in Discord
- **Format Support**: MP3, FLAC, OGG, WAV, and more via Allegro audio codecs

## Dependencies

- **Allegro 5**: Graphics, audio, and input handling
- **SQLite3**: Music library database
- **TagLib**: Audio metadata extraction
- **Discord Game SDK**: Rich presence integration (bundled in `external/`)

## Building

### Linux

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install liballegro5-dev libsqlite3-dev libtag-dev

# Build
cmake -S . -B build
cmake --build build

# Run (must run from build directory)
cd build
./audiovis
```

### Windows

```powershell
# Install dependencies via vcpkg or pkg-config

# Build
cmake -S . -B build
cmake --build build

# Run (must run from build directory)
cd build
./audiovis
```

## Architecture

The application uses a singleton pattern (`AppState`) for global resource management. The music library maintains two layers:
- **Persistent**: Raw SQLite CRUD operations via `MusicDatabase`
- **In-memory**: Cached entity maps and enriched views via `Library`

Audio playback uses Allegro's voice→mixer→stream pipeline, managed by `MusicEngine`. UI components use a declarative hierarchy with UV coordinate positioning for responsive layouts.

## Why C++?

This is a rewrite from C because:
- C kinda sucks when you don't want to handroll data structures
- Ownership and lifetime are annoying!