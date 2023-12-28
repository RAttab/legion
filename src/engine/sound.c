/* sound.c
   Remi Attab (remi.attab@gmail.com), 19 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

// PCM encoding configured in the pcm target of the Makefile
constexpr uint16_t sound_sample_rate = 48000;
constexpr uint8_t sound_sample_channels = 2;

constexpr size_t sound_periods = 100;
constexpr size_t sound_period_samples = sound_sample_rate / sound_periods;

constexpr size_t sound_sfx_cap = 4;


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct sound_pcm
{
    enum render_sound type;
    const float *it, *start, *end;
};

struct
{
    snd_pcm_t *pcm;

    threads_id thread;
    struct threads *threads;

    struct { legion_atomic enum render_sound bgm, sfx[sound_sfx_cap]; } queue;
    struct { atomic_float master, sfx, bgm; atomic_bool paused; } mix;
    struct { struct sound_pcm bgm, sfx[sound_sfx_cap]; } pcms;

    struct
    {
        size_t index;
        float samples[sound_period_samples * sound_sample_channels];
    } out;
} sound;


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

// Will eventually implement a delta encoding for our PCM data. For now we're
// loading the data as is.
static struct sound_pcm sound_db(enum render_sound sound)
{
    struct sound_pcm make_pcm(
            enum render_sound type, const uint8_t *ptr, uint32_t len)
    {
        const float *start = (const float *) ptr;
        const float *end = (const float *) (ptr + len);
        return (struct sound_pcm) {
            .type = type,
            .it = start,
            .start = start,
            .end = end,
        };
    }

    switch (sound)
    {

    case sound_nil: {
        return (struct sound_pcm) { .type = sound_nil };
    }

    case bgm_piano: {
        extern const uint8_t db_pcm_bgm_piano_data[];
        extern const uint32_t db_pcm_bgm_piano_len;
        return make_pcm(sound, db_pcm_bgm_piano_data, db_pcm_bgm_piano_len);
    }

    case sfx_button: {
        extern const uint8_t db_pcm_sfx_button_data[];
        extern const uint32_t db_pcm_sfx_button_len;
        return make_pcm(sound, db_pcm_sfx_button_data, db_pcm_sfx_button_len);
    }

    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// sound
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
}

static void sound_close(void)
{
    snd_pcm_drop(sound.pcm);
    snd_pcm_close(sound.pcm);
}

static size_t sound_mix(void)
{
    constexpr size_t cap = array_len(sound.out.samples);
    if (likely(sound.out.index < cap)) return cap - sound.out.index;

    memset(&sound.out, 0, sizeof(sound.out));

    void mix(struct sound_pcm *pcm, float level, bool loop)
    {
        if (!pcm->type || level <= 0.0) return;

        for (size_t out = 0; out < cap; ) {
            const float *end = legion_min(pcm->it + (cap - out), pcm->end);

            for (; pcm->it < end; ++pcm->it, ++out)
                sound.out.samples[out] += *pcm->it * level;

            if (pcm->it == pcm->end) {
                pcm->it = pcm->start;
                if (!loop) { pcm->type = sound_nil; return; }
            }
        }
    }

    float mix_bgm = atomic_load_explicit(&sound.mix.bgm, memory_order_relaxed);
    bool mix_paused = atomic_load_explicit(&sound.mix.paused, memory_order_relaxed);
    if (!mix_paused) mix(&sound.pcms.bgm, mix_bgm, true);

    float mix_sfx = atomic_load_explicit(&sound.mix.sfx, memory_order_relaxed);
    for (size_t i = 0; i < array_len(sound.pcms.sfx); ++i)
        mix(sound.pcms.sfx + i, mix_sfx, false);

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
    if (bgm) sound.pcms.bgm = sound_db(bgm);

    for (size_t i = 0; i < array_len(sound.queue.sfx); ++i) {
        enum render_sound sfx = atomic_exchange_explicit(
                sound.queue.sfx + i, sound_nil, memory_order_relaxed);
        if (!sfx) continue;

        for (size_t j = 0; j < array_len(sound.pcms.sfx); ++j) {
            struct sound_pcm *pcm = sound.pcms.sfx + j;
            if (pcm->type) continue;
            *pcm = sound_db(sfx);
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

        constexpr time_sys delay = ts_sec / sound_periods / 2;
        time_sys next = ts_now() + delay;

        while (!threads_done(sound.threads, thread_id())) {
            ts_sleep_until(next);
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
