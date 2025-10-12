#pragma once

#include <string>
#include <cstdint>
#include <sqlite3.h>
#include <optional>

class MusicDatabase {
public:
    explicit MusicDatabase(const std::string& dbPath);
    ~MusicDatabase();

    bool open();
    void close();

    // Execute arbitrary SQL (wrapper around sqlite3_exec)
    bool exec(const std::string& sql);

    // Schema creation helper (uses included init SQL)
    bool createSchema();

    // Transaction helpers
    bool beginTransaction();
    bool commit();
    bool rollback();

    // Add entities (return id on success, nullopt on error)
    std::optional<int64_t> addGenre(const std::string& name);
    std::optional<int64_t> addArtist(const std::string& name, const std::string& picture_path = "", const std::string& description = "");
    std::optional<int64_t> addPlaylist(const std::string& name, const std::string& picture_path = "", const std::string& description = "");
    std::optional<int64_t> addAlbum(const std::string& album_name, int64_t artist_id, const std::string& picture_path = "", std::optional<int> year = std::nullopt);
    std::optional<int64_t> addSong(const std::string& song_path, const std::string& title, int64_t album_id, int track, const std::string& comment, int duration);

    // Associations
    bool addSongArtist(int64_t song_id, int64_t artist_id);
    bool addSongGenre(int64_t song_id, int64_t genre_id);
    std::optional<int64_t> addPlaylistSong(int64_t playlist_id, int64_t song_id, int position);

    std::string lastError() const;

private:
    std::string path;
    sqlite3* db = nullptr;
    mutable std::string lastErr;

    bool addToJunctionTable(const std::string& table, const std::string& col1, const std::string& col2, int64_t id1, int64_t id2);
    int64_t getLastPositionInPlaylist(int64_t playlist_id);
};
