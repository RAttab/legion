/* threads.h
   Rémi Attab (remi.attab@gmail.com), 17 Dec 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// threads
// -----------------------------------------------------------------------------

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

typedef uint8_t threads_id;
typedef void (* threads_fn) (void *ctx);

void threads_init(enum threads_profile);
void threads_close(void);

size_t threads_for(enum threads_pool);

void threads_fork(enum threads_pool, threads_fn, void *ctx, threads_id *id);
void threads_join(threads_id);

bool threads_done(threads_id);
void threads_exit(threads_id);
