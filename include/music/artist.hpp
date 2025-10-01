#pragma once
#include <string>

namespace music {
struct Artist {
    int id;                 // Unique identifier for the artist
    std::string name;       // Name of the artist
    std::string image_path; // Path to the artist's image file
    std::string description; // Short biography or description

    Artist(int id, const std::string& name, const std::string& image_path, const std::string& description)
        : id(id), name(name), image_path(image_path), description(description) {}

    // Function to use for printing artist information
    std::string toString() const {
        return "Artist[ID: " + std::to_string(id) + ", Name: " + name + ", Image: " + image_path + ", Description: " + description + "]";
    }

    // Comparison operator for sorting artists by name
    bool operator<(const Artist& other) const {
        return name < other.name;
    }
};
} // namespace music