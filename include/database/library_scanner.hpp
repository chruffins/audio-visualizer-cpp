#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <optional>
#include <cstdint>

#include <allegro5/allegro.h>

namespace database {
    class MusicDatabase;
}

namespace music {
struct SongMetadata {
    std::string filepath;
    std::string title;
    std::string artist;
    std::string album;
    std::string album_artist;
    std::string genre;
    std::string comment;
    
    uint32_t track;
    uint32_t disc;
    uint32_t year;
    uint32_t duration;

    float bpm;
};
}

namespace database {

struct ScanOptions {
	std::vector<std::string> extensions = { ".mp3", ".flac", ".wav", ".ogg" };
	bool transactional = true;
};

struct ScanResult {
	size_t scanned = 0;
	size_t imported = 0;
	size_t skipped = 0;
	std::vector<std::pair<std::string,std::string>> failures; // path, error
};

using ProgressCallback = std::function<void(size_t scanned, size_t imported)>;

class LibraryScanner {
public:
	LibraryScanner() = default;

	// Scan the directory (uses Allegro filesystem). Caller must ensure Allegro filesystem is initialized.
	ScanResult scan(MusicDatabase& db,
					const std::string& root_dir,
					const ScanOptions& opts = ScanOptions(),
					ProgressCallback progress = nullptr,
					std::atomic<bool>* cancel = nullptr);

	// helpers
	static bool isAudioFile(const std::string& path);
	static bool isImageFile(const std::string& path);
private:
    music::SongMetadata readSongMetadata(const std::string& path);
};

} // namespace database
