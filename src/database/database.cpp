#include "database/database.hpp"
#include "database/init.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

#include "music/album.hpp"
#include "music/artist.hpp"
#include "music/genre.hpp"
#include "music/playlist.hpp"
#include "music/song.hpp"

using namespace database;
MusicDatabase::MusicDatabase(const std::string &dbPath) : path(dbPath), db(nullptr) {}

MusicDatabase::~MusicDatabase() { close(); }

bool MusicDatabase::open() {
    if (db) return true;
    int rc = sqlite3_open(path.c_str(), &db);
    if (rc != SQLITE_OK) {
        lastErr = sqlite3_errmsg(db ? db : nullptr);
        if (db) sqlite3_close(db);
        db = nullptr;
        return false;
    }
    // Enable foreign keys
    exec("PRAGMA foreign_keys = ON;");

    // Ensure schema exists
    createSchema();
    return true;
}

void MusicDatabase::close() {
    if (!db) return;
    sqlite3_close(db);
    db = nullptr;
}

bool MusicDatabase::exec(const std::string &sql) {
    if (!db) { lastErr = "DB not open"; return false; }
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        lastErr = err ? err : "sqlite3_exec failed";
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

bool MusicDatabase::createSchema() {
    // INIT_SQL comes from include/database/init.hpp
    return exec(INIT_SQL);
}

bool MusicDatabase::beginTransaction() {
    return exec("BEGIN TRANSACTION;");
}

bool MusicDatabase::commit() {
    return exec("COMMIT;");
}

bool MusicDatabase::rollback() {
    return exec("ROLLBACK;");
}

std::string MusicDatabase::lastError() const {
    return lastErr;
}

void MusicDatabase::clearLastError() {
    lastErr = "";
}

std::optional<int64_t> MusicDatabase::addGenre(const std::string& name) {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* selectSql = "SELECT id FROM genres WHERE name = ?1";
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) { int64_t id = sqlite3_column_int64(stmt,0); sqlite3_finalize(stmt); return id; }
    sqlite3_finalize(stmt);

    const char* insertSql = "INSERT INTO genres (name) VALUES (?1);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) { lastErr = sqlite3_errmsg(db); sqlite3_finalize(stmt); return std::nullopt; }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db);
}

std::optional<int64_t> MusicDatabase::addArtist(const std::string& name, const std::string& picture_path, const std::string& description) {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* selectSql = "SELECT id FROM artists WHERE name = ?1";
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) { int64_t id = sqlite3_column_int64(stmt,0); sqlite3_finalize(stmt); return id; }
    sqlite3_finalize(stmt);

    const char* insertSql = "INSERT INTO artists (name, picture_path, desc) VALUES (?1, ?2, ?3);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, picture_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) { lastErr = sqlite3_errmsg(db); sqlite3_finalize(stmt); return std::nullopt; }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db);
}

std::optional<int64_t> MusicDatabase::addPlaylist(const std::string& name, const std::string& picture_path, const std::string& description) {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* selectSql = "SELECT id FROM playlists WHERE name = ?1";
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) { int64_t id = sqlite3_column_int64(stmt,0); sqlite3_finalize(stmt); return id; }
    sqlite3_finalize(stmt);

    const char* insertSql = "INSERT INTO playlists (name, picture_path, desc) VALUES (?1, ?2, ?3);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, picture_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) { lastErr = sqlite3_errmsg(db); sqlite3_finalize(stmt); return std::nullopt; }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db);
}

std::optional<int64_t> MusicDatabase::addAlbum(const std::string& album_name, int64_t artist_id, const std::string& picture_path, std::optional<int> year, const std::vector<unsigned char>* cover_art_data, const std::string& cover_art_mime) {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* selectSql = "SELECT id FROM albums WHERE name = ?1 AND artist_id IS ?2"; // artist may be null
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, album_name.c_str(), -1, SQLITE_TRANSIENT);
    if (artist_id > 0) sqlite3_bind_int(stmt, 2, artist_id); else sqlite3_bind_null(stmt, 2);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) { int64_t id = sqlite3_column_int64(stmt,0); sqlite3_finalize(stmt); return id; }
    sqlite3_finalize(stmt);

    const char* insertSql = "INSERT INTO albums (name, year, picture_path, artist_id, cover_art_block, cover_art_mime) VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, album_name.c_str(), -1, SQLITE_TRANSIENT);
    if (year.has_value()) sqlite3_bind_int(stmt, 2, year.value()); else sqlite3_bind_null(stmt, 2);
    sqlite3_bind_text(stmt, 3, picture_path.c_str(), -1, SQLITE_TRANSIENT);
    if (artist_id > 0) sqlite3_bind_int(stmt, 4, artist_id); else sqlite3_bind_null(stmt, 4);
    
    // Bind cover art BLOB if provided
    if (cover_art_data && !cover_art_data->empty()) {
        sqlite3_bind_blob(stmt, 5, cover_art_data->data(), static_cast<int>(cover_art_data->size()), SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, cover_art_mime.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 5);
        sqlite3_bind_null(stmt, 6);
    }
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) { lastErr = sqlite3_errmsg(db); sqlite3_finalize(stmt); return std::nullopt; }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db);
}

