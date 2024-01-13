/* threads.c
   Rémi Attab (remi.attab@gmail.com), 17 Dec 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// threads
// -----------------------------------------------------------------------------

constexpr int threads_cpu_min = 4;

static struct
{
    size_t cpus;
    enum threads_profile profile;
} threads_config = {0};

struct threads_thread
{
    threads_id id;

    bool joined;
    atomic_bool join;

    pthread_t handle;

    threads_fn fn;
    void *ctx;
};

struct threads
{
    size_t it, first, last;
    struct threads_thread threads[];
};

static thread_local size_t threads_local_id = 0;

static void threads_pin(threads_id id)
{
    cpu_set_t set = {0};
    CPU_ZERO(&set);
    CPU_SET(id, &set);

    int ret = sched_setaffinity(0, sizeof(set), &set);
    if (ret == -1) errf_errno("unable to pin thread '%u'", id);
}

void threads_init(enum threads_profile profile)
{
    int cpus = get_nprocs();
    assert(cpus >= threads_cpu_min);

    threads_config.profile = profile;
    threads_config.cpus = legion_min((unsigned) cpus, threads_cpu_cap);

    threads_local_id = 0;
    threads_pin(0);
}

void threads_close(void) {}


static struct threads_thread *threads_get(struct threads *threads, threads_id id)
{
    assert(id >= threads->first && id < threads->last);
    return threads->threads + (id - threads->first);
}

struct threads *threads_alloc(enum threads_pool pool)
{
    size_t first = 0, last = 0;
    const size_t cpus = threads_config.cpus;

    switch (threads_config.profile)
    {

    case threads_profile_local: {
        switch (pool) {
        case threads_pool_nil:    { first = 0; last = 1; break; }
        case threads_pool_engine: { first = 0; last = 0; break; }
        case threads_pool_sound:  { first = 1; last = 2; break; }
        case threads_pool_sim:    { first = 2; last = 3; break; }
        case threads_pool_shards: { first = 3; last = cpus; break; }
        default: { assert(false); }
        }
        break;
    }

    case threads_profile_client: {
        switch (pool) {
        case threads_pool_nil:    { first = 0; last = 1; break; }
        case threads_pool_engine: { first = 1; last = 2; break; }
        case threads_pool_sound:  { first = 2; last = 3; break; }
        case threads_pool_sim:    { first = 0; last = 0; break; }
        case threads_pool_shards: { first = 0; last = 0; break; }
        default: { assert(false); }
        }
        break;
    }

    case threads_profile_server: {
        switch (pool) {
        case threads_pool_nil:    { first = 0; last = 1; break; }
        case threads_pool_engine: { first = 0; last = 0; break; }
        case threads_pool_sound:  { first = 0; last = 0; break; }
        case threads_pool_sim:    { first = 2; last = 3; break; }
        case threads_pool_shards: { first = 3; last = cpus; break; }
        default: { assert(false); }
        }
        break;
    }

    default: { assert(false); }
    }

    struct threads *threads = mem_struct_alloc_t(
            threads, threads->threads[0], last - first);
    *threads = (struct threads) { .it = first, .first = first, .last = last };
    return threads;
}

void threads_free(struct threads *threads)
{
    for (threads_id id = threads->first; id < threads->it; id++)
        threads_join(threads, id);
    mem_free(threads);
}


size_t thread_id(void)
{
    return threads_local_id;
}

size_t threads_cpus(struct threads *threads)
{
    return threads->last - threads->first;
}

threads_id threads_fork(struct threads *threads, threads_fn fn, void *ctx)
{
    threads_id id = threads->it++;
    struct threads_thread *thread = threads_get(threads, id);
    thread->id = id;
    thread->fn = fn;
    thread->ctx = ctx;

    void *run(void *ctx)
    {
        struct threads_thread *thread = ctx;
        threads_local_id = thread->id;
        threads_pin(thread->id);
        thread->fn(thread->ctx);
        atomic_store_explicit(&thread->join, true, memory_order_relaxed);
        return NULL;
    }

    int err = pthread_create(&thread->handle, nullptr, run, thread);
    if (!err) return id;

    failf_posix(err, "unable to create thread '%u'", id);
}

void threads_join(struct threads *threads, threads_id id)
{
    struct threads_thread *thread = threads_get(threads, id);
    if (thread->joined) return;

    threads_exit(threads, id);

    int err = pthread_join(thread->handle, NULL);
    if (err) errf_posix(err, "unable to join thread '%u'", id);

    thread->joined = true;
}

bool threads_done(struct threads *threads, threads_id id)
{
    struct threads_thread *thread = threads_get(threads, id);
    return atomic_load_explicit(&thread->join, memory_order_relaxed);
}

void threads_exit(struct threads *threads, threads_id id)
{
    struct threads_thread *thread = threads_get(threads, id);
    return atomic_store_explicit(&thread->join, true, memory_order_relaxed);
}
