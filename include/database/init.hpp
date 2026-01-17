#pragma once

const char* INIT_SQL = "CREATE TABLE IF NOT EXISTS songs (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "song_path TEXT NOT NULL UNIQUE, "\
    "title TEXT, " \
    "album_id INTEGER, " \
    "track INTEGER, " \
    "comment TEXT, " \
    "duration INTEGER, " \
    "FOREIGN KEY(album_id) REFERENCES albums(id) ON DELETE CASCADE);" \
    \
    "CREATE TABLE IF NOT EXISTS artists (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "name TEXT NOT NULL UNIQUE, " \
    "picture_path TEXT, " \
    "desc TEXT);" \
    \
    "CREATE TABLE IF NOT EXISTS albums (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "release_type INTEGER DEFAULT 1, " \
    "name TEXT NOT NULL, " \
    "year INTEGER, " \
    "picture_path TEXT, " \
    "cover_art_block BLOB, " \
    "cover_art_mime TEXT, " \
    "artist_id INTEGER, " \
    "FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE);" \
    \
    "CREATE TABLE IF NOT EXISTS genres(" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "name TEXT NOT NULL UNIQUE," \
    "parent_id INTEGER" \
    ");" \
    \
    "CREATE TABLE IF NOT EXISTS playlists (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "name TEXT NOT NULL, " \
    "picture_path TEXT," \
    "desc TEXT);" \
    \
    "CREATE TABLE IF NOT EXISTS album_types (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "name TEXT NOT NULL UNIQUE" \
    ");" \
    \
    "CREATE TABLE IF NOT EXISTS song_artists(" \
    "song_id INTEGER NOT NULL, " \
    "artist_id INTEGER NOT NULL, " \
    "FOREIGN KEY(song_id) REFERENCES songs(id) ON DELETE CASCADE, " \
    "FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE, " \
    "PRIMARY KEY(song_id, artist_id)" \
    ");" \
    \
    "CREATE TABLE IF NOT EXISTS song_genres (" \
    "song_id INTEGER NOT NULL, " \
    "genre_id INTEGER NOT NULL, " \
    "FOREIGN KEY(song_id) REFERENCES songs(id) ON DELETE CASCADE, " \
    "FOREIGN KEY(genre_id) REFERENCES genres(id) ON DELETE CASCADE, " \
    "PRIMARY KEY(song_id, genre_id));" \
    \
    "CREATE TABLE IF NOT EXISTS playlist_songs (" \
    "playlist_id INTEGER NOT NULL, " \
    "song_id INTEGER NOT NULL, " \
    "position INTEGER NOT NULL, " \
    "FOREIGN KEY(playlist_id) REFERENCES playlists(id) ON DELETE CASCADE, " \
    "FOREIGN KEY(song_id) REFERENCES songs(id) ON DELETE CASCADE, " \
    "PRIMARY KEY(playlist_id, position));";