std::optional<int64_t> MusicDatabase::addSong(const std::string& song_path, const std::string& title, int64_t album_id, int track, const std::string& comment, int duration) {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    // check for existing song
    sqlite3_stmt* stmt = nullptr;
    const char* selectSql = "SELECT id FROM songs WHERE song_path = ?1";
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, song_path.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) { int64_t id = sqlite3_column_int64(stmt,0); sqlite3_finalize(stmt); return id; }
    sqlite3_finalize(stmt);

    const char* insertSql = "INSERT INTO songs (song_path, title, album_id, track, comment, duration) VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_text(stmt, 1, song_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    if (album_id > 0) sqlite3_bind_int(stmt, 3, album_id); else sqlite3_bind_null(stmt, 3);
    sqlite3_bind_int(stmt, 4, track);
    sqlite3_bind_text(stmt, 5, comment.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, duration);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) { lastErr = sqlite3_errmsg(db); sqlite3_finalize(stmt); return std::nullopt; }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db);
}

bool MusicDatabase::addToJunctionTable(const std::string& table, const std::string& col1, const std::string& col2, int64_t id1, int64_t id2) {
    if (!db) { lastErr = "DB not open"; return false; }
    
    sqlite3_stmt* stmt = nullptr;
    const std::string insertSql = "INSERT INTO " + table + " (" + col1 + ", " + col2 + ") VALUES (?1, ?2);";

    int rc = sqlite3_prepare_v2(db, insertSql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return false; }

    sqlite3_bind_int64(stmt, 1, id1);
    sqlite3_bind_int64(stmt, 2, id2);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) { lastErr = sqlite3_errmsg(db); sqlite3_finalize(stmt); return false; }

    sqlite3_finalize(stmt);
    return true;
}

bool MusicDatabase::addSongArtist(int64_t song_id, int64_t artist_id) {
    return addToJunctionTable("song_artists", "song_id", "artist_id", song_id, artist_id);
}

bool MusicDatabase::addSongGenre(int64_t song_id, int64_t genre_id) {
    return addToJunctionTable("song_genres", "song_id", "genre_id", song_id, genre_id);
}

std::optional<int64_t> MusicDatabase::addPlaylistSong(int64_t playlist_id, int64_t song_id, int position) {
    // we can't even add to junction table because we have to worry about the third field order
    // also we need to grab order if the order isn't ordered (means that we will insert immediately after)
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO playlist_songs (playlist_id, song_id, position) VALUES (?, ?, ?)";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    // means that we'll need to determine the position for ourselves
    if (position <= 0) {
        position = getLastPositionInPlaylist(playlist_id);
        if (position == -1) {
            lastErr = "failed to find good position value";
            return std::nullopt;
        }
        position += 1; // insert at the end
    }

    if (rc != SQLITE_OK) {
        lastErr = sqlite3_errmsg(db);
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, playlist_id);
    sqlite3_bind_int(stmt, 2, song_id);
    sqlite3_bind_int(stmt, 3, position);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        lastErr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    sqlite3_finalize(stmt);
    return position;
}

int64_t MusicDatabase::getLastPositionInPlaylist(int64_t playlist_id) {
    if (!db) { lastErr = "DB not open"; return -1; }

    sqlite3_stmt* stmt = nullptr;
    const char* selectSql = "SELECT MAX(position) FROM playlist_songs WHERE playlist_id = ?1";
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return -1; }
    sqlite3_bind_int64(stmt, 1, playlist_id);
    int rc = sqlite3_step(stmt);
    int64_t position = -1;
    if (rc == SQLITE_ROW) { position = sqlite3_column_int64(stmt, 0); }
    sqlite3_finalize(stmt);
    return position;
}

