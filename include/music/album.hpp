#pragma once
#include <string>

namespace music {
struct Album {
    int id;                 // Unique identifier for the album
    std::string title;      // Title of the album
    int year;               // Release year
    std::string cover_image;// Path to the cover image file
    int artist_id;          // Foreign key to the artist

    Album(int id, const std::string& title, int year, const std::string& cover_image, int artist_id)
        : id(id), title(title), year(year), cover_image(cover_image), artist_id(artist_id) {}

    // Function to use for printing album information
    std::string toString() const {
        return "Album[ID: " + std::to_string(id) + ", Title: " + title + ", Year: " + std::to_string(year) + ", Cover: " + cover_image + ", Artist ID: " + std::to_string(artist_id) + "]";
    }

    // Comparison operator for sorting albums by year, then title
    bool operator<(const Album& other) const {
        if (year != other.year) {
            return year < other.year;
        }
        return title < other.title;
    }
};
} // namespace music