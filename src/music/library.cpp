#include "music/library.hpp"
#include <sqlite3.h>
#include <exception>
#include <stdexcept>
#include <random>
#include <algorithm>
#include "database/database.hpp"

namespace music {
bool Library::loadFromDatabase(database::MusicDatabase& db) {
    db.clearLastError();

    // load vectors from database
    auto albumVec = db.getAllAlbums();
    auto artistVec = db.getAllArtists();
    auto genreVec = db.getAllGenres();
    auto playlistVec = db.getAllPlaylists();
    auto songVec = db.getAllSongs();

    // convert to maps
    albums.clear();
    for (auto&& a : albumVec) {
        albums.emplace(a.id, std::move(a));
    }

    artists.clear();
    for (auto&& a : artistVec) {
        artists.emplace(a.id, std::move(a));
    }

    genres.clear();
    for (auto&& g : genreVec) {
        genres.emplace(g.id, std::move(g));
    }

    playlists.clear();
    for (auto&& p : playlistVec) {
        playlists.emplace(p.id, std::move(p));
    }

    songs.clear();
    for (auto&& s : songVec) {
        songs.emplace(s.id, std::move(s));
    }

    if (!db.lastError().empty()) {
        return false;
    }

    recreateViews();

    return true;
}

void Library::recreateViews() {
    // clear existing views
    songViews.clear();
    albumViews.clear();
    playlistViews.clear();

    // create SongViews - we need to join with album, artist, and genre data
    // note: since songs can have multiple artists/genres (junction tables),
    // we'll use the primary album artist and first genre for now
    
    for (const auto& [songId, song] : songs) {
        const Album* album = getAlbumById(song.album_id);
        std::string albumTitle = album ? album->title : "";
        std::string artistName = "";
        int year = album ? album->year : 0;
        int disc = 1; // Default disc number (not in current schema)
        
        if (album) {
            const Artist* artist = getArtistById(album->artist_id);
            if (artist) {
                artistName = artist->name;
            }
        }
        
        // for genre, we'd need to query song_genres junction table
        std::string genreName = "";
        
        songViews.emplace_back(
            song.id,
            song.title,
            albumTitle,
            artistName,
            genreName,
            song.comment,
            song.track_number,
            disc,
            year,
            song.duration,
            song.filename
        );
    }

    // create AlbumViews - group songs by album
    for (const auto& [albumId, album] : albums) {
        std::vector<SongView> albumSongs;
        
        // find all songs for this album
        for (const auto& songView : songViews) {
            // check if the original song belongs to this album
            const Song* song = getSongById(songView.id);
            if (song && song->album_id == albumId) {
                albumSongs.push_back(songView);
            }
        }
        
        // sort songs by track number
        std::sort(albumSongs.begin(), albumSongs.end(), 
                  [](const SongView& a, const SongView& b) {
                      return a.track_number < b.track_number;
                  });
        
        const Artist* artist = getArtistById(album.artist_id);
        std::string artistName = artist ? artist->name : "";
        
        // load artwork (lazy-loaded via cache when needed)
        std::shared_ptr<Artwork> artwork = nullptr;
        if (!album.cover_image.empty()) {
            // TODO: artwork loading would happen here with ArtworkCache
        }
        
        albumViews.emplace_back(
            album.id,
            album.year,
            album.title,
            artistName,
            albumSongs,
            artwork
        );
    }

    // Create PlaylistViews
    // Note: This requires querying the playlist_songs junction table from database
    // For now, create empty playlist views - a full implementation would need
    // database access or a way to pass that data in
    for (const auto& [playlistId, playlist] : playlists) {
        std::vector<SongView> playlistSongs; // Empty for now
        std::shared_ptr<Artwork> artwork = nullptr;
        
        playlistViews.emplace_back(
            playlist.id,
            playlist.created_at,
            playlist.name,
            playlist.description,
            playlistSongs,
            artwork
        );
    }
}

const Song* Library::getSongById(int id) const {
    auto it = songs.find(id);
    return (it != songs.end()) ? &it->second : nullptr;
}

const Album* Library::getAlbumById(int id) const {
    auto it = albums.find(id);
    return (it != albums.end()) ? &it->second : nullptr;
}

const Artist* Library::getArtistById(int id) const {
    auto it = artists.find(id);
    return (it != artists.end()) ? &it->second : nullptr;
}

const Genre* Library::getGenreById(int id) const {
    auto it = genres.find(id);
    return (it != genres.end()) ? &it->second : nullptr;
}

const Playlist* Library::getPlaylistById(int id) const {
    auto it = playlists.find(id);
    return (it != playlists.end()) ? &it->second : nullptr;
}

const SongView* Library::getRandomSong() const {
    if (songViews.empty()) {
        return nullptr;
    }
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, songViews.size() - 1);
    return &songViews[dis(gen)];
}

}; // namespace music