std::optional<music::Genre> MusicDatabase::getGenreById(int64_t id) const {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name FROM genres WHERE id = ?1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    std::optional<music::Genre> result = std::nullopt;
    if (rc == SQLITE_ROW) {
        int gid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* text = sqlite3_column_text(stmt, 1);
        std::string name = text ? reinterpret_cast<const char*>(text) : std::string();
        result = music::Genre(gid, name);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<music::Artist> MusicDatabase::getArtistById(int64_t id) const {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, picture_path, desc FROM artists WHERE id = ?1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    std::optional<music::Artist> result = std::nullopt;
    if (rc == SQLITE_ROW) {
        int aid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* nameTxt = sqlite3_column_text(stmt, 1);
        const unsigned char* picTxt = sqlite3_column_text(stmt, 2);
        const unsigned char* descTxt = sqlite3_column_text(stmt, 3);
        std::string name = nameTxt ? reinterpret_cast<const char*>(nameTxt) : std::string();
        std::string pic = picTxt ? reinterpret_cast<const char*>(picTxt) : std::string();
        std::string desc = descTxt ? reinterpret_cast<const char*>(descTxt) : std::string();
        result = music::Artist(aid, name, pic, desc);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<music::Album> MusicDatabase::getAlbumById(int64_t id) const {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, year, picture_path, artist_id, cover_art_block, cover_art_mime FROM albums WHERE id = ?1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    std::optional<music::Album> result = std::nullopt;
    if (rc == SQLITE_ROW) {
        int aid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* titleTxt = sqlite3_column_text(stmt, 1);
        int year = sqlite3_column_type(stmt,2) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,2);
        const unsigned char* picTxt = sqlite3_column_text(stmt, 3);
        int artist_id = sqlite3_column_type(stmt,4) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,4);
        
        std::string title = titleTxt ? reinterpret_cast<const char*>(titleTxt) : std::string();
        std::string pic = picTxt ? reinterpret_cast<const char*>(picTxt) : std::string();
        
        // Read cover art BLOB and MIME type
        std::vector<unsigned char> cover_art_data;
        std::string cover_art_mime;
        
        if (sqlite3_column_type(stmt, 5) == SQLITE_BLOB) {
            const void* blob = sqlite3_column_blob(stmt, 5);
            int blob_size = sqlite3_column_bytes(stmt, 5);
            if (blob && blob_size > 0) {
                cover_art_data.resize(blob_size);
                std::memcpy(cover_art_data.data(), blob, blob_size);
            }
        }
        
        const unsigned char* mimeTxt = sqlite3_column_text(stmt, 6);
        if (mimeTxt) {
            cover_art_mime = reinterpret_cast<const char*>(mimeTxt);
        }

        // TODO: fix this later???
        ui::ImageModel cover_art_model;
        if (!cover_art_data.empty()) {
            cover_art_model.loadFromMemory(cover_art_data.data(), cover_art_data.size(), cover_art_mime);
        }
        
        result = music::Album(aid, title, year, pic, artist_id, std::move(cover_art_model), cover_art_mime);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<music::Song> MusicDatabase::getSongById(int64_t id) const {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, song_path, title, album_id, track, comment, duration FROM songs WHERE id = ?1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    std::optional<music::Song> result = std::nullopt;
    if (rc == SQLITE_ROW) {
        int sid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* pathTxt = sqlite3_column_text(stmt, 1);
        const unsigned char* titleTxt = sqlite3_column_text(stmt, 2);
        int album_id = sqlite3_column_type(stmt,3) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,3);
        int track = sqlite3_column_type(stmt,4) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,4);
        const unsigned char* commentTxt = sqlite3_column_text(stmt, 5);
        int duration = sqlite3_column_type(stmt,6) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,6);
        std::string path = pathTxt ? reinterpret_cast<const char*>(pathTxt) : std::string();
        std::string title = titleTxt ? reinterpret_cast<const char*>(titleTxt) : std::string();
        std::string comment = commentTxt ? reinterpret_cast<const char*>(commentTxt) : std::string();
        result = music::Song(sid, path, title, album_id, track, comment, duration);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<music::Playlist> MusicDatabase::getPlaylistById(int64_t id) const {
    if (!db) { lastErr = "DB not open"; return std::nullopt; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, picture_path, desc FROM playlists WHERE id = ?1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return std::nullopt; }
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    std::optional<music::Playlist> result = std::nullopt;
    if (rc == SQLITE_ROW) {
        int pid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* nameTxt = sqlite3_column_text(stmt, 1);
        const unsigned char* picTxt = sqlite3_column_text(stmt, 2);
        const unsigned char* descTxt = sqlite3_column_text(stmt, 3);
        std::string name = nameTxt ? reinterpret_cast<const char*>(nameTxt) : std::string();
        std::string pic = picTxt ? reinterpret_cast<const char*>(picTxt) : std::string();
        std::string desc = descTxt ? reinterpret_cast<const char*>(descTxt) : std::string();
        time_t created_at = 0; // schema doesn't store created_at currently
    result = music::Playlist(pid, name, pic, desc, created_at);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<music::Genre> MusicDatabase::getAllGenres() const {
    std::vector<music::Genre> out;
    if (!db) { lastErr = "DB not open"; return out; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name FROM genres ORDER BY id ASC";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return out; }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int gid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* nameTxt = sqlite3_column_text(stmt, 1);
        std::string name = nameTxt ? reinterpret_cast<const char*>(nameTxt) : std::string();
        out.emplace_back(gid, name);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<music::Artist> MusicDatabase::getAllArtists() const {
    std::vector<music::Artist> out;
    if (!db) { lastErr = "DB not open"; return out; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, picture_path, desc FROM artists ORDER BY id ASC";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return out; }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int aid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* nameTxt = sqlite3_column_text(stmt, 1);
        const unsigned char* picTxt = sqlite3_column_text(stmt, 2);
        const unsigned char* descTxt = sqlite3_column_text(stmt, 3);
        std::string name = nameTxt ? reinterpret_cast<const char*>(nameTxt) : std::string();
        std::string pic = picTxt ? reinterpret_cast<const char*>(picTxt) : std::string();
        std::string desc = descTxt ? reinterpret_cast<const char*>(descTxt) : std::string();
        out.emplace_back(aid, name, pic, desc);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<music::Album> MusicDatabase::getAllAlbums() const {
    std::vector<music::Album> out;
    if (!db) { lastErr = "DB not open"; return out; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, year, picture_path, artist_id, cover_art_block, cover_art_mime FROM albums ORDER BY id ASC";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return out; }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int aid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* titleTxt = sqlite3_column_text(stmt, 1);
        int year = sqlite3_column_type(stmt,2) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,2);
        const unsigned char* picTxt = sqlite3_column_text(stmt, 3);
        int artist_id = sqlite3_column_type(stmt,4) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,4);
        
        std::string title = titleTxt ? reinterpret_cast<const char*>(titleTxt) : std::string();
        std::string pic = picTxt ? reinterpret_cast<const char*>(picTxt) : std::string();
        
        // Read cover art BLOB and MIME type
        std::vector<unsigned char> cover_art_data;
        std::string cover_art_mime;
        
        if (sqlite3_column_type(stmt, 5) == SQLITE_BLOB) {
            const void* blob = sqlite3_column_blob(stmt, 5);
            int blob_size = sqlite3_column_bytes(stmt, 5);
            if (blob && blob_size > 0) {
                cover_art_data.resize(blob_size);
                std::memcpy(cover_art_data.data(), blob, blob_size);
            }
        }
        
        const unsigned char* mimeTxt = sqlite3_column_text(stmt, 6);
        if (mimeTxt) {
            cover_art_mime = reinterpret_cast<const char*>(mimeTxt);
        }

        auto cover_art_model = ui::ImageModel();
        if (!cover_art_data.empty()) {
            cover_art_model.loadFromMemory(cover_art_data.data(), cover_art_data.size(), cover_art_mime);
        }
        
        out.emplace_back(aid, title, year, pic, artist_id, std::move(cover_art_model), cover_art_mime);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<music::Song> MusicDatabase::getAllSongs() const {
    std::vector<music::Song> out;
    if (!db) { lastErr = "DB not open"; return out; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, song_path, title, album_id, track, comment, duration FROM songs ORDER BY id ASC";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return out; }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int sid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* pathTxt = sqlite3_column_text(stmt, 1);
        const unsigned char* titleTxt = sqlite3_column_text(stmt, 2);
        int album_id = sqlite3_column_type(stmt,3) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,3);
        int track = sqlite3_column_type(stmt,4) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,4);
        const unsigned char* commentTxt = sqlite3_column_text(stmt, 5);
        int duration = sqlite3_column_type(stmt,6) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt,6);
        std::string path = pathTxt ? reinterpret_cast<const char*>(pathTxt) : std::string();
        std::string title = titleTxt ? reinterpret_cast<const char*>(titleTxt) : std::string();
        std::string comment = commentTxt ? reinterpret_cast<const char*>(commentTxt) : std::string();
        out.emplace_back(sid, path, title, album_id, track, comment, duration);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<music::Playlist> MusicDatabase::getAllPlaylists() const {
    std::vector<music::Playlist> out;
    if (!db) { lastErr = "DB not open"; return out; }
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, picture_path, desc FROM playlists ORDER BY id ASC";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) { lastErr = sqlite3_errmsg(db); return out; }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int pid = static_cast<int>(sqlite3_column_int64(stmt, 0));
        const unsigned char* nameTxt = sqlite3_column_text(stmt, 1);
        const unsigned char* picTxt = sqlite3_column_text(stmt, 2);
        const unsigned char* descTxt = sqlite3_column_text(stmt, 3);
        std::string name = nameTxt ? reinterpret_cast<const char*>(nameTxt) : std::string();
        std::string pic = picTxt ? reinterpret_cast<const char*>(picTxt) : std::string();
        std::string desc = descTxt ? reinterpret_cast<const char*>(descTxt) : std::string();
        time_t created_at = 0;
    out.emplace_back(pid, name, pic, desc, created_at);
    }
    sqlite3_finalize(stmt);
    return out;
}
