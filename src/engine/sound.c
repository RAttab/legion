/* sound.c
   Remi Attab (remi.attab@gmail.com), 19 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

// Global Config
constexpr size_t sound_sample_rate = 48000;
constexpr size_t sound_sample_channels = 2;

// Opus Config
constexpr size_t sound_frame_samples = 5760;
constexpr size_t sound_frame_len = sound_frame_samples * sound_sample_channels;

// ALSA Config
constexpr size_t sound_periods = 100;
constexpr size_t sound_period_samples = sound_sample_rate / sound_periods;

// Internal Config
constexpr size_t sound_sfx_cap = 4;


// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

#define fail_opus(error, msg) \
    failf("[opus] %s: %s (%d)", msg, opus_strerror(error), error)


// -----------------------------------------------------------------------------
// file
// -----------------------------------------------------------------------------

struct sound_file
{
    const char *name;
    enum render_sound type;
    const void *it, *start, *end;
};

struct sound_page
{
    const struct legion_packed
    {
        uint32_t capture;
        uint8_t version;
        uint8_t flags;
        uint64_t granule;
        uint32_t serial;
        uint32_t page_number;
        uint32_t crc32;
        uint8_t segments;
        uint8_t segment[];
    } *head;

    size_t len;
    const void *data;
};

static struct sound_page sound_next_page(struct sound_file *file)
{
    struct sound_page page = {0};

    page.head = file->it; file->it += sizeof(*page.head);
    if (file->it > file->end) fail("premature end of opus file.");

    if (page.head->capture != 0x5367674FU) // little-endian encoding of OggS
        failf("invalid opus capture '%08x' for '%s'", page.head->capture, file->name);

    if (page.head->version != 0x00)
        failf("invalid opus version '%02x' for '%s'", page.head->version, file->name);

    file->it += page.head->segments * sizeof(page.head->segment[0]);
    if (file->it > file->end)
        failf("premature eof for '%s'", file->name);

    for (size_t i = 0; i < page.head->segments; ++i)
        page.len += page.head->segment[i];

    page.data = file->it; file->it += page.len;
    if (file->it > file->end)
        failf("premature eof for '%s'", file->name);

    return page;
}

static struct sound_file sound_open_file(
        enum render_sound type,
        const char *name,
        const void *data, size_t len)
{
    struct sound_file file = {
        .type = type,
        .name = name,
        .it = data,
        .start = data,
        .end = data + len,
    };

    struct sound_page page = sound_next_page(&file);
    const struct legion_packed
    {
        uint64_t magic;
        uint8_t version;
        uint8_t channels;
        uint16_t pre_skip;
        uint32_t sample_rate;
        uint16_t gain;
        uint8_t family;
    } *head = page.data;

    if (page.len < sizeof(*head))
        failf("invalid opus head size '%zu' for '%s'", page.len, name);
    if (head->magic != 0x646165487375704FUL) // little-endian encoding of OpusHead
        failf("missing opus header for '%s'", name);
    if (head->version != 0x01)
        failf("invalid opus header version '%02x' for '%s'", head->version, name);
    if (head->channels != sound_sample_channels)
        failf("invalid opus channels '%02x' for '%s'", head->channels, name);
    if (head->sample_rate != sound_sample_rate)
        failf("invalid opus sample rate '%u' for '%s'", head->sample_rate, name);
    if (head->gain != 0)
        failf("invalid opus gain '%u' for '%s'", head->gain, name);
    if (head->family != 0)
        failf("invalid opus family '%u' for '%s'", head->family, name);

    page = sound_next_page(&file);
    const struct legion_packed { uint64_t magic; } *tags = page.data;

    if (page.len < sizeof(*tags))
        failf("invalid opus tags size '%zu' for '%s'", page.len, name);
    if (tags->magic != 0x736761547375704FUL) // little-ending encoding of OpusTags
        failf("missing opus header for '%s'", name);

    file.start = file.it;
    return file;
}

#define sound_populate_file(type)                               \
    ({                                                          \
        extern const uint8_t db_sound_ ## type ## _data[];      \
        extern const uint32_t db_sound_ ## type ## _len;        \
        sound_open_file(                                        \
                type, #type,                                    \
                db_sound_ ## type ## _data,                     \
                db_sound_ ## type ## _len);                     \
    })


// -----------------------------------------------------------------------------
// track
// -----------------------------------------------------------------------------

struct sound_track
{
    const float *it, *end;
    float *frame;
    OpusDecoder *opus;
    struct sound_file file;
    struct sound_page page;
    struct { size_t offset, index; } segment;
};

static void sound_track_init(struct sound_track *track)
{
    int ret = 0;
    track->opus = opus_decoder_create(sound_sample_channels, sound_sample_rate, &ret);
    if (ret != OPUS_OK) fail_opus(ret, "opus_decoder_create");

    track->frame = mem_array_alloc_t(*track->frame, sound_frame_len);
    track->it = track->end = track->frame;

    memset(&track->file, 0, sizeof(track->file));
}

static void sound_track_free(struct sound_track *track)
{
    opus_decoder_destroy(track->opus);
    mem_free(track->frame);
}

static void sound_track_reset(struct sound_track *track)
{
    int ret = opus_decoder_ctl(track->opus, OPUS_RESET_STATE);
    if (ret != OPUS_OK) fail_opus(ret, "opus_decoder_ctl(OPUS_RESET_STATE)");

    track->it = track->end = track->frame;
    track->file.it = track->file.start;

    memset(&track->page, 0, sizeof(track->page));
    track->segment.index = track->segment.offset = 0;
}

static void sound_track_set(
        struct sound_track *track,
        const struct sound_file *file)
{
    track->file = *file;
    sound_track_reset(track);
}

static void sound_track_mix(
        struct sound_track *track,
        float *const start, const float *const end,
        float level, bool loop)
{
    if (!track->file.type || level == 0.0) return;

    float *it = start;
    while (it < end) {
        if (track->it == track->end) {

            if (!track->page.head || track->segment.index == track->page.head->segments) {
                if (track->file.it < track->file.end) {
                    track->segment.offset = track->segment.index = 0;
                    track->page = sound_next_page(&track->file);
                }
                else if (loop) {
                    sound_track_reset(track);
                    track->page = sound_next_page(&track->file);
                }
                else {
                    memset(&track->file, 0, sizeof(track->file));
                    break;
                }
            }

            size_t segment_len = track->page.head->segment[track->segment.index];

            constexpr int fec = false;
            int samples = opus_decode_float(
                    track->opus,
                    track->page.data + track->segment.offset,
                    segment_len,
                    track->frame,
                    sound_frame_samples,
                    fec);
            if (samples < 0) fail_opus(samples, "opus_decode_float");
            assert(samples > 0);

            track->it = track->frame;
            track->end = track->it + (samples * sound_sample_channels);

            track->segment.offset += segment_len;
            track->segment.index++;
        }

        for (; it < end && track->it < track->end; ++it, ++track->it)
            *it += *track->it * level;
    }
}


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct
{
    snd_pcm_t *pcm;

    threads_id thread;
    struct threads *threads;

    struct { legion_atomic enum render_sound bgm, sfx[sound_sfx_cap]; } queue;
    struct { atomic_float master, sfx, bgm; atomic_bool paused; } mix;

    struct sound_file files[sound_files_len];
    struct { struct sound_track bgm, sfx[sound_sfx_cap]; } tracks;

    struct
    {
        size_t index;
        float samples[sound_period_samples * sound_sample_channels];
    } out;
} sound;



// -----------------------------------------------------------------------------
// alsa
// -----------------------------------------------------------------------------

#define err_snd(err, fn, msg)                                           \
    do {                                                                \
        fprintf(stderr, "%s:%u: <err.snd." fn "> " msg ": %s (%d)\n",   \
                __FILE__, __LINE__, snd_strerror(err), err);            \
    } while(false)

#define fail_snd(err, fn, msg)                                          \
    do {                                                                \
        err_snd(err, fn, msg);                                          \
        abort();                                                        \
    } while(false)

#define check_snd(ret, fn, msg)                         \
    do {                                                \
        typeof(ret) _ret = ret;                         \
        if (unlikely(_ret < 0)) fail_snd(_ret, fn, msg);    \
    } while(false)

static void sound_init(void)
{
    snd_pcm_t *pcm = nullptr;
    check_snd(
            snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK),
            "init", "snd_pcm_open");

    constexpr int snd_nonblock = 1;
    check_snd(snd_pcm_nonblock(pcm, snd_nonblock),
            "init", "snd_pcm_nonblock");

    snd_pcm_hw_params_t *hw = nullptr;
    snd_pcm_hw_params_alloca(&hw);

    check_snd(snd_pcm_hw_params_any(pcm, hw),
            "init", "snd_pcm_hw_params_any");
    check_snd(snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED),
            "init", "snd_pcm_hw_params_set_acess");
    check_snd(snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_FLOAT_LE),
            "init", "snd_pcm_hw_params_set_format");
    check_snd(snd_pcm_hw_params_set_channels(pcm, hw, sound_sample_channels),
            "init", "snd_pcm_hw_params_set_channels");
    check_snd(snd_pcm_hw_params_set_rate(pcm, hw, sound_sample_rate, 0),
            "init", "snd_pcm_hw_params_set_rate");
    check_snd(snd_pcm_hw_params_set_periods(pcm, hw, sound_periods, 0),
            "init", "snd_pcm_hw_params_set_periods");
    check_snd(snd_pcm_hw_params_set_period_size(pcm, hw, sound_period_samples, 0),
            "init", "snd_pcm_hw_params_set_period_size");
    check_snd(snd_pcm_hw_params(pcm, hw),
            "init", "snd_pcm_hw_params");

    sound.pcm = pcm;
    sound.out.index = array_len(sound.out.samples);

    sound.mix.master = 1.0;
    sound.mix.bgm = 1.0;
    sound.mix.sfx = 1.0;
    sound.mix.paused = true;

    sound_track_init(&sound.tracks.bgm);
    for (size_t i = 0; i < array_len(sound.tracks.sfx); ++i)
        sound_track_init(sound.tracks.sfx + i);

    sound.files[bgm_piano] = sound_populate_file(bgm_piano);
    sound.files[sfx_button] = sound_populate_file(sfx_button);
}

static void sound_close(void)
{
    sound_track_free(&sound.tracks.bgm);
    for (size_t i = 0; i < array_len(sound.tracks.sfx); ++i)
        sound_track_free(sound.tracks.sfx + i);

    snd_pcm_drop(sound.pcm);
    snd_pcm_close(sound.pcm);
}

static size_t sound_mix(void)
{
    constexpr size_t cap = array_len(sound.out.samples);
    if (likely(sound.out.index < cap)) return cap - sound.out.index;

    memset(&sound.out, 0, sizeof(sound.out));

    float *out = sound.out.samples;
    float *const end = sound.out.samples + cap;

    bool mix_paused = atomic_load_explicit(&sound.mix.paused, memory_order_relaxed);
    if (!mix_paused) {
        float mix_bgm = atomic_load_explicit(&sound.mix.bgm, memory_order_relaxed);
        sound_track_mix(&sound.tracks.bgm, out, end, mix_bgm, true);
    }

    float mix_sfx = atomic_load_explicit(&sound.mix.sfx, memory_order_relaxed);
    for (size_t i = 0; i < array_len(sound.tracks.sfx); ++i)
        sound_track_mix(sound.tracks.sfx + i, out, end, mix_sfx, false);

    float mix_master = atomic_load_explicit(&sound.mix.master, memory_order_relaxed);
    for (size_t i = 0; i < cap; ++i)
        sound.out.samples[i] = legion_bound(
                sound.out.samples[i] * mix_master, -1.0f, 1.0f);

    return cap;
}

static void sound_write(void)
{
    void recover(void)
    {
        check_snd(snd_pcm_recover(sound.pcm, -EPIPE, 1),
                "write", "snd_pcm_recover");
        err_snd(-EPIPE, "write", "snd_pcm_writei");
    }

    while (true) {
        int avail = snd_pcm_avail(sound.pcm);
        if (unlikely(avail < 0)) {
            if (avail == -EPIPE) { recover(); continue; }
            fail_snd(avail, "write", "snd_pcm_avail");
        }

        size_t buffered = sound_sample_rate - avail;
        if (buffered >= 2 * sound_period_samples) return;

        snd_pcm_uframes_t frames = sound_mix() / sound_sample_channels;
        snd_pcm_sframes_t ret = snd_pcm_writei(sound.pcm, sound.out.samples, frames);
        if (ret < 0) {
            if (ret == -EAGAIN) return;
            if (ret == -EPIPE) { recover(); continue; }
            fail_snd((int) ret, "write", "snd_pcm_writei");
        }

        sound.out.index += ret * sound_sample_channels;
        assert(sound.out.index <= array_len(sound.out.samples));
    }
}

static void sound_queue(void)
{
    enum render_sound bgm = atomic_exchange_explicit(
            &sound.queue.bgm, sound_nil, memory_order_relaxed);
    if (bgm) sound_track_set(&sound.tracks.bgm, sound.files + bgm);

    for (size_t i = 0; i < array_len(sound.queue.sfx); ++i) {
        enum render_sound sfx = atomic_exchange_explicit(
                sound.queue.sfx + i, sound_nil, memory_order_relaxed);
        if (!sfx) continue;

        for (size_t j = 0; j < array_len(sound.tracks.sfx); ++j) {
            struct sound_track *track = sound.tracks.sfx + j;
            if (track->file.type) continue;

            sound_track_set(track, sound.files + sfx);
            break;
        }
    }
}


// -----------------------------------------------------------------------------
// thread
// -----------------------------------------------------------------------------

static void sound_fork(void)
{
    void sound_loop(void *)
    {
        sound_init();

        constexpr sys_ts delay = sys_sec / sound_periods / 2;
        sys_ts next = sys_now() + delay;

        while (!threads_done(sound.threads, thread_id())) {
            sys_sleep_until(next);
            next += delay;

            sound_queue();
            sound_write();
        }

        sound_close();
    }

    sound.threads = threads_alloc(threads_pool_sound);
    sound.thread = threads_fork(sound.threads, sound_loop, nullptr);
}

static void sound_join(void)
{
    threads_join(sound.threads, sound.thread);
    threads_free(sound.threads);
}


// -----------------------------------------------------------------------------
// interface
// -----------------------------------------------------------------------------

float sound_mix_master(void)
{
    return atomic_load_explicit(&sound.mix.master, memory_order_relaxed);
}

void sound_mix_master_set(float level)
{
    assert(level >= 0.0f && level <= 1.0f);
    atomic_store_explicit(&sound.mix.master, level, memory_order_relaxed);
}

float sound_mix_sfx(void)
{
    return atomic_load_explicit(&sound.mix.sfx, memory_order_relaxed);
}

void sound_mix_sfx_set(float level)
{
    assert(level >= 0.0f && level <= 1.0f);
    atomic_store_explicit(&sound.mix.sfx, level, memory_order_relaxed);
}

float sound_mix_bgm(void)
{
    return atomic_load_explicit(&sound.mix.bgm, memory_order_relaxed);
}

void sound_mix_bgm_set(float level)
{
    assert(level >= 0.0f && level <= 1.0f);
    atomic_store_explicit(&sound.mix.bgm, level, memory_order_relaxed);
}

void sound_sfx_play(enum render_sound sfx)
{
    assert(sfx >= sound_sfx_first && sfx < sound_sfx_last);

    for (size_t i = 0; i < array_len(sound.queue.sfx); ++i) {
        enum render_sound exp = sound_nil;
        bool queued = atomic_compare_exchange_strong_explicit(
                sound.queue.sfx + i, &exp, sfx,
                memory_order_relaxed, memory_order_relaxed);
        if (queued) break;
    }
}

void sound_bgm_play(enum render_sound bgm)
{
    assert(bgm >= sound_bgm_first && bgm < sound_bgm_last);

    atomic_store_explicit(&sound.queue.bgm, bgm, memory_order_relaxed);
    sound_bgm_pause(false);
}

void sound_bgm_pause(bool paused)
{
    atomic_store_explicit(&sound.mix.paused, paused, memory_order_relaxed);
}

bool sound_bgm_paused(void)
{
    return atomic_load_explicit(&sound.mix.paused, memory_order_relaxed);
}
