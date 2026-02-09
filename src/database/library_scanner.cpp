#include "database/library_scanner.hpp"
#include "database/database.hpp"

#include <cstring>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <cctype>

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/mp4file.h>
#include <taglib/mp4coverart.h>
#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/opusfile.h>
#include <taglib/xiphcomment.h>
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
    const char* id = al_identify_sample(path.c_str());
    bool result = (id != nullptr);
    return result;
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

    if (al_get_fs_entry_mode(entry) & ALLEGRO_FILEMODE_ISDIR)
        return ALLEGRO_FOR_EACH_FS_ENTRY_OK;

    const char *name = al_get_fs_entry_name(entry);
    std::string path(name);

    st->scanned++;
    if (!LibraryScanner::isAudioFile(path))
    {
        // std::cerr << "Skipped non-audio file: " << path << "\n";
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
            // Extract cover art from the first song in the album (only once per album, not per song)
            std::vector<unsigned char> cover_data;
            std::string cover_mime;
            
            // Try embedded cover art from the first track
            auto embedded_cover = st.scanner->extractCoverArtFromFile(rep.filepath);
            if (embedded_cover)
            {
                cover_data = std::move(embedded_cover->first);
                cover_mime = std::move(embedded_cover->second);
            }
            
            // If no embedded cover art, try to find cover image in the album folder
            if (cover_data.empty() && !rep.filepath.empty())
            {
                std::string folder_path = std::filesystem::path(rep.filepath).parent_path().string();
                auto folder_cover = st.scanner->findAlbumCoverInFolder(folder_path);
                if (folder_cover)
                {
                    cover_data = std::move(folder_cover->first);
                    cover_mime = std::move(folder_cover->second);
                }
            }
            
            const std::vector<unsigned char>* cover_art_ptr = cover_data.empty() ? nullptr : &cover_data;
            
            album_id_opt = db.addAlbum(rep.album, static_cast<int>(artist_id), "", std::optional<int>(rep.year), cover_art_ptr, cover_mime);
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

    // Extract album_artist (but NOT cover art - handled per-album in scan())
    // MP3/ID3v2
    if (auto* mpegFile = dynamic_cast<TagLib::MPEG::File*>(f.file())) {
        if (mpegFile->ID3v2Tag()) {
            // Extract album artist (TPE2 frame)
            auto albumArtistFrames = mpegFile->ID3v2Tag()->frameListMap()["TPE2"];
            if (!albumArtistFrames.isEmpty()) {
                meta.album_artist = albumArtistFrames.front()->toString().to8Bit(true);
            }
        }
    }
    // FLAC
    else if (auto* flacFile = dynamic_cast<TagLib::FLAC::File*>(f.file())) {
        // Extract album artist from Xiph comment
        if (flacFile->xiphComment()) {
            auto fieldMap = flacFile->xiphComment()->fieldListMap();
            if (fieldMap.contains("ALBUMARTIST")) {
                meta.album_artist = fieldMap["ALBUMARTIST"].front().to8Bit(true);
            }
        }
    }
    // MP4/M4A
    else if (auto* mp4File = dynamic_cast<TagLib::MP4::File*>(f.file())) {
        if (mp4File->tag()) {
            auto itemMap = mp4File->tag()->itemMap();
            
            // Extract album artist
            if (itemMap.contains("aART")) {
                meta.album_artist = itemMap["aART"].toStringList().front().to8Bit(true);
            }
        }
    }
    // OGG Vorbis
    else if (auto* vorbisFile = dynamic_cast<TagLib::Ogg::Vorbis::File*>(f.file())) {
        if (vorbisFile->tag()) {
            auto xiphComment = vorbisFile->tag();
            
            // Extract album artist from Xiph comment
            auto fieldMap = xiphComment->fieldListMap();
            if (fieldMap.contains("ALBUMARTIST")) {
                meta.album_artist = fieldMap["ALBUMARTIST"].front().to8Bit(true);
            }
        }
    } else if (auto* opusFile = dynamic_cast<TagLib::Ogg::Opus::File*>(f.file())) {
        if (opusFile->tag()) {
            auto xiphComment = opusFile->tag();
            
            auto fieldMap = xiphComment->fieldListMap();
            if (fieldMap.contains("ALBUMARTIST")) {
                meta.album_artist = fieldMap["ALBUMARTIST"].front().to8Bit(true);
            }
        }   
    }

    return meta;
}

