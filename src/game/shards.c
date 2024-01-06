/* shards.c
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2023
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// sync
// -----------------------------------------------------------------------------

typedef atomic_size_t shard_sync;
typedef size_t shard_sync_epoch;

static_assert(sizeof(size_t) == 8);
constexpr size_t shard_sync_quit_mask = 1ULL << 63;
constexpr size_t shard_sync_epoch_bit = 1ULL << 8;

static shard_sync_epoch shard_sync_get_epoch(size_t value)
{
    return value & ~(shard_sync_epoch_bit - 1);
}

static uint8_t shard_sync_get_count(size_t value)
{
    return value & (shard_sync_epoch_bit - 1);
}

// main thread

static void shard_sync_init(shard_sync *sync)
{
    atomic_store_explicit(sync, shard_sync_epoch_bit - 1, memory_order_relaxed);
}

static void shard_sync_quit(shard_sync *sync)
{
    atomic_fetch_or_explicit(sync, shard_sync_quit_mask, memory_order_relaxed);
}

static void shard_sync_start(shard_sync *sync)
{
    size_t value = atomic_load_explicit(sync, memory_order_relaxed);
    value = shard_sync_get_epoch(value) + shard_sync_epoch_bit;
    atomic_store_explicit(sync, value, memory_order_release);
}

static void shard_sync_wait_end(shard_sync *sync, uint8_t shards)
{
    size_t value = 0;
    do {
        value = atomic_load_explicit(sync, memory_order_acquire);
    } while (shard_sync_get_count(value) < shards);
}

static void shard_sync_safe(shard_sync *sync, uint8_t shards)
{
    size_t value = atomic_load_explicit(sync, memory_order_relaxed);
    assert(value >= shards);
}

// shard thread

static bool shard_sync_wait_start(shard_sync *sync, shard_sync_epoch epoch)
{
    // When paused, we don't want to spin the thread needlessly. A futex would
    // make this more responsive but a simple 1ms sleep is a good-enough simple
    // approximation that will work in most cases.
    constexpr sys_ts period = 1 * sys_msec;
    sys_ts next = sys_now() + period;
    bool fast = true;

    while (true) {
        size_t value = atomic_load_explicit(sync, memory_order_acquire);
        if (value & shard_sync_quit_mask) return false;
        if (shard_sync_get_epoch(value) > epoch) return true;

        if (fast && sys_now() < next) continue;
        next += period; fast = false;
        sys_sleep_until(next);
    }

}

static shard_sync_epoch shard_sync_end(shard_sync *sync)
{
    size_t value = atomic_fetch_add_explicit(sync, 1, memory_order_release);
    return shard_sync_get_epoch(value);
}


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct shard_probe
{
    struct coord src, dst;
    enum item item;
    ssize_t value;
};

struct shard_scan
{
    struct coord src;
    struct scan_it it;
    struct coord value;
};

struct shard
{
    struct world *world;
    struct vec64 *chunks;
    struct save *out;

    threads_id thread;
    struct threads *threads;
    shard_sync *sync;

    struct
    {
        size_t len, cap;
        struct shard_probe *table;
    } probe;

    struct
    {
        size_t len, cap;
        struct shard_scan *table;
    } scan;
};

// -----------------------------------------------------------------------------
// init
// -----------------------------------------------------------------------------

struct shard *shard_alloc(struct world *world)
{
    struct shard *shard = calloc(1, sizeof(*shard));
    *shard = (struct shard) {
        .world = world,
        .out = save_mem_new(),
    };
    return shard;
}

void shard_free(struct shard *shard)
{
    vec64_free(shard->chunks);
    save_mem_free(shard->out);
    free(shard->probe.table);
    free(shard);
}

void shard_register(struct shard *shard, struct chunk *chunk)
{
    shard->chunks = vec64_append(shard->chunks, (uint64_t) chunk);
    chunk_shard(chunk, shard);
}

struct chunk *shard_chunk_alloc(
        struct shard *shard, const struct star *star, user_id user, vm_word name)
{
    struct chunk *chunk = chunk_alloc(star, user, name);
    shard_register(shard, chunk);
    return chunk;
}


// -----------------------------------------------------------------------------
// read-only
// -----------------------------------------------------------------------------

world_ts shard_time(const struct shard *shard)
{
    return world_time(shard->world);
}

const struct mods *shard_mods(struct shard *shard)
{
    return world_mods(shard->world);
}

const struct tech *shard_tech(struct shard *shard, user_id owner)
{
    return world_tech(shard->world, owner);
}


// -----------------------------------------------------------------------------
// read-write
// -----------------------------------------------------------------------------

void shard_user_io_push(struct shard *shard, user_id user, struct user_io packet)
{
    save_write_magic(shard->out, save_magic_io);
    save_write_value(shard->out, user);
    save_write_value(shard->out, packet.io);
    save_write_value(shard->out, packet.src);
    save_write_value(shard->out, packet.len);
    save_write(shard->out, packet.args, packet.len * sizeof(packet.args[0]));
    save_write_magic(shard->out, save_magic_io);
}

static void shard_user_io_pop(struct shard *shard)
{
    user_id user = save_read_type(shard->out, typeof(user));

    struct user_io *packet = world_user_io(shard->world, user);
    save_read_into(shard->out, &packet->io);
    save_read_into(shard->out, &packet->src);
    save_read_into(shard->out, &packet->len);
    save_read(shard->out, packet->args, packet->len * sizeof(packet->args[0]));
}


void shard_log_push(struct shard *shard, user_id user, struct log_line log)
{
    save_write_magic(shard->out, save_magic_log);
    save_write_value(shard->out, user);
    save_write_value(shard->out, log.star);
    save_write_value(shard->out, log.time);
    save_write_value(shard->out, log.id);
    save_write_value(shard->out, log.key);
    save_write_value(shard->out, log.value);
    save_write_magic(shard->out, save_magic_log);
}

static void shard_log_pop(struct shard *shard)
{
    user_id user = save_read_type(shard->out, typeof(user));

    struct log_line log = {0};
    save_read_into(shard->out, &log.star);
    save_read_into(shard->out, &log.time);
    save_read_into(shard->out, &log.id);
    save_read_into(shard->out, &log.key);
    save_read_into(shard->out, &log.value);

    log_push(world_log(shard->world, user), log);
}


void shard_tech_push(struct shard *shard, user_id user, enum item item, uint8_t bit)
{
    save_write_magic(shard->out, save_magic_tech);
    save_write_value(shard->out, user);
    save_write_value(shard->out, item);
    save_write_value(shard->out, bit);
    save_write_magic(shard->out, save_magic_tech);
}

static void shard_tech_pop(struct shard *shard)
{
    user_id user = save_read_type(shard->out, typeof(user));
    enum item item = save_read_type(shard->out, typeof(item));
    uint8_t bit = save_read_type(shard->out, typeof(bit));

    tech_learn_bit(world_tech(shard->world, user), item, bit);
}


void shard_lanes_push(struct shard *shard, struct lanes_packet packet)
{
    save_write_magic(shard->out, save_magic_lanes);
    save_write_value(shard->out, packet.owner);
    save_write_value(shard->out, packet.item);
    save_write_value(shard->out, packet.len);
    save_write_value(shard->out, packet.speed);
    save_write_value(shard->out, packet.src);
    save_write_value(shard->out, packet.dst);
    save_write(shard->out, packet.data, packet.len * sizeof(*packet.data));
    save_write_magic(shard->out, save_magic_lanes);
}

static void shard_lanes_pop(struct shard *shard)
{
    struct lanes_packet packet = {0};
    save_read_into(shard->out, &packet.owner);
    save_read_into(shard->out, &packet.item);
    save_read_into(shard->out, &packet.len);
    save_read_into(shard->out, &packet.speed);
    save_read_into(shard->out, &packet.src);
    save_read_into(shard->out, &packet.dst);

    packet.data = save_bytes(shard->out) + save_len(shard->out);
    save_read_skip(shard->out, packet.len * sizeof(*packet.data));

    lanes_launch(world_lanes(shard->world), packet);
}


void shard_probe_push(
        struct shard *shard,
        struct coord src,
        struct coord dst,
        enum item item)
{
    save_write_magic(shard->out, save_magic_probe);
    save_write_value(shard->out, src);
    save_write_value(shard->out, dst);
    save_write_value(shard->out, item);
    save_write_magic(shard->out, save_magic_probe);
}

static struct shard_probe *shard_probe_append(struct shard *shard)
{
    if (shard->probe.len == shard->probe.cap) {
        size_t old = shard->probe.cap;
        shard->probe.table = realloc_zero(
                shard->probe.table,
                old, shard->probe.cap = old ? old * 2 : 4,
                sizeof(*shard->probe.table));
    }

    return shard->probe.table + shard->probe.len++;
}

static void shard_probe_pop(struct shard *shard)
{
    struct shard_probe *probe = shard_probe_append(shard);
    save_read_into(shard->out, &probe->src);
    save_read_into(shard->out, &probe->dst);
    save_read_into(shard->out, &probe->item);
}

ssize_t shard_probe_get(const struct shard *shard, struct coord coord, enum item item)
{
    for (size_t i = 0; i < shard->probe.len; ++i) {
        const struct shard_probe *probe = shard->probe.table + i;
        if (item != probe->item && !coord_eq(coord, probe->dst)) continue;
        return probe->value;
    }

    assert(false);
}


void shard_scan_push(struct shard *shard, struct coord src, struct scan_it it)
{
    save_write_magic(shard->out, save_magic_scan);
    save_write_value(shard->out, src);
    save_write_value(shard->out, it);
    save_write_magic(shard->out, save_magic_scan);
}

static struct shard_scan *shard_scan_append(struct shard *shard)
{
    if (shard->scan.len == shard->scan.cap) {
        size_t old = shard->scan.cap;
        shard->scan.table = realloc_zero(
                shard->scan.table,
                old, shard->scan.cap = old ? old * 2 : 4,
                sizeof(*shard->scan.table));
    }

    return shard->scan.table + shard->scan.len++;
}

static void shard_scan_pop(struct shard *shard)
{
    struct shard_scan *scan = shard_scan_append(shard);
    save_read_into(shard->out, &scan->src);
    save_read_into(shard->out, &scan->it);
}

struct coord shard_scan_get(struct shard *shard, struct scan_it it)
{
    for (size_t i = 0; i < shard->scan.len; ++i) {
        const struct shard_scan *scan = shard->scan.table + i;
        if (!scan_it_eq(scan->it, it)) continue;
        return scan->value;
    }

    assert(false);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void shard_begin(struct shard *shard)
{
    for (size_t i = 0; i < shard->probe.len; ++i) {
        struct shard_probe *probe = shard->probe.table + i;
        probe->value = world_probe(shard->world, probe->dst, probe->item);
    }

    for (size_t i = 0; i < shard->scan.len; ++i) {
        struct shard_scan *scan = shard->scan.table + i;
        scan->value = world_scan(shard->world, scan->it);
    }

    save_mem_reset(shard->out);
}

static void shard_exec(struct shard *shard)
{
    for (size_t i = 0; i < vec64_len(shard->chunks); ++i)
        chunk_step((struct chunk *)shard->chunks->vals[i]);
}

static void shard_end(struct shard *shard)
{
    shard->probe.len = 0;

    size_t len = save_len(shard->out);
    save_mem_reset(shard->out);

    while (save_len(shard->out) < len) {
        enum save_magic magic = save_read_type(shard->out, typeof(magic));

        switch (magic)
        {
        case save_magic_io: { shard_user_io_pop(shard); break; }
        case save_magic_lanes: { shard_lanes_pop(shard); break; }
        case save_magic_log: { shard_log_pop(shard); break; }
        case save_magic_tech: { shard_tech_pop(shard); break; }
        case save_magic_probe: { shard_probe_pop(shard); break; }
        case save_magic_scan: { shard_scan_pop(shard); break; }
        default: { assert(false); }
        }

        assert(save_read_magic(shard->out, magic));
    }
}

void shard_step(struct shard *shard)
{
    shard_begin(shard);
    shard_exec(shard);
    shard_end(shard);
}


// -----------------------------------------------------------------------------
// thread
// -----------------------------------------------------------------------------

static void shard_thread_run(void *ctx)
{
    struct shard *shard = ctx;
    shard_sync_epoch epoch = 0;

    while (shard_sync_wait_start(shard->sync, epoch)) {
        shard_exec(ctx);
        epoch = shard_sync_end(shard->sync);
    }
}

static struct shard *shard_thread_alloc(
        struct world *world, struct threads *threads, shard_sync *sync)
{
    struct shard *shard = shard_alloc(world);

    shard->sync = sync;
    shard->threads = threads;
    shard->thread = threads_fork(threads, shard_thread_run, shard);

    return shard;
}

static void shard_thread_free(struct shard *shard)
{
    threads_join(shard->threads, shard->thread);
    shard_free(shard);
}


// -----------------------------------------------------------------------------
// shards
// -----------------------------------------------------------------------------

struct shards
{
    struct threads *threads;
    struct world *world;
    shard_sync sync;

    size_t len, active;
    struct shard *shards[threads_cpu_cap];
};

struct shards *shards_alloc(struct world *world)
{
    struct shards *shards = calloc(1, sizeof(*shards));
    shards->threads = threads_alloc(threads_pool_shards);
    shards->world = world;
    shard_sync_init(&shards->sync);
    shards->len = threads_cpus(shards->threads);
    return shards;
}

void shards_free(struct shards *shards)
{
    shard_sync_safe(&shards->sync, shards->active);

    shard_sync_quit(&shards->sync);
    for (size_t i = 0; i < shards->len; ++i) {
        struct shard *shard = shards->shards[i];
        if (shard) shard_thread_free(shard);
    }
    threads_free(shards->threads);
}

struct shard *shards_get(struct shards *shards, struct coord coord)
{
    shard_sync_safe(&shards->sync, shards->active);

    size_t index = hash_u64(coord_to_u64(coord)) % shards->len;

    struct shard **shard = shards->shards + index;
    if (!*shard) {
        *shard = shard_thread_alloc(shards->world, shards->threads, &shards->sync);
        shards->active++;
    }

    return *shard;
}

void shards_register(struct shards *shards, struct chunk *chunk)
{
    struct shard *shard = shards_get(shards, chunk_star(chunk)->coord);
    shard_register(shard, chunk);
}

void shards_step(struct shards *shards)
{
    shard_sync_safe(&shards->sync, shards->active);

    for (size_t i = 0; i < shards->len; ++i) {
        struct shard *shard = shards->shards[i];
        if (shard) shard_begin(shard);
    }

    shard_sync_start(&shards->sync);
    shard_sync_wait_end(&shards->sync, shards->active);

    for (size_t i = 0; i < shards->len; ++i) {
        struct shard *shard = shards->shards[i];
        if (shard) shard_end(shard);
    }
}

void shards_save(struct shards *shards, struct save *save)
{
    shard_sync_safe(&shards->sync, shards->active);
    save_write_magic(save, save_magic_shards);

    for (size_t i = 0; i < shards->len; ++i) {
        struct shard *shard = shards->shards[i];
        if (!shard) continue;

        for (size_t j = 0; j < shard->probe.len; ++j) {
            struct shard_probe *probe = shard->probe.table + j;
            save_write_value(save, (uint8_t) 0xFF);
            save_write_value(save, probe->src);
            save_write_value(save, probe->dst);
            save_write_value(save, probe->item);
        }
    }
    save_write_value(save, (uint8_t) 0x00);

    for (size_t i = 0; i < shards->len; ++i) {
        struct shard *shard = shards->shards[i];
        if (!shard) continue;

        for (size_t j = 0; j < shard->scan.len; ++j) {
            struct shard_scan *scan = shard->scan.table + j;
            save_write_value(save, (uint8_t) 0xFF);
            save_write_value(save, scan->src);
            save_write_value(save, scan->it);
        }
    }
    save_write_value(save, (uint8_t) 0x00);

    save_write_magic(save, save_magic_shards);
}

struct shards *shards_load(struct world *world, struct save *save)
{
    struct shards *shards = shards_alloc(world);
    if (!save_read_magic(save, save_magic_shards)) goto fail;

    uint8_t head = 0x00;

    while ((head = save_read_type(save, typeof(head)))) {
        assert(head == 0xFF);
        struct coord src = save_read_type(save, typeof(src));

        struct shard *shard = shards_get(shards, src);
        struct shard_probe *probe = shard_probe_append(shard);
        probe->src = src;
        save_read_into(save, &probe->dst);
        save_read_into(save, &probe->item);
    }

    while ((head = save_read_type(save, typeof(head)))) {
        assert(head == 0xFF);
        struct coord src = save_read_type(save, typeof(src));

        struct shard *shard = shards_get(shards, src);
        struct shard_scan *scan = shard_scan_append(shard);
        scan->src = src;
        save_read_into(save, &scan->it);
    }

    if (!save_read_magic(save, save_magic_shards)) goto fail;
    return shards;

  fail:
    shards_free(shards);
    return nullptr;
}
