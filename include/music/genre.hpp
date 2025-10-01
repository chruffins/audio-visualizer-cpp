#pragma once
#include <string>

namespace music {
struct Genre {
    int id;
    std::string name;

    Genre(int id, const std::string& name)
        : id(id), name(name) {}
    
    std::string toString() const {
        return "Genre[ID: " + std::to_string(id) + ", Name: " + name + "]";
    }

    bool operator<(const Genre& other) const {
        return name < other.name;
    }
};
} // namespace music