std::optional<std::pair<std::vector<unsigned char>, std::string>> LibraryScanner::extractCoverArtFromFile(const std::string &path)
{
    TagLib::FileRef f(path.c_str());
    
    // MP3/ID3v2
    if (auto* mpegFile = dynamic_cast<TagLib::MPEG::File*>(f.file())) {
        if (mpegFile->ID3v2Tag()) {
            auto frameList = mpegFile->ID3v2Tag()->frameListMap()["APIC"];
            if (!frameList.isEmpty()) {
                auto* coverFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
                auto picture = coverFrame->picture();
                std::vector<unsigned char> data(picture.data(), picture.data() + picture.size());
                std::string mime = coverFrame->mimeType().to8Bit(true);
                return std::make_pair(std::move(data), std::move(mime));
            }
        }
    }
    // FLAC
    else if (auto* flacFile = dynamic_cast<TagLib::FLAC::File*>(f.file())) {
        auto picList = flacFile->pictureList();
        if (!picList.isEmpty()) {
            auto* pic = picList.front();
            auto picture_data = pic->data();
            std::vector<unsigned char> data(picture_data.data(), picture_data.data() + picture_data.size());
            std::string mime = pic->mimeType().to8Bit(true);
            return std::make_pair(std::move(data), std::move(mime));
        }
    }
    // MP4/M4A
    else if (auto* mp4File = dynamic_cast<TagLib::MP4::File*>(f.file())) {
        if (mp4File->tag()) {
            auto itemMap = mp4File->tag()->itemMap();
            if (itemMap.contains("covr")) {
                auto coverList = itemMap["covr"].toCoverArtList();
                if (!coverList.isEmpty()) {
                    auto cover = coverList.front();
                    auto picture_data = cover.data();
                    std::vector<unsigned char> data(picture_data.data(), picture_data.data() + picture_data.size());
                    
                    std::string mime;
                    switch (cover.format()) {
                        case TagLib::MP4::CoverArt::JPEG:
                            mime = "image/jpeg";
                            break;
                        case TagLib::MP4::CoverArt::PNG:
                            mime = "image/png";
                            break;
                        default:
                            mime = "image/jpeg";
                            break;
                    }
                    return std::make_pair(std::move(data), std::move(mime));
                }
            }
        }
    }
    // OGG Vorbis
    else if (auto* vorbisFile = dynamic_cast<TagLib::Ogg::Vorbis::File*>(f.file())) {
        if (vorbisFile->tag()) {
            auto picList = vorbisFile->tag()->pictureList();
            if (!picList.isEmpty()) {
                auto* pic = picList.front();
                auto picture_data = pic->data();
                std::vector<unsigned char> data(picture_data.data(), picture_data.data() + picture_data.size());
                std::string mime = pic->mimeType().to8Bit(true);
                return std::make_pair(std::move(data), std::move(mime));
            }
        }
    }
    // OGG Opus
    else if (auto* opusFile = dynamic_cast<TagLib::Ogg::Opus::File*>(f.file())) {
        if (opusFile->tag()) {
            auto picList = opusFile->tag()->pictureList();
            if (!picList.isEmpty()) {
                auto* pic = picList.front();
                auto picture_data = pic->data();
                std::vector<unsigned char> data(picture_data.data(), picture_data.data() + picture_data.size());
                std::string mime = pic->mimeType().to8Bit(true);
                return std::make_pair(std::move(data), std::move(mime));
            }
        }
    }
    
    return std::nullopt;
}

std::optional<std::pair<std::vector<unsigned char>, std::string>> LibraryScanner::findAlbumCoverInFolder(const std::string &folder_path)
{
    // Common album cover filenames (case-insensitive)
    const std::vector<std::string> cover_names = {
        "cover", "folder", "front", "album", "albumart", "albumartsmall"
    };
    
    const std::vector<std::string> image_extensions = {
        ".jpg", ".jpeg", ".png", ".bmp", ".gif"
    };
    
    // Try to open the directory
    ALLEGRO_FS_ENTRY *dir = al_create_fs_entry(folder_path.c_str());
    if (!dir || !al_fs_entry_exists(dir) || !(al_get_fs_entry_mode(dir) & ALLEGRO_FILEMODE_ISDIR))
    {
        if (dir)
            al_destroy_fs_entry(dir);
        return std::nullopt;
    }
    
    if (!al_open_directory(dir))
    {
        al_destroy_fs_entry(dir);
        return std::nullopt;
    }
    
    std::optional<std::pair<std::vector<unsigned char>, std::string>> result;
    
    // Iterate through directory entries
    while (ALLEGRO_FS_ENTRY *entry = al_read_directory(dir))
    {
        if (al_get_fs_entry_mode(entry) & ALLEGRO_FILEMODE_ISDIR)
        {
            al_destroy_fs_entry(entry);
            continue;
        }
        
        const char *entry_name = al_get_fs_entry_name(entry);
        std::string filename(entry_name);
        std::string basename = std::filesystem::path(filename).stem().string();
        std::string extension = std::filesystem::path(filename).extension().string();
        
        // Convert to lowercase for comparison
        std::string basename_lower = basename;
        std::string extension_lower = extension;
        for (auto &c : basename_lower)
            c = tolower(c);
        for (auto &c : extension_lower)
            c = tolower(c);
        
        // Check if extension is an image
        bool is_image_ext = false;
        for (const auto &ext : image_extensions)
        {
            if (extension_lower == ext)
            {
                is_image_ext = true;
                break;
            }
        }
        
        if (!is_image_ext)
        {
            al_destroy_fs_entry(entry);
            continue;
        }
        
        // Check if filename matches common cover names
        bool is_cover = false;
        for (const auto &cover_name : cover_names)
        {
            if (basename_lower == cover_name)
            {
                is_cover = true;
                break;
            }
        }
        
        if (is_cover)
        {
            // Read the image file
            ALLEGRO_FILE *file = al_fopen(filename.c_str(), "rb");
            if (file)
            {
                int64_t file_size = al_fsize(file);
                if (file_size > 0 && file_size < 10 * 1024 * 1024) // Limit to 10MB
                {
                    std::vector<unsigned char> data(file_size);
                    if (al_fread(file, data.data(), file_size) == static_cast<size_t>(file_size))
                    {
                        // Determine MIME type from extension
                        std::string mime_type;
                        if (extension_lower == ".jpg" || extension_lower == ".jpeg")
                            mime_type = "image/jpeg";
                        else if (extension_lower == ".png")
                            mime_type = "image/png";
                        else if (extension_lower == ".bmp")
                            mime_type = "image/bmp";
                        else if (extension_lower == ".gif")
                            mime_type = "image/gif";
                        else
                            mime_type = "application/octet-stream"; // fallback
                        
                        result = std::make_pair(std::move(data), std::move(mime_type));
                        al_fclose(file);
                        al_destroy_fs_entry(entry);
                        break;
                    }
                }
                al_fclose(file);
            }
        }
        
        al_destroy_fs_entry(entry);
    }
    
    al_close_directory(dir);
    al_destroy_fs_entry(dir);
    
    return result;
}