# forte player

A desktop music player built with C++. Has views for currently playing song, album library, visualizations, and album view. Built on top of the game programming library Allegro 5. As it is right now, I actually daily drive this over Spotify on desktop since it has all of the features I need to enjoy a music player.

<img width="1046" height="559" alt="image" src="https://github.com/user-attachments/assets/6953b440-2908-4edb-8457-db1237c3c395" />

## Features

- **Music Library Management**: Automatic scanning and importing of audio files with metadata extraction into a database
- **Album Artwork**: Extracts and displays embedded cover art from audio files (as well as external cover images)
- **Album Playback**: Interfaces for viewing and playing albums in your music library
- **Audio Visualizations**: See the song.
- **Playback Controls**: Play, pause, skip back/forward, loop, and progress tracking
- **Discord Rich Presence**: Display currently playing track on Discord (minus album art currently)
- **Last.fm Integration**: Automatically scrobbles your songs! No cheating though, the song has to be finished.
- **Format Support**: MP3 (insanely hacky solution), FLAC, OGG, WAV, and more via Allegro 5

## Dependencies

- **Allegro 5**: Graphics, audio, and input handling
- **SQLite3**: Music library database
- **TagLib**: Audio metadata extraction
- **Discord Game SDK**: Rich presence integration (I can't store the shared library in the repo. Kept in *my* `external/`, though.)

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
# Okay, I used MSYS2 for this. It should be the same as Linux actually...

# Build
cmake -S . -B build
cmake --build build

# Run (must run from build directory)
cd build
./audiovis
```

## Architecture

The application uses a singleton pattern (`AppState`) for global resource management. The music library maintains two layers:
- **Persistent**: SQLite CRUD operations via `MusicDatabase`
- **In-memory**: Cached entity maps and enriched views via `Library`

Audio playback uses Allegro's audio pipeline, managed by `MusicEngine`. There's a play queue which is used to determine what songs are played and when. The music engine also manages audio properties. The only one that's really currently leveraged is for volume controls.

UI components use a declarative hierarchy with UV coordinate positioning for responsive layouts. They can also utilize callbacks to consume events. This utilizes an event dispatcher which handles passing input events to UI elements. The UI library was handrolled by me, and is loosely based off of my imagining of how Visual Basic 6's UI works.

## Why C++?

This is a rewrite from C because:
- C kinda sucks when you don't want to handroll data structures
- Ownership and lifetime are annoying!
- Discord's Rich Presence API doesn't have C bindings.

## Future Work
There's a decent number of features I can add just because of the database schema. That would include searching and displaying genres, editing music metadata, creating, editing, and listening to playlists, and way more.
