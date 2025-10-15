#include "music/library.hpp"
#include <sqlite3.h>
#include "database/database.hpp"

namespace music {
bool Library::loadFromDatabase(database::MusicDatabase& db) {
    db.clearLastError();

    this->albums = db.getAllAlbums();
    this->artists = db.getAllArtists();
    this->genres = db.getAllGenres();
    this->playlists = db.getAllPlaylists();
    this->songs = db.getAllSongs();

    if (!db.lastError().empty()) {
        return false;
    }

    return true;
}
} // namespace music