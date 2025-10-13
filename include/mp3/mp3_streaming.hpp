#pragma once
#include "minimp3_ex.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <new>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <algorithm>

namespace mp3streaming
{
    /* Forward declare AUDIO_STREAM so the function pointer typedefs can refer to it. */
    struct AUDIO_STREAM;

    typedef size_t (*stream_callback_t)(mp3streaming::AUDIO_STREAM *, void *, size_t);
    typedef void (*unload_feeder_t)(mp3streaming::AUDIO_STREAM *);
    typedef bool (*rewind_feeder_t)(mp3streaming::AUDIO_STREAM *);
    typedef bool (*seek_feeder_t)(mp3streaming::AUDIO_STREAM *, double);
    typedef double (*get_feeder_position_t)(mp3streaming::AUDIO_STREAM *);
    typedef double (*get_feeder_length_t)(mp3streaming::AUDIO_STREAM *);
    typedef bool (*set_feeder_loop_t)(mp3streaming::AUDIO_STREAM *, double, double);

    typedef struct MP3FILE
    {
        mp3dec_t dec;

        uint8_t *file_buffer;      /* encoded MP3 file */
        int64_t file_size;         /* in bytes */
        int64_t next_frame_offset; /* next frame offset, in bytes */

        int file_pos;     /* position in samples */
        int file_samples; /* in samples */
        double loop_start;
        double loop_end;

        mp3d_sample_t frame_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME]; /* decoded MP3 frame */
        int frame_pos;                                             /* position in the frame buffer, in samples */

        int *frame_offsets; /* in bytes */
        int num_frames;
        int frame_samples; /* in samples, same across all frames */

