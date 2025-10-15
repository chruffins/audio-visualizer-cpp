#pragma once
#include <string>
#include <deque>
#include <vector>

#include "music/album.hpp"
#include "music/artist.hpp"
#include "music/genre.hpp"
#include "music/playlist.hpp"
#include "music/song.hpp"

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

    // Get all entities
    const std::vector<Album>& getAllAlbums() const { return albums; }
    const std::vector<Artist>& getAllArtists() const { return artists; }
    const std::vector<Genre>& getAllGenres() const { return genres; }
    const std::vector<Playlist>& getAllPlaylists() const { return playlists; }
    const std::vector<Song>& getAllSongs() const { return songs; }
private:
    std::vector<Album> albums;
    std::vector<Artist> artists;
    std::vector<Genre> genres;
    std::vector<Playlist> playlists;
    std::vector<Song> songs;
};

}