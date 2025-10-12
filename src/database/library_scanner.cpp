#include "database/library_scanner.hpp"
#include "database/database.hpp"

#include <cstring>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <cctype>

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <allegro5/allegro_audio.h>

using namespace database;

// Helper: case-insensitive ends-with
static bool ends_with_ci(const std::string &s, const std::string &suffix)
{
    if (s.size() < suffix.size())
        return false;
    auto a = s.substr(s.size() - suffix.size());
    for (size_t i = 0; i < a.size(); ++i)
        a[i] = tolower(a[i]);
    auto b = suffix;
    for (auto &c : b)
        c = tolower(c);
    return a == b;
}

bool LibraryScanner::isAudioFile(const std::string &path)
{
    // Use Allegro's identifers when possible
    return al_identify_sample(path.c_str()) != nullptr;
}

bool LibraryScanner::isImageFile(const std::string &path)
{
    return al_identify_bitmap(path.c_str()) != nullptr;
}

// Allegro callback that wraps a C++ lambda
struct ScanState
{
    database::MusicDatabase *db;
    LibraryScanner *scanner;
    ScanOptions opts;
    ProgressCallback progress;
    std::atomic<bool> *cancel;
    size_t scanned = 0;
    size_t imported = 0;
    size_t skipped = 0;
    std::vector<std::string> files; // collected file paths
};

static int for_each_recursive_cb(ALLEGRO_FS_ENTRY *entry, void *vstate)
{
    auto *st = reinterpret_cast<ScanState *>(vstate);
    if (st->cancel && st->cancel->load())
        return ALLEGRO_FOR_EACH_FS_ENTRY_STOP;

    if (al_get_fs_entry_mode(entry) == ALLEGRO_FILEMODE_ISDIR)
        return ALLEGRO_FOR_EACH_FS_ENTRY_OK;

    const char *name = al_get_fs_entry_name(entry);
    std::string path(name);

    st->scanned++;
    if (!LibraryScanner::isAudioFile(path))
    {
        std::cerr << "Skipped non-audio file: " << path << "\n";
        st->skipped++;
        return ALLEGRO_FOR_EACH_FS_ENTRY_OK;
    }

    // Collect file for later batch processing
    st->files.push_back(path);
    if (st->progress)
        st->progress(st->scanned, st->imported);
    return ALLEGRO_FOR_EACH_FS_ENTRY_OK;
}

ScanResult LibraryScanner::scan(MusicDatabase &db,
                                const std::string &root_dir,
                                const ScanOptions &opts,
                                ProgressCallback progress,
                                std::atomic<bool> *cancel)
{
    ScanResult res;
    ALLEGRO_FS_ENTRY *dir = al_create_fs_entry(root_dir.c_str());
    if (!dir)
    {
        res.failures.emplace_back(root_dir, "failed to create fs entry");
        return res;
    }
    if (!al_fs_entry_exists(dir) || al_get_fs_entry_mode(dir) == ALLEGRO_FILEMODE_ISFILE)
    {
        res.failures.emplace_back(root_dir, "not a directory");
        al_destroy_fs_entry(dir);
        return res;
    }

    ScanState st;
    st.db = &db;
    st.scanner = this;
    st.opts = opts;
    st.progress = progress;
    st.cancel = cancel;

    // traverse and collect files
    al_for_each_fs_entry(dir, for_each_recursive_cb, &st);

    // Now process collected files grouped by album
    if (opts.transactional)
    {
        if (!db.beginTransaction())
        {
            res.failures.emplace_back(root_dir, "failed to begin transaction: " + db.lastError());
            al_destroy_fs_entry(dir);
            return res;
        }
    }

    // Group files by album key (album name + album artist). If no album, group by "__single__:<filepath>"
    std::unordered_map<std::string, std::vector<music::SongMetadata>> groups;
    for (auto &p : st.files)
    {
        if (st.cancel && st.cancel->load())
            break;
        music::SongMetadata meta = st.scanner->readSongMetadata(p);
        if (meta.title.empty())
            meta.title = std::filesystem::path(p).filename().string();
        std::string key;
        if (!meta.album.empty())
        {
            key = meta.album + "|" + meta.album_artist;
        }
        else
        {
            key = std::string("__single__:") + p;
        }
        groups[key].push_back(std::move(meta));
    }

    for (auto &kv : groups)
    {
        if (st.cancel && st.cancel->load())
            break;
        auto &vec = kv.second;
        // pick a representative to decide artist/album/genre
        auto &rep = vec.front();
        std::optional<int64_t> artist_id_opt;
        if (!rep.album_artist.empty())
            artist_id_opt = db.addArtist(rep.album_artist, "", "");
        if (!artist_id_opt && !rep.artist.empty())
            artist_id_opt = db.addArtist(rep.artist, "", "");
        int64_t artist_id = artist_id_opt ? *artist_id_opt : 0;

        std::optional<int64_t> album_id_opt;
        if (!rep.album.empty())
        {
            album_id_opt = db.addAlbum(rep.album, static_cast<int>(artist_id), "");
        }
        int64_t album_id = album_id_opt ? *album_id_opt : 0;

        // add genre if present
        std::optional<int64_t> genre_id_opt;
        if (!rep.genre.empty())
            genre_id_opt = db.addGenre(rep.genre);

        // insert songs in this group
        for (auto &meta : vec)
        {
            if (cancel && cancel->load())
                break;
            auto song_id_opt = db.addSong(meta.filepath, meta.title, static_cast<int>(album_id), static_cast<int>(meta.track), meta.comment, static_cast<int>(meta.duration));
            if (!song_id_opt)
            {
                res.failures.emplace_back(meta.filepath, db.lastError());
                st.skipped++;
                continue;
            }
            st.imported++;
            // associate artist(s)
            if (!meta.artist.empty())
            {
                auto aid = db.addArtist(meta.artist, "", "");
                if (aid)
                    db.addSongArtist(static_cast<int>(*song_id_opt), static_cast<int>(*aid));
            }
            if (!meta.genre.empty() && genre_id_opt)
            {
                db.addSongGenre(static_cast<int>(*song_id_opt), static_cast<int>(*genre_id_opt));
            }
        }
    }

    if (opts.transactional)
    {
        if (!res.failures.empty())
            db.rollback();
        else
            db.commit();
    }

    al_destroy_fs_entry(dir);

    res.scanned = st.scanned;
    res.imported = st.imported;
    res.skipped = st.skipped;
    return res;
}

music::SongMetadata LibraryScanner::readSongMetadata(const std::string &path)
{
    TagLib::FileRef f(path.c_str());
    music::SongMetadata meta;

    meta.filepath = path;
    if (!f.isNull() && f.tag())
    {
        TagLib::Tag *tag = f.tag();
        meta.title = tag->title().to8Bit(true);
        meta.artist = tag->artist().to8Bit(true);
        meta.album = tag->album().to8Bit(true);
        meta.year = tag->year();
        meta.comment = tag->comment().to8Bit(true);
        meta.track = tag->track();
        meta.duration = f.audioProperties()->lengthInSeconds();
        // meta.bpm = f.audioProperties()->bpm();
    }
    return meta;
}