        int freq;
        ALLEGRO_CHANNEL_CONF chan_conf;
    } MP3FILE;

    struct ALLEGRO_SAMPLE
    {
        ALLEGRO_AUDIO_DEPTH depth;
        ALLEGRO_CHANNEL_CONF chan_conf;
        unsigned int frequency;
        int len;
        void *buffer;
        bool free_buf;
        /* Whether `buffer' needs to be freed when the sample
         * is destroyed, or when `buffer' changes.
         */
        void *dtor_item;
    };

    /* Read some samples into a mixer buffer.
     *
     * source:
     *    The object to read samples from.  This may be one of several types.
     *
     * *vbuf: (in-out parameter)
     *    Pointer to pointer to destination buffer.
     *    (should confirm what it means to change the pointer on return)
     *
     * *samples: (in-out parameter)
     *    On input indicates the maximum number of samples that can fit into *vbuf.
     *    On output indicates the actual number of samples that were read.
     *
     * buffer_depth:
     *    The audio depth of the destination buffer.
     *
     * dest_maxc:
     *    The number of channels in the destination.
     */
    typedef void (*stream_reader_t)(void *source, void **vbuf,
                                    unsigned int *samples, ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc);

    typedef struct
    {
        union
        {
            ALLEGRO_MIXER *mixer;
            ALLEGRO_VOICE *voice;
            void *ptr;
        } u;
        bool is_voice;
    } sample_parent_t;

    struct ALLEGRO_SAMPLE_INSTANCE
    {
        /* ALLEGRO_SAMPLE_INSTANCE does not generate any events yet but ALLEGRO_AUDIO_STREAM
         * does, which can inherit only ALLEGRO_SAMPLE_INSTANCE. */
        ALLEGRO_EVENT_SOURCE es;

        ALLEGRO_SAMPLE spl_data;

        volatile bool is_playing;
        /* Is this sample is playing? */

        ALLEGRO_PLAYMODE loop;
        float speed;
        float gain;
        float pan;

        /* When resampling an audio stream there will be fractional sample
         * positions due to the difference in frequencies.
         */
        int pos;
        int pos_bresenham_error;

        int loop_start;
        int loop_end;

        int step;
        int step_denom;
        /* The numerator and denominator of the step are
         * stored separately. The actual step is obtained by
         * dividing step by step_denom */

        float *matrix;
        /* Used to convert from this format to the attached
         * mixers, if any.  Otherwise is NULL.
         * The gain is premultiplied in.
         */

        bool is_mixer;
        stream_reader_t spl_read;
        /* Reads sample data into the provided buffer, using
         * the specified format, converting as necessary.
         */

        ALLEGRO_MUTEX *mutex;
        /* Points to the parent object's mutex.  It is NULL if
         * the sample is not directly or indirectly attached
         * to a voice.
         */

        sample_parent_t parent;
        /* The object that this sample is attached to, if any.
         */
        void *dtor_item;
    };

    struct AUDIO_STREAM
    {
        ALLEGRO_SAMPLE_INSTANCE spl;
        /* ALLEGRO_AUDIO_STREAM is derived from
         * ALLEGRO_SAMPLE_INSTANCE.
         */

        unsigned int buf_count;
        /* The stream buffer is divided into a number of
         * fragments; this is the number of fragments.
         */

        void *main_buffer;
        /* Pointer to a single buffer big enough to hold all
         * the fragments. Each fragment has additional samples
         * at the start for linear/cubic interpolation.
         */

        void **pending_bufs;
        void **used_bufs;
        /* Arrays of offsets into the main_buffer.
         * The arrays are each 'buf_count' long.
         *
         * 'pending_bufs' holds pointers to fragments supplied
         * by the user which are yet to be handed off to the
         * audio driver.
         *
         * 'used_bufs' holds pointers to fragments which
         * have been sent to the audio driver and so are
         * ready to receive new data.
         */

        volatile bool is_draining;
        /* Set to true if sample data is not going to be passed
         * to the stream any more. The stream must change its
         * playing state to false after all buffers have been
         * played.
         */

        uint64_t consumed_fragments;
        /* Number of complete fragment buffers consumed since
         * the stream was started.
         */

        ALLEGRO_THREAD *feed_thread;
        ALLEGRO_MUTEX *feed_thread_started_mutex;
        ALLEGRO_COND *feed_thread_started_cond;
        bool feed_thread_started;
        volatile bool quit_feed_thread;
        mp3streaming::unload_feeder_t unload_feeder;
        mp3streaming::rewind_feeder_t rewind_feeder;
        mp3streaming::seek_feeder_t seek_feeder;
        mp3streaming::get_feeder_position_t get_feeder_position;
        mp3streaming::get_feeder_length_t get_feeder_length;
        mp3streaming::set_feeder_loop_t set_feeder_loop;
        mp3streaming::stream_callback_t feeder;
        /* If ALLEGRO_AUDIO_STREAM has been created by
         * al_load_audio_stream(), the stream will be fed
         * by a thread using the 'feeder' callback. Such
         * streams don't need to be fed by the user.
         */

        void *dtor_item;

        void *extra;
        /* Extra data for use by the flac/vorbis addons. */
    };

    /* Forward declaration so mp3_stream_close can call it before its definition. */
    static ALLEGRO_MUTEX *maybe_lock_mutex(ALLEGRO_MUTEX *mutex);
    static void maybe_unlock_mutex(ALLEGRO_MUTEX *mutex);
    ALLEGRO_CHANNEL_CONF _al_count_to_channel_conf(int num_channels);
    ALLEGRO_AUDIO_DEPTH _al_word_size_to_depth_conf(int word_size);
    static bool mp3_stream_seek(AUDIO_STREAM *stream, double time);
    static bool mp3_stream_rewind(AUDIO_STREAM *stream);
    static double mp3_stream_get_position(AUDIO_STREAM *stream);
    static double mp3_stream_get_length(AUDIO_STREAM *stream);
    static bool mp3_stream_set_loop(AUDIO_STREAM *stream, double start, double end);
    static size_t mp3_stream_update(AUDIO_STREAM *stream, void *data, size_t buf_size);
    static void mp3_stream_close(AUDIO_STREAM *stream);
    void *_al_kcm_feed_stream(ALLEGRO_THREAD *self, void *vstream);
    void _al_acodec_start_feed_thread(AUDIO_STREAM *stream);
    void _al_acodec_stop_feed_thread(AUDIO_STREAM *stream);

    static ALLEGRO_MUTEX *maybe_lock_mutex(ALLEGRO_MUTEX *mutex)
    {
        if (mutex)
        {
            al_lock_mutex(mutex);
        }
        return mutex;
    }

    static void maybe_unlock_mutex(ALLEGRO_MUTEX *mutex)
    {
        if (mutex)
        {
            al_unlock_mutex(mutex);
        }
    }

    ALLEGRO_CHANNEL_CONF _al_count_to_channel_conf(int num_channels)
    {
        switch (num_channels)
        {
        case 1:
            return ALLEGRO_CHANNEL_CONF_1;
        case 2:
            return ALLEGRO_CHANNEL_CONF_2;
        case 3:
            return ALLEGRO_CHANNEL_CONF_3;
        case 4:
            return ALLEGRO_CHANNEL_CONF_4;
        case 6:
            return ALLEGRO_CHANNEL_CONF_5_1;
        case 7:
            return ALLEGRO_CHANNEL_CONF_6_1;
        case 8:
            return ALLEGRO_CHANNEL_CONF_7_1;
        default:
            return (ALLEGRO_CHANNEL_CONF)0;
        }
    }

    ALLEGRO_AUDIO_DEPTH _al_word_size_to_depth_conf(int word_size)
    {
        switch (word_size)
        {
        case 1:
            return ALLEGRO_AUDIO_DEPTH_UINT8;
        case 2:
            return ALLEGRO_AUDIO_DEPTH_INT16;
        case 3:
            return ALLEGRO_AUDIO_DEPTH_INT24;
        case 4:
            return ALLEGRO_AUDIO_DEPTH_FLOAT32;
        default:
            return (ALLEGRO_AUDIO_DEPTH)0;
        }
    }

    static bool mp3_stream_seek(AUDIO_STREAM *stream, double time)
    {
        MP3FILE *mp3file = (MP3FILE *)stream->extra;
        int file_pos = time * mp3file->freq;
        int frame = file_pos / mp3file->frame_samples;
        /* It is necessary to start decoding a little earlier than where we are
         * seeking to, because frames will reuse decoder state from previous frames.
         * minimp3 assures us that 10 frames is sufficient. */
        int sync_frame = std::max(0, frame - 10);
        int frame_pos = file_pos - frame * mp3file->frame_samples;
        if (frame < 0 || frame > mp3file->num_frames)
        {
            return false;
        }
        int frame_offset = mp3file->frame_offsets[frame];
        int sync_frame_offset = mp3file->frame_offsets[sync_frame];

        mp3dec_frame_info_t frame_info;
        do
        {
            mp3dec_decode_frame(&mp3file->dec,
                                mp3file->file_buffer + sync_frame_offset,
                                mp3file->file_size - sync_frame_offset,
                                mp3file->frame_buffer, &frame_info);
            sync_frame_offset += frame_info.frame_bytes;
        } while (sync_frame_offset <= frame_offset);

        mp3file->next_frame_offset = frame_offset + frame_info.frame_bytes;
        mp3file->file_pos = file_pos;
        mp3file->frame_pos = frame_pos;

        return true;
    }

    static bool mp3_stream_rewind(AUDIO_STREAM *stream)
    {
        MP3FILE *mp3file = (MP3FILE *)stream->extra;

        return mp3_stream_seek(stream, mp3file->loop_start);
    }

    static double mp3_stream_get_position(AUDIO_STREAM *stream)
    {
        MP3FILE *mp3file = (MP3FILE *)stream->extra;

        return (double)mp3file->file_pos / mp3file->freq;
    }

    static double mp3_stream_get_length(AUDIO_STREAM *stream)
    {
        MP3FILE *mp3file = (MP3FILE *)stream->extra;

        return (double)mp3file->file_samples / mp3file->freq;
    }

    static bool mp3_stream_set_loop(AUDIO_STREAM *stream, double start, double end)
    {
        MP3FILE *mp3file = (MP3FILE *)stream->extra;
        mp3file->loop_start = start;
        mp3file->loop_end = end;
        return true;
    }

    /* mp3_stream_update:
     *  Updates 'stream' with the next chunk of data.
     *  Returns the actual number of bytes written.
     */
    static size_t mp3_stream_update(AUDIO_STREAM *stream, void *data,
                                    size_t buf_size)
    {
        MP3FILE *mp3file = (MP3FILE *)stream->extra;
        int sample_size = sizeof(mp3d_sample_t) * al_get_channel_count(mp3file->chan_conf);
        int samples_needed = buf_size / sample_size;
        ;
        double ctime = mp3_stream_get_position(stream);
        double btime = (double)samples_needed / mp3file->freq;

        if (stream->spl.loop != _ALLEGRO_PLAYMODE_STREAM_ONCE && ctime + btime > mp3file->loop_end)
        {
            samples_needed = (mp3file->loop_end - ctime) * mp3file->freq;
        }
        if (samples_needed < 0)
            return 0;

        mp3dec_t dec;
        mp3dec_init(&dec);

        int samples_read = 0;
        while (samples_read < samples_needed)
        {
            int samples_from_this_frame = std::min(
                mp3file->frame_samples - mp3file->frame_pos,
                samples_needed - samples_read);
            memcpy(data,
                   mp3file->frame_buffer + mp3file->frame_pos * al_get_channel_count(mp3file->chan_conf),
                   samples_from_this_frame * sample_size);

            mp3file->frame_pos += samples_from_this_frame;
            mp3file->file_pos += samples_from_this_frame;
            data = (char *)(data) + samples_from_this_frame * sample_size;
            samples_read += samples_from_this_frame;

            if (mp3file->frame_pos >= mp3file->frame_samples)
            {
                mp3dec_frame_info_t frame_info;
                int frame_samples = mp3dec_decode_frame(&mp3file->dec,
                                                        mp3file->file_buffer + mp3file->next_frame_offset,
                                                        mp3file->file_size - mp3file->next_frame_offset,
                                                        mp3file->frame_buffer, &frame_info);
                if (frame_samples == 0)
                {
                    mp3_stream_rewind(stream);
                    break;
                }
                mp3file->frame_pos = 0;
                mp3file->next_frame_offset += frame_info.frame_bytes;
            }
        }
        return samples_read * sample_size;
    }

    static void mp3_stream_close(AUDIO_STREAM *stream)
    {
        MP3FILE *mp3file = (MP3FILE *)stream->extra;

        /* Prototype for stop function is declared below; ensure it's visible. */
        _al_acodec_stop_feed_thread(stream);

        al_free(mp3file->frame_offsets);
        al_free(mp3file->file_buffer);
        al_free(mp3file);
        stream->extra = NULL;
        stream->feed_thread = NULL;
    }

    void *_al_kcm_feed_stream(ALLEGRO_THREAD *self, void *vstream)
    {
        AUDIO_STREAM *stream = (AUDIO_STREAM *)vstream;
        ALLEGRO_EVENT_QUEUE *queue;
        bool finished_event_sent = false;
        bool prefill = true;
        (void)self;

        queue = al_create_event_queue();
        al_register_event_source(queue, &stream->spl.es);

        stream->quit_feed_thread = false;

        while (!stream->quit_feed_thread)
        {
            char *fragment;
            ALLEGRO_EVENT event;

            if (!prefill)
                al_wait_for_event(queue, &event);

            if ((prefill || event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) && !stream->is_draining)
            {
                unsigned long bytes;
                unsigned long bytes_written;
                ALLEGRO_MUTEX *stream_mutex;

                fragment = (char *)al_get_audio_stream_fragment((ALLEGRO_AUDIO_STREAM *)stream);
                if (!fragment)
                {
                    /* This is not an error. */
                    continue;
                }

                bytes = (stream->spl.spl_data.len) *
                        al_get_channel_count(stream->spl.spl_data.chan_conf) *
                        al_get_audio_depth_size(stream->spl.spl_data.depth);

                stream_mutex = maybe_lock_mutex(stream->spl.mutex);
                bytes_written = stream->feeder(stream, fragment, bytes);
                maybe_unlock_mutex(stream_mutex);

                if (stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONEDIR)
                {
                    /* Keep rewinding until the fragment is filled. */
                    while (bytes_written < bytes &&
                           stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONEDIR)
                    {
                        size_t bw;
                        al_rewind_audio_stream((ALLEGRO_AUDIO_STREAM *)stream);
                        stream_mutex = maybe_lock_mutex(stream->spl.mutex);
                        bw = stream->feeder(stream, fragment + bytes_written,
                                            bytes - bytes_written);
                        bytes_written += bw;
                        maybe_unlock_mutex(stream_mutex);
                    }
                }
                else if (bytes_written < bytes)
                {
                    /* Fill the rest of the fragment with silence. */
                    int silence_samples = (bytes - bytes_written) /
                                          (al_get_channel_count(stream->spl.spl_data.chan_conf) *
                                           al_get_audio_depth_size(stream->spl.spl_data.depth));
                    al_fill_silence(fragment + bytes_written, silence_samples,
                                    stream->spl.spl_data.depth, stream->spl.spl_data.chan_conf);
                }

                if (!al_set_audio_stream_fragment((ALLEGRO_AUDIO_STREAM *)stream, fragment))
                {
                    continue;
                }

                /* The streaming source doesn't feed any more, so drain the stream.
                 * Don't quit in case the user decides to seek and then restart the
                 * stream. */
                if (bytes_written != bytes &&
                    (stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONCE ||
                     stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_LOOP_ONCE))
                {
                    /* Why not al_drain_audio_stream? We don't want to block on draining
                     * because the user might adjust the stream loop points and restart
                     * the stream. */
                    stream->is_draining = true;

                    if (!finished_event_sent)
                    {
                        ALLEGRO_EVENT fin_event;
                        fin_event.user.type = ALLEGRO_EVENT_AUDIO_STREAM_FINISHED;
                        fin_event.user.timestamp = al_get_time();
                        al_emit_user_event(&stream->spl.es, &fin_event, NULL);
                        finished_event_sent = true;
                    }
                }
                else
                {
                    finished_event_sent = false;
                }
            }
            else if (event.type == _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE)
            {
                ALLEGRO_EVENT fin_event;
                stream->quit_feed_thread = true;

                fin_event.user.type = ALLEGRO_EVENT_AUDIO_STREAM_FINISHED;
                fin_event.user.timestamp = al_get_time();
                al_emit_user_event(&stream->spl.es, &fin_event, NULL);
            }
            if (prefill)
            {
                al_lock_mutex(stream->feed_thread_started_mutex);
                stream->feed_thread_started = true;
                al_broadcast_cond(stream->feed_thread_started_cond);
                al_unlock_mutex(stream->feed_thread_started_mutex);
            }
            prefill = false;
        }

        al_destroy_event_queue(queue);

        return NULL;
    }

    void _al_acodec_start_feed_thread(AUDIO_STREAM *stream)
    {
        stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
        stream->feed_thread_started_cond = al_create_cond();
        stream->feed_thread_started_mutex = al_create_mutex();
        al_start_thread(stream->feed_thread);

        /* Need to wait for the thread to start, otherwise the quit event may be
         * sent before the event source is registered with the queue.
         *
         * This also makes the pre-fill system thread safe, as it needs to operate
         * before the mutexes are set up.
         */
        al_lock_mutex(stream->feed_thread_started_mutex);
        while (!stream->feed_thread_started)
        {
            al_wait_cond(stream->feed_thread_started_cond, stream->feed_thread_started_mutex);
        }
        al_unlock_mutex(stream->feed_thread_started_mutex);
    }

    void _al_acodec_stop_feed_thread(AUDIO_STREAM *stream)
    {
        ALLEGRO_EVENT quit_event;

        quit_event.type = _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE;
        al_emit_user_event(al_get_audio_stream_event_source((ALLEGRO_AUDIO_STREAM *)stream), &quit_event, NULL);
        al_join_thread(stream->feed_thread, NULL);
        al_destroy_thread(stream->feed_thread);
        al_destroy_cond(stream->feed_thread_started_cond);
        al_destroy_mutex(stream->feed_thread_started_mutex);

        stream->feed_thread = NULL;
    }

    ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream_f(ALLEGRO_FILE *f, size_t buffer_count, unsigned int samples)
    {
        MP3FILE *mp3file = (MP3FILE *)al_calloc(sizeof(MP3FILE), 1);
        mp3dec_init(&mp3file->dec);

        /* Variables declared up front to avoid crossing initializations with goto. */
        int frame_offset_capacity = 0;
        int offset_so_far = 0;
        size_t readbytes = 0;
        AUDIO_STREAM *stream = NULL;

        /* Read our file size. */
        mp3file->file_size = al_fsize(f);
        if (mp3file->file_size == -1)
        {
            goto failure;
        }

        /* Allocate buffer and read all the file. */
        mp3file->file_buffer = (uint8_t *)al_malloc(mp3file->file_size);
        readbytes = al_fread(f, mp3file->file_buffer, mp3file->file_size);
        if (readbytes != (size_t)mp3file->file_size)
        {
            goto failure;
        }

        /* Go through all the frames, to build the offset table. */
        while (true)
        {
            if (mp3file->num_frames + 1 > frame_offset_capacity)
            {
                frame_offset_capacity = mp3file->num_frames * 3 / 2 + 1;
                mp3file->frame_offsets = (int *)al_realloc(mp3file->frame_offsets,
                                                           sizeof(int) * frame_offset_capacity);
            }
            mp3dec_frame_info_t frame_info;
            int frame_samples = mp3dec_decode_frame(&mp3file->dec,
                                                    mp3file->file_buffer + offset_so_far,
                                                    mp3file->file_size - offset_so_far, NULL, &frame_info);
            if (frame_samples == 0)
            {
                if (mp3file->num_frames == 0)
                {
                    goto failure;
                }
                else
                {
                    break;
                }
            }
            /* Grab the file information from the first frame. */
            if (offset_so_far == 0)
            {
                mp3file->chan_conf = _al_count_to_channel_conf(frame_info.channels);
                mp3file->freq = frame_info.hz;
                mp3file->frame_samples = frame_samples;
            }

            mp3file->frame_offsets[mp3file->num_frames] = offset_so_far;
            mp3file->num_frames += 1;
            offset_so_far += frame_info.frame_bytes;
            mp3file->file_samples += frame_samples;
        }
        mp3file->loop_end = (double)mp3file->file_samples * mp3file->freq;

        stream = (mp3streaming::AUDIO_STREAM *)al_create_audio_stream(
            buffer_count, samples, mp3file->freq,
            _al_word_size_to_depth_conf(sizeof(mp3d_sample_t)),
            mp3file->chan_conf);
        if (!stream)
        {
            goto failure;
        }

        stream->extra = mp3file;
        stream->feeder = mp3_stream_update;
        stream->unload_feeder = mp3_stream_close;
        stream->rewind_feeder = mp3_stream_rewind;
        stream->seek_feeder = mp3_stream_seek;
        stream->get_feeder_position = mp3_stream_get_position;
        stream->get_feeder_length = mp3_stream_get_length;
        stream->set_feeder_loop = mp3_stream_set_loop;

        mp3_stream_rewind(stream);

        _al_acodec_start_feed_thread(stream);

        return (ALLEGRO_AUDIO_STREAM *)stream;
    failure:
        al_free(mp3file->frame_offsets);
        al_free(mp3file->file_buffer);
        al_free(mp3file);
        return NULL;
    }

    ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream(const char *filename, size_t buffer_count, unsigned int samples)
    {
        ALLEGRO_FILE *f;
        ALLEGRO_AUDIO_STREAM *stream;

        f = al_fopen(filename, "rb");
        if (!f)
        {
            return NULL;
        }

        stream = _al_load_mp3_audio_stream_f(f, buffer_count, samples);
        /* We load the entire file into memory. */
        al_fclose(f);

        return stream;
    }

    // Implementation: check for ID3 header or MP3 frame sync (0xFF 0xE0..0xFF)
    static bool mp3_sample_identifier(ALLEGRO_FILE *f)
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

    void addMP3Support()
    {
        // Register our identifier for .mp3 files
        auto result = al_register_sample_identifier(".mp3", mp3_sample_identifier);
        std::cout << "Sample identifier registration result: " << result << "\n";
        al_register_audio_stream_loader(".mp3", _al_load_mp3_audio_stream);
    }
} // namespace mp3streaming