#pragma once
#include <string>

namespace music {
struct playlist {
    int id;
    std::string name;
    std::string image_path;
    std::string description;
    time_t created_at;

    playlist(int id, const std::string& name, const std::string& image_path,
             const std::string& description, time_t created_at)
        : id(id), name(name), image_path(image_path),
          description(description), created_at(created_at) {}
    
    playlist() : id(0), name(""), image_path(""), description(""), created_at(0) {}

    // for printing
    std::string toString() const {
        return "Playlist(id: " + std::to_string(id) +
                ", name: " + name +
                ", image_path: " + image_path +
                ", description: " + description +
                ", created_at: " + std::to_string(created_at) + ")";
    }

    // order by created_at
    bool operator<(const playlist& other) const {
        return created_at < other.created_at;
    }

    bool operator==(const playlist& other) const {
        return id == other.id;
    }
};
}