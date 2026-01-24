#pragma once
#include <string>
#include <deque>
#include <vector>
#include <unordered_map>

#include "music/album.hpp"
#include "music/artist.hpp"
#include "music/genre.hpp"
#include "music/playlist.hpp"
#include "music/song.hpp"
#include "music/library_views.hpp"

// forward decl for database
namespace database {
class MusicDatabase;
}

namespace music {

// forward decls for music entities
struct Album;
struct Artist;
struct Genre;
struct Playlist;
struct Song;
struct sqlite3;

class Library {
public:
    Library() = default;
    ~Library() = default;

    // Load library from database connection
    bool loadFromDatabase(database::MusicDatabase& db);

    // Create views
    void recreateViews();

    // Get all entities (returns map for iteration)
    const std::unordered_map<int, Album>& getAllAlbums() const { return albums; }
    const std::unordered_map<int, Artist>& getAllArtists() const { return artists; }
    const std::unordered_map<int, Genre>& getAllGenres() const { return genres; }
    const std::unordered_map<int, Playlist>& getAllPlaylists() const { return playlists; }
    const std::unordered_map<int, Song>& getAllSongs() const { return songs; }

    // Get entities by ID (returns pointer; nullptr if not found)
    const SongView* getSongById(int id) const;
    const Album* getAlbumById(int id) const;
    const Artist* getArtistById(int id) const;
    const Genre* getGenreById(int id) const;
    const Playlist* getPlaylistById(int id) const;

    // Get views
    const std::vector<SongView>& getSongViews() const { return songViews; }
    const std::vector<AlbumView>& getAlbumViews() const { return albumViews; }
    const std::vector<PlaylistView>& getPlaylistViews() const { return playlistViews; }

    // Get random song view (returns pointer; nullptr if empty)
    const SongView* getRandomSong() const;
private:
    std::unordered_map<int, Album> albums;
    std::unordered_map<int, Artist> artists;
    std::unordered_map<int, Genre> genres;
    std::unordered_map<int, Playlist> playlists;
    std::unordered_map<int, Song> songs;

    std::vector<SongView> songViews;
    std::vector<AlbumView> albumViews;
    std::vector<PlaylistView> playlistViews;
};
};