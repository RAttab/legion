/* chunk_save.c
   Remi Attab (remi.attab@gmail.com), 04 Nov 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// save - all
// -----------------------------------------------------------------------------

static void chunk_save_provided(struct chunk *chunk, struct save *save)
{
    save_write_value(save, chunk->provided.len);
    for (const struct htable_bucket *it = htable_next(&chunk->provided, NULL);
         it; it = htable_next(&chunk->provided, it))
    {
        save_write_value(save, (enum item) it->key);
        ring16_save((struct ring16 *) it->value, save);
    }
}

static bool chunk_load_provided(struct chunk *chunk, struct save *save)
{
    size_t len = save_read_type(save, typeof(chunk->provided.len));
    htable_reserve(&chunk->provided, len);

    for (size_t i = 0; i < len; ++i) {
        enum item item = save_read_type(save, typeof(item));

        struct ring16 *ring = ring16_load(save);
        if (!ring) return false;

        struct htable_ret ret = htable_put(&chunk->provided, item, (uintptr_t) ring);
        assert(ret.ok);
    }

    return true;
}

void workers_save(const struct workers *workers, struct save *save, bool ops)
{
    save_write_magic(save, save_magic_workers);

    save_write_value(save, workers->count);
    save_write_value(save, workers->queue);
    save_write_value(save, workers->idle);
    save_write_value(save, workers->fail);
    save_write_value(save, workers->clean);
    if (ops) save_write_vec32(save, workers->ops);

    save_write_magic(save, save_magic_workers);
}

static void chunk_save_workers(struct chunk *chunk, struct save *save)
{
    workers_save(&chunk->workers, save, true);
}

bool workers_load(struct workers *workers, struct save *save, bool ops)
{
    if (!save_read_magic(save, save_magic_workers)) return false;

    save_read_into(save, &workers->count);
    save_read_into(save, &workers->queue);
    save_read_into(save, &workers->idle);
    save_read_into(save, &workers->fail);
    save_read_into(save, &workers->clean);
    if (ops && !save_read_vec32(save, &workers->ops)) return false;

    return save_read_magic(save, save_magic_workers);
}

static bool chunk_load_workers(struct chunk *chunk, struct save *save)
{
    return workers_load(&chunk->workers, save, true);
}

static void chunk_save_listen(struct chunk *chunk, struct save *save)
{
    save_write_value(save, chunk->listen.len);
    for (const struct htable_bucket *it = htable_next(&chunk->listen, NULL);
         it; it = htable_next(&chunk->listen, it))
    {
        save_write_value(save, it->key);
        save_write_value(save, it->value);
    }
}

static bool chunk_load_listen(struct chunk *chunk, struct save *save)
{
    size_t len = save_read_type(save, typeof(chunk->listen.len));
    htable_reserve(&chunk->listen, len);
    for (size_t i = 0; i < len; ++i) {
        uint64_t src = save_read_type(save, typeof(src));
        uint64_t chans = save_read_type(save, typeof(chans));

        struct htable_ret ret = htable_get(&chunk->listen, src);
        if (ret.ok)
            ret = htable_xchg(&chunk->listen, src, chans);
        else ret = htable_put(&chunk->listen, src, chans);
        assert(ret.ok);
    }

    return true;
}

static void chunk_save_active(struct chunk *chunk, struct save *save)
{
    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        active_save(it, save);
}

static bool chunk_load_active(struct chunk *chunk, struct save *save)
{
    for (size_t i = 0; i < array_len(chunk->active); ++i) {
        struct active *it = chunk->active + i;

        active_init(it, items_active_first + i);
        if (it->skip) continue;

        if (!active_load(it, save, chunk)) return false;
    }
    return true;
}

void chunk_save(struct chunk *chunk, struct save *save)
{
    save_write_magic(save, save_magic_chunk);

    save_write_value(save, chunk->name);
    save_write_value(save, chunk->owner);
    star_save(&chunk->star, save);

    chunk_save_provided(chunk, save);
    ring16_save(chunk->requested, save);
    ring16_save(chunk->storage, save);

    pills_save(&chunk->pills, save);
    energy_save(&chunk->energy, save);
    log_save(chunk->log, save);

    chunk_save_workers(chunk, save);
    chunk_save_listen(chunk, save);
    chunk_save_active(chunk, save);

    save_write_magic(save, save_magic_chunk);
}

struct chunk *chunk_load(struct save *save, struct shard *shard)
{
    if (!save_read_magic(save, save_magic_chunk)) return NULL;

    struct chunk *chunk = calloc(1, sizeof(*chunk));
    chunk->shard = shard;

    save_read_into(save, &chunk->name);
    save_read_into(save, &chunk->owner);
    star_load(&chunk->star, save);

    if (!chunk_load_provided(chunk, save)) goto fail;
    if (!(chunk->requested = ring16_load(save))) goto fail;
    if (!(chunk->storage = ring16_load(save))) goto fail;

    if (!(pills_load(&chunk->pills, save))) goto fail;
    if (!energy_load(&chunk->energy, save)) goto fail;

    if (!(chunk->log = log_load(save))) goto fail;
    if (!chunk_load_workers(chunk, save)) goto fail;
    if (!chunk_load_listen(chunk, save)) goto fail;
    if (!chunk_load_active(chunk, save)) goto fail;

    if (!save_read_magic(save, save_magic_chunk)) goto fail;
    return chunk;

  fail:
    chunk_free(chunk);
    return NULL;
}


// -----------------------------------------------------------------------------
// save - delta
// -----------------------------------------------------------------------------

static void chunk_save_delta_provided(
        struct chunk *chunk, struct save *save, const struct chunk_ack *ack)
{
    assert(chunk->provided.len < items_max);
    for (const struct htable_bucket *it = htable_next(&chunk->provided, NULL);
         it; it = htable_next(&chunk->provided, it))
    {
        struct htable_ret ret = htable_get(&ack->provided, it->key);
        struct ring_ack rack = ring_ack_from_u64(ret.value);

        // Optimization to avoid sending full deltas for acknowledged empty rings.
        struct ring16 *ring = (void *) it->value;
        if (rack.head == rack.tail && ring->head == ring->tail) continue;

        save_write_value(save, (enum item) it->key);
        ring16_save_delta(ring, save, &rack);
    }

    save_write_value(save, (enum item) 0);
}

static bool chunk_load_delta_provided(
        struct chunk *chunk, struct save *save, struct chunk_ack *ack)
{
    enum item item = 0;
    while ((item = save_read_type(save, typeof(item)))) {

        struct htable_ret ret_ack = htable_get(&ack->provided, item);
        struct ring_ack rack = ring_ack_from_u64(ret_ack.value);

        struct htable_ret ret = htable_get(&chunk->provided, item);
        struct ring16 *ring = (void *) ret.value;

        if (!ring16_load_delta(&ring, save, &rack)) {
            if (!ret.ok) ring16_free(ring);
            return false;
        }

        if (ret.ok)
            ret = htable_xchg(&chunk->provided, item, (uintptr_t) ring);
        else ret = htable_put(&chunk->provided, item, (uintptr_t) ring);
        assert(ret.ok);

        if (ret_ack.ok)
            ret_ack = htable_xchg(&ack->provided, item, ring_ack_to_u64(rack));
        else ret_ack = htable_put(&ack->provided, item, ring_ack_to_u64(rack));
        assert(ret_ack.ok);
    }

    return true;
}

static void chunk_save_delta_active(
        struct chunk *chunk, struct save *save, const struct chunk_ack *ack)
{
    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it)) {
        hash_val hash = active_hash(it, hash_init());
        if (ack->active[it->type - items_active_first] == hash) continue;

        save_write_value(save, it->type);
        save_write_value(save, hash);
        active_save(it, save);
    }
    save_write_value(save, (enum item) 0);
}

static bool chunk_load_delta_active(
        struct chunk *chunk, struct save *save, struct chunk_ack *ack)
{
    enum item type = 0;
    while ((type = save_read_type(save, typeof(type)))) {
        hash_val hash = save_read_type(save, typeof(hash));

        struct active *active = active_index(chunk, type);
        assert(active && !active->skip);

        if (!active_load(active, save, NULL)) return false;
        ack->active[type - items_active_first] = hash;
    }

    return true;
}

static void chunk_save_delta_pills(
        struct chunk *chunk, struct save *save, const struct chunk_ack *ack)
{
    hash_val hash = pills_hash(&chunk->pills, hash_init());
    if (hash == ack->pills) {
        save_write_value(save, ((typeof(hash)) 0));
        return;
    }

    save_write_value(save, hash);
    pills_save(&chunk->pills, save);
}

static bool chunk_load_delta_pills(
        struct chunk *chunk, struct save *save, struct chunk_ack *ack)
{
    hash_val hash = save_read_type(save, typeof(hash));
    if (!hash) return true;

    if (!pills_load(&chunk->pills, save)) return false;
    ack->pills = hash;
    return true;
}

void chunk_save_delta(
        struct chunk *chunk, struct save *save, const struct ack *ack)
{
    save_write_magic(save, save_magic_chunk);

    save_write_value(save, chunk->name);
    save_write_value(save, chunk->owner);
    star_save(&chunk->star, save);

    const struct chunk_ack *cack = &ack->chunk;
    static const struct chunk_ack cack_nil = {0};
    if (!coord_eq(ack->chunk.coord, chunk->star.coord)) cack = &cack_nil;

    chunk_save_delta_provided(chunk, save, cack);
    ring16_save_delta(chunk->requested, save, &cack->requested);
    ring16_save_delta(chunk->storage, save, &cack->storage);

    chunk_save_delta_pills(chunk, save, cack);
    energy_save(&chunk->energy, save);
    log_save_delta(chunk->log, save, cack->time);

    chunk_save_workers(chunk, save);
    chunk_save_listen(chunk, save);
    chunk_save_delta_active(chunk, save, cack);

    save_write_magic(save, save_magic_chunk);
}

bool chunk_load_delta(struct chunk *chunk, struct save *save, struct ack *ack)
{
    if (!save_read_magic(save, save_magic_chunk)) return false;

    save_read_into(save, &chunk->name);
    save_read_into(save, &chunk->owner);
    if (!star_load(&chunk->star, save)) return false;

    if (!coord_eq(ack->chunk.coord, chunk->star.coord)) ack_reset_chunk(ack);

    if (!chunk_load_delta_provided(chunk, save, &ack->chunk)) return false;
    if (!ring16_load_delta(&chunk->requested, save, &ack->chunk.requested)) return false;
    if (!ring16_load_delta(&chunk->storage, save, &ack->chunk.storage)) return false;

    if (!chunk_load_delta_pills(chunk, save, &ack->chunk)) return false;
    if (!energy_load(&chunk->energy, save)) return false;

    if (!log_load_delta(chunk->log, save, ack->chunk.time)) return false;
    if (!chunk_load_workers(chunk, save)) return false;
    if (!chunk_load_listen(chunk, save)) return false;
    if (!chunk_load_delta_active(chunk, save, &ack->chunk)) return false;

    if (!save_read_magic(save, save_magic_chunk)) return false;

    ack->chunk.coord = chunk->star.coord;
    return true;
}
