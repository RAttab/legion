/* metrics.h
   Rémi Attab (remi.attab@gmail.com), 06 Jan 2024
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

constexpr bool metrics_config_time = true;
constexpr sys_ts metrics_config_period = 5 * sys_sec;

// -----------------------------------------------------------------------------
// metric
// -----------------------------------------------------------------------------

struct metric { uint64_t n, t; };

#define metric_now() \
    ({ metrics_config_time ? sys_now() : (sys_ts) 0; })

#define metric_inc(_s, _f, _n, _t0)             \
    ({                                          \
        sys_ts _t1 = metric_now();              \
        (_s)->_f.t += _t1 - (_t0);              \
        (_s)->_f.n += (_n);                     \
        _t1;                                    \
    })


// -----------------------------------------------------------------------------
// metrics
// -----------------------------------------------------------------------------

struct metrics_shard
{
    legion_pad(64); // ensure that there's no false sharing between threads.

    bool active;
    struct { struct metric idle, chunks; } shard;
    struct {
        struct metric workers;
        struct metric active[items_active_len];
    } chunk;
};

struct metrics
{
    struct { sys_ts start, next; } t;
    struct { world_ts start, now; } ts;
    struct { struct metric lanes; } world;
    struct { struct metric idle, cmd, publish; } sim;
    struct { struct metric begin, wait, end; } shards;
    struct metrics_shard shard[shards_cap];
};

void metrics_open(const char *path);
void metrics_close(void);
void metrics_dump(struct metrics *metrics);
