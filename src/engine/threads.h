/* threads.h
   Rémi Attab (remi.attab@gmail.com), 17 Dec 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// threads
// -----------------------------------------------------------------------------

typedef uint8_t threads_id;
typedef void (* threads_fn) (void *ctx);

constexpr size_t threads_cpu_cap = 64;

enum threads_profile : unsigned
{
    threads_profile_nil = 0,
    threads_profile_local,
    threads_profile_client,
    threads_profile_server,
};

enum threads_pool : unsigned
{
    threads_pool_nil = 0,
    threads_pool_engine,
    threads_pool_sound,
    threads_pool_sim,
    threads_pool_shards,
    threads_pool_len,
};

struct threads;

void threads_init(enum threads_profile);
void threads_close(void);

struct threads *threads_alloc(enum threads_pool);
void threads_free(struct threads *);

size_t thread_id(void);
size_t threads_cpus(struct threads *);

threads_id threads_fork(struct threads *, threads_fn, void *ctx);
void threads_join(struct threads *, threads_id);

bool threads_done(struct threads *, threads_id);
void threads_exit(struct threads *, threads_id);
