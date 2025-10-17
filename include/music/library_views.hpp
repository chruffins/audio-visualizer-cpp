#pragma once
#include <string>
#include <deque>
#include <vector>
#include <memory>
#include <unordered_map>
#include <ctime>

struct Song;
struct Album;
struct Playlist;
struct Genre;
struct Artist;

struct ALLEGRO_BITMAP;

namespace music {
class Artwork {
public:
    Artwork(const std::string& path);
    ~Artwork();

    ALLEGRO_BITMAP* get() const { return bitmap; }
private:
    ALLEGRO_BITMAP* bitmap;
};

class ArtworkCache {
public:
    std::shared_ptr<Artwork> load(const std::string& path);
private:
    std::unordered_map<std::string, std::weak_ptr<Artwork>> cache;
};

struct SongView {
    int id;
    std::string title;
    std::string album;
    std::string artist;
    std::string genre;
    std::string comment;
    std::string filename;
    int track_number;
    int disc;
    int year;
    int duration;

    SongView(int id, const std::string& title, const std::string& album,
             const std::string& artist, const std::string& genre,
             const std::string& comment, int track_number, int disc,
             int year, int duration, const std::string& filename) : id(id), title(title), album(album),
             artist(artist), genre(genre), comment(comment), track_number(track_number),
             disc(disc), year(year), duration(duration), filename(filename) {};
    // SongView(Song& song);
    std::string toString() const;
};

struct AlbumView {
    int id;
    int year;
    std::string title;
    std::string artist;
    std::vector<SongView> songs;
    std::shared_ptr<Artwork> artwork;

    AlbumView(int id, int year, const std::string& title, const std::string& artist,
              const std::vector<SongView>& songs, std::shared_ptr<Artwork> artwork)
        : id(id), year(year), title(title), artist(artist), songs(songs), artwork(artwork) {}
};

struct PlaylistView {
    int id;
    time_t created_at;
    std::string name;
    std::string description;
    std::vector<SongView> songs;
    std::shared_ptr<Artwork> artwork;

    PlaylistView(int id, time_t created_at, const std::string& name,
                 const std::string& description, const std::vector<SongView>& songs,
                 std::shared_ptr<Artwork> artwork)
        : id(id), created_at(created_at), name(name), description(description),
          songs(songs), artwork(artwork) {}
};

struct PlayQueueView {
    std::deque<SongView> songs;
    size_t current_index;
    bool is_shuffled;
    bool is_repeating;

    PlayQueueView(const std::deque<SongView>& songs, size_t current_index,
                  bool is_shuffled, bool is_repeating)
        : songs(songs), current_index(current_index),
          is_shuffled(is_shuffled), is_repeating(is_repeating) {}
};
}
