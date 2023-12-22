/* threads.c
   Rémi Attab (remi.attab@gmail.com), 17 Dec 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// threads
// -----------------------------------------------------------------------------

constexpr int threads_cpu_min = 4;
constexpr size_t threads_cpu_cap = 64;


struct threads_pool_state
{
    threads_id it, first, last;
};

struct threads_thread
{
    threads_id id;

    threads_fn fn;
    void *ctx;

    pthread_t handle;
    atomic_bool join;
};

struct
{
    size_t len;
    struct threads_pool_state pools[threads_pool_len];
    struct threads_thread cpus[threads_cpu_cap];
} threads;

static void threads_pools_init(enum threads_profile profile)
{
    void assign(enum threads_pool pool, threads_id first, threads_id last)
    {
        struct threads_pool_state *state = threads.pools + pool;
        state->it = first;
        state->first = first;
        state->last = last;
    }

    switch (profile)
    {

    case threads_profile_local: {
        assign(threads_pool_nil,    0, 1);
        assign(threads_pool_engine, 0, 0);
        assign(threads_pool_sound,  1, 2);
        assign(threads_pool_sim,    2, 3);
        assign(threads_pool_shards, 2, threads.len);
        break;
    }

    case threads_profile_client: {
        assign(threads_pool_nil,    0, 1);
        assign(threads_pool_engine, 1, 2);
        assign(threads_pool_sound,  2, 3);
        assign(threads_pool_sim,    0, 0);
        assign(threads_pool_shards, 0, 0);
        break;
    }

    case threads_profile_server: {
        assign(threads_pool_nil,    0, 1);
        assign(threads_pool_engine, 0, 0);
        assign(threads_pool_sound,  0, 0);
        assign(threads_pool_sim,    2, 3);
        assign(threads_pool_shards, 2, threads.len);
        break;
    }

    default: { assert(false); }
    }
}

static void threads_pin(threads_id id)
{
    cpu_set_t set = {0};
    CPU_ZERO(&set);
    CPU_SET(id, &set);

    int ret = sched_setaffinity(0, sizeof(set), &set);
    if (ret == -1) errf_errno("unable to pin thread '%u'", id);
}

static threads_id threads_assign(enum threads_pool pool)
{
    struct threads_pool_state *state = threads.pools + pool;
    assert(state->it < threads.len);
    assert(state->it < state->last);
    assert(state->it >= state->first);
    return state->it++;
}

void threads_init(enum threads_profile profile)
{
    int cpus = get_nprocs();
    assert(cpus >= threads_cpu_min);

    threads.len = legion_min((unsigned) cpus, threads_cpu_cap);
    threads_pools_init(profile);
    threads_pin(threads_assign(threads_pool_nil));
}

void threads_close(void)
{
    for (threads_id id = 0; id < array_len(threads.cpus); ++id)
        if (threads.cpus[id].id) threads_join(id);
}

size_t threads_for(enum threads_pool pool)
{
    return threads.pools[pool].last - threads.pools[pool].first;
}

void threads_fork(enum threads_pool pool, threads_fn fn, void *ctx, threads_id *id)
{
    *id = threads_assign(pool);

    struct threads_thread *thread = threads.cpus + *id;
    thread->id = *id;
    thread->fn = fn;
    thread->ctx = ctx;

    void *run(void *thread_)
    {
        struct threads_thread *thread = thread_;

        threads_pin(thread->id);
        thread->fn(thread->ctx);
        threads_exit(thread->id);

        return NULL;
    }

    int err = pthread_create(&thread->handle, NULL, run, thread);
    if (!err) return;

    failf_posix(err, "unable to create thread '%u' for pool '%u'", *id, pool);
}

static struct threads_thread *threads_get(threads_id id)
{
    assert(id < threads_cpu_cap);

    struct threads_thread *thread = threads.cpus + id;
    assert(thread->id == id);

    return thread;
}

void threads_join(threads_id id)
{
    struct threads_thread *thread = threads_get(id);

    threads_exit(id);
    int err = pthread_join(thread->handle, NULL);
    if (err) errf_posix(err, "unable to join thread '%u'", thread->id);

    memset(thread, 0, sizeof(*thread));
}

bool threads_done(threads_id id)
{
    struct threads_thread *thread = threads_get(id);
    return atomic_load_explicit(&thread->join, memory_order_relaxed);
}

void threads_exit(threads_id id)
{
    struct threads_thread *thread = threads_get(id);
    return atomic_store_explicit(&thread->join, true, memory_order_relaxed);
}
