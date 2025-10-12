#pragma once
#include "minimp3_ex.h"

#include <allegro5/allegro_audio.h>
#include <new>
#include <cstddef>
#include <cstdint>
#include <iostream>

static ALLEGRO_AUDIO_STREAM* stream_loader(const char* filename, size_t buffer_count, unsigned int samples) {
    mp3dec_ex_t dec;
    if (mp3dec_ex_open(&dec, filename, MP3D_SEEK_TO_SAMPLE)) {
        return nullptr; // Failed to open MP3 file
    }

    // Ensure we have valid stream info (hz/channels). If not, try to decode a small chunk to populate it.
    if (dec.info.hz == 0 || dec.info.channels == 0) {
        mp3d_sample_t tmpbuf[MINIMP3_PREDECODE_FRAMES * 2];
        (void)mp3dec_ex_read(&dec, tmpbuf, 1);
    }

    unsigned int hz = dec.info.hz ? dec.info.hz : 44100u;
    unsigned int channels = dec.info.channels ? dec.info.channels : 2u;

    ALLEGRO_CHANNEL_CONF chan_conf = (channels == 1) ? ALLEGRO_CHANNEL_CONF_1 : ALLEGRO_CHANNEL_CONF_2;

    ALLEGRO_AUDIO_STREAM* stream = al_create_audio_stream((int)buffer_count, samples, hz, ALLEGRO_AUDIO_DEPTH_INT16, chan_conf);
    if (!stream) {
        mp3dec_ex_close(&dec);
        return nullptr; // Failed to create audio stream
    }

    // Allocate one buffer we can reuse for fragments. Allegro copies fragment data internally.
    mp3d_sample_t* buffer = new (std::nothrow) mp3d_sample_t[samples * channels];
    if (!buffer) {
        al_destroy_audio_stream(stream);
        mp3dec_ex_close(&dec);
        return nullptr; // Memory allocation failure
    }

    size_t samples_read = 0;
    do {
        samples_read = mp3dec_ex_read(&dec, buffer, samples);
        if (samples_read > 0) {
            size_t bytes = samples_read * channels * sizeof(mp3d_sample_t);
                // Older Allegro versions have al_set_audio_stream_fragment(stream, data)
                // Newer versions use al_set_audio_stream_fragment(stream, data, size)
                // Use the two-arg form here for maximum compatibility with available headers.
                al_set_audio_stream_fragment(stream, buffer);
        }
    } while (samples_read > 0);

    delete[] buffer;
    mp3dec_ex_close(&dec);
    return stream;
}

// Implementation: check for ID3 header or MP3 frame sync (0xFF 0xE0..0xFF)
static bool mp3_sample_identifier(ALLEGRO_FILE* f)
{
    if (!f)
        return false;

    // Read the first 3 bytes to look for "ID3"
    char header[3];
    size_t read_bytes = al_fread(f, header, sizeof(header));
    if (read_bytes < sizeof(header))
        return false;

    return (header[0] == 'I' && header[1] == 'D' && header[2] == '3');
}

void addMP3Support() {
    // Register our identifier for .mp3 files
    auto result = al_register_sample_identifier(".mp3", mp3_sample_identifier);
    std::cout << "Sample identifier registration result: " << result << "\n";
    al_register_audio_stream_loader(".mp3", stream_loader);
}