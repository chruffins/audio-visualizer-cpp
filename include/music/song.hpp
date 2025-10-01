#pragma once
#include <string>

namespace music {
struct Song {
    int id;                 // Unique identifier for the song
    std::string filename; // Filename or path to the song file
    std::string title;    // Title of the song
    int album_id;
    int track_number;
    std::string comment;
    int duration; // Duration of the song in seconds

    Song(int id, const std::string& filename, const std::string& title, int album_id,
         int track_number, const std::string& comment, int duration)
        : id(id), filename(filename), title(title), album_id(album_id),
          track_number(track_number), comment(comment), duration(duration) {}
    
    Song() : id(0), filename(""), title(""), album_id(0), track_number(0), comment(""), duration(0) {}

    // for printing
    std::string toString() const {
        return "Song(id: " + std::to_string(id) +
               ", filename: " + filename +
               ", title: " + title +
               ", album_id: " + std::to_string(album_id) +
               ", track_number: " + std::to_string(track_number) +
               ", comment: " + comment +
               ", duration: " + std::to_string(duration) + "s)";
    }

    // track comparator
    bool operator<(const Song& other) const {
        return track_number < other.track_number;
    }

    bool operator==(const Song& other) const {
        return id == other.id;
    }
};
} // namespace music