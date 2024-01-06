/* lanes.c
   Rémi Attab (remi.attab@gmail.com), 03 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// data
// -----------------------------------------------------------------------------

legion_packed struct lane_data
{
    user_id owner;
    enum item item;
    bool forward;
    uint8_t len;
    legion_pad(4);
    vm_word data[];
};

static_assert(sizeof(struct lane_data) == 8);

static size_t lane_data_len(size_t len)
{
    return sizeof(struct lane_data) + len * sizeof(vm_word);
}


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

legion_packed struct lane_queue
{
    world_ts ts;
    heap_ix data;
};

static_assert(sizeof(struct lane_queue) == 8);


struct lane
{
    struct coord src, dst;

    uint16_t len, cap;
    struct lane_queue *queue;
};


static struct lane *lane_alloc(struct coord src, struct coord dst)
{
    constexpr size_t cap_default = 1;

    struct lane *lane = calloc(1, sizeof(*lane));
    *lane = (struct lane) {
        .src = src,
        .dst = dst,
        .cap = cap_default,
        .queue = calloc(cap_default, sizeof(struct lane_queue)),
    };
    return lane;
}

static void lane_free(struct lane *lane)
{
    free(lane->queue);
    free(lane);
}

static struct lane *lane_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_lane)) return NULL;

    struct lane *lane = calloc(1, sizeof(*lane));
    save_read_into(save, &lane->src);
    save_read_into(save, &lane->dst);
    save_read_into(save, &lane->len);
    save_read_into(save, &lane->cap);

    lane->queue = calloc(lane->cap, sizeof(*lane->queue));
    save_read(save, lane->queue, lane->cap * sizeof(*lane->queue));

    if (!save_read_magic(save, save_magic_lane)) goto fail;
    return lane;

  fail:
    free(lane->queue);
    free(lane);
    return NULL;
}

static void lane_save(struct lane *lane, struct save *save)
{
    save_write_magic(save, save_magic_lane);
    save_write_value(save, lane->src);
    save_write_value(save, lane->dst);
    save_write_value(save, lane->len);
    save_write_value(save, lane->cap);
    save_write(save, lane->queue, lane->cap * sizeof(*lane->queue));
    save_write_magic(save, save_magic_lane);
}

static void lane_grow(struct lane *lane)
{
    if (likely(lane->len != lane->cap)) return;

    lane->cap *= 2;
    lane->queue = reallocarray(lane->queue, lane->cap, sizeof(*lane->queue));
}

static void lane_push(struct lane *lane, world_ts ts, heap_ix data)
{
    lane_grow(lane);

    size_t index = lane->len++;
    struct lane_queue *it = &lane->queue[index];
    *it = (struct lane_queue) { .ts = ts, .data = data };

    while (index) {
        struct lane_queue *parent = lane->queue + (index / 2);
        if (parent->ts < it->ts) break;

        legion_swap(it, parent);
        it = parent;
        index /= 2;
    }
}

static world_ts lane_peek(struct lane *lane)
{
    return lane->len ? lane->queue[0].ts : (world_ts) -1;
}

static heap_ix lane_pop(struct lane *lane)
{
    assert(lane->len);

    heap_ix ret = lane->queue[0].data;
    lane->queue[0] = lane->queue[--lane->len];

    size_t index = 0;
    while (index < lane->len) {
        struct lane_queue *it = lane->queue + index;

        size_t li = index * 2;
        size_t ri = index * 2 + 1;

        struct lane_queue *lp = li < lane->len ? lane->queue + li : NULL;
        struct lane_queue *rp = ri < lane->len ? lane->queue + ri : NULL;

        size_t child = 0;
        struct lane_queue *ptr = NULL;

        if ((lp && lp->ts < it->ts) && (!rp || lp->ts < rp->ts)) {
            child = li;
            ptr = lp;
        }
        else if (rp && rp->ts < it->ts) {
            child = ri;
            ptr = rp;
        }
        else break;

        legion_swap(it, ptr);
        index = child;
        it = ptr;
    }

    return ret;
}


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

void lanes_init(struct lanes *lanes, struct world *world)
{
    lanes->world = world;
    heap_init(&lanes->data);
}

void lanes_free(struct lanes *lanes)
{
    const struct htable_bucket *it = NULL;

    it = htable_next(&lanes->lanes, NULL);
    for (; it; it = htable_next(&lanes->lanes, it))
        lane_free((void *) it->value);
    htable_reset(&lanes->lanes);

    it = htable_next(&lanes->index, NULL);
    for (; it; it = htable_next(&lanes->index, it))
        hset_free((void *) it->value);
    htable_reset(&lanes->index);

    heap_free(&lanes->data);
}

static uint64_t lanes_key(struct coord src, struct coord dst)
{
    return hash_u64(coord_to_u64(src)) ^ hash_u64(coord_to_u64(dst));
}

static void lanes_index_put(struct lanes *lanes, struct coord key, struct coord val)
{
    const uint64_t key64 = coord_to_u64(key);
    struct htable_ret ret = htable_get(&lanes->index, key64);

    struct hset *old = (void *) ret.value;
    struct hset *new = hset_put(old, coord_to_u64(val));
    if (old == new) return;

    if (old) ret = htable_xchg(&lanes->index, key64, (uintptr_t) new);
    else ret = htable_put(&lanes->index, key64, (uintptr_t) new);
    assert(ret.ok);
}

static void lanes_index_del(struct lanes *lanes, struct coord key, struct coord val)
{
    uint64_t key64 = coord_to_u64(key);
    struct htable_ret ret = htable_get(&lanes->index, key64);
    assert(ret.ok);

    struct hset *set = (void *) ret.value;
    bool ok = hset_del(set, coord_to_u64(val));
    assert(ok);

    if (!set->len) {
        hset_free(set);
        ret = htable_del(&lanes->index, key64);
        assert(ret.ok);
    }
}

void lanes_save(struct lanes *lanes, struct save *save)
{
    save_write_magic(save, save_magic_lanes);
    save_write_value(save, (uint32_t) lanes->lanes.len);

    for (const struct htable_bucket *it = htable_next(&lanes->lanes, NULL);
         it; it = htable_next(&lanes->lanes, it))
    {
        lane_save((void *) it->value, save);
    }

    heap_save(&lanes->data, save);
    save_write_magic(save, save_magic_lanes);
}

bool lanes_load(struct lanes *lanes, struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_lanes)) return false;
    lanes->world = world;

    uint32_t len = save_read_type(save, typeof(len));
    for (size_t i = 0; i < len; ++i) {
        struct lane *lane = lane_load(save);
        if (!lane) goto fail;

        uint64_t key = lanes_key(lane->src, lane->dst);
        struct htable_ret ret = htable_put(&lanes->lanes, key, (uintptr_t) lane);
        assert(ret.ok);

        lanes_index_put(lanes, lane->src, lane->dst);
        lanes_index_put(lanes, lane->dst, lane->src);
    }

    if (!heap_load(&lanes->data, save)) goto fail;
    if (!save_read_magic(save, save_magic_lanes)) goto fail;
    return true;

  fail:
    lanes_free(lanes);
    return false;
}

world_ts_delta lanes_travel(size_t speed, struct coord src, struct coord dst)
{
    return coord_dist(src, dst) / speed;
}

const struct hset *lanes_list(struct lanes *lanes, struct coord key)
{
    struct htable_ret ret = htable_get(&lanes->index, coord_to_u64(key));
    return (void *) ret.value;
}

void lanes_list_save(
        struct lanes *lanes,
        struct save *save,
        struct world *world,
        user_set filter)
{
    save_write_magic(save, save_magic_lanes);

    for (const struct htable_bucket *it = htable_next(&lanes->lanes, NULL);
         it; it = htable_next(&lanes->lanes, it))
    {
        const struct lane *lane = (void *) it->value;

        if (!world_user_access(world, filter, lane->src) &&
                !world_user_access(world, filter, lane->dst))
            continue;

        save_write_value(save, coord_to_u64(lane->src));
        save_write_value(save, coord_to_u64(lane->dst));
    }

    save_write_value(save, (uint64_t) 0);
    save_write_magic(save, save_magic_lanes);
}

bool lanes_list_load_into(struct htable *lanes, struct save *save)
{
    // Clear the htable and the hset while avoiding unnecessary allocations.
    for (const struct htable_bucket *it = htable_next(lanes, NULL);
         it; it = htable_next(lanes, it))
    {
        struct hset *set = (void *) it->value;

        if (set->len) hset_clear(set);
        else {
            struct htable_ret ret = htable_del(lanes, it->key);
            assert(ret.ok);
            hset_free(set);
        }
    }

    if (!save_read_magic(save, save_magic_lanes)) return false;

    uint64_t key = 0;
    while ((key = save_read_type(save, typeof(key)))) {
        uint64_t val = save_read_type(save, typeof(val));

        void index(struct htable *lanes, uint64_t key, uint64_t val)
        {
            struct htable_ret ret = htable_get(lanes, key);

            struct hset *set = NULL;
            if (ret.ok) set = (void *) ret.value;
            set = hset_put(set, val);

            if (ret.ok) ret = htable_xchg(lanes, key, (uintptr_t) set);
            else ret = htable_put(lanes, key, (uintptr_t) set);
            assert(ret.ok);
        }
        index(lanes, key, val);
        index(lanes, val, key);
    }

    if (!save_read_magic(save, save_magic_lanes)) assert(false);
    return true;
}

void lanes_launch(struct lanes *lanes, struct lanes_packet packet)
{
    uint64_t key = lanes_key(packet.src, packet.dst);
    struct htable_ret ret = htable_get(&lanes->lanes, key);

    struct lane *lane = NULL;
    if (ret.ok) lane = (void *) ret.value;
    else {
        lane = lane_alloc(packet.src, packet.dst);
        ret = htable_put(&lanes->lanes, key, (uintptr_t) lane);
        assert(ret.ok);

        lanes_index_put(lanes, packet.src, packet.dst);
        lanes_index_put(lanes, packet.dst, packet.src);
    }

    heap_ix data_index = heap_new(&lanes->data, lane_data_len(packet.len));
    {
        struct lane_data *data_ptr = heap_ptr(&lanes->data, data_index);
        *data_ptr = (struct lane_data) {
            .owner = packet.owner,
            .item = packet.item,
            .forward = coord_eq(packet.src, lane->src),
            .len = packet.len,
        };
        memcpy(data_ptr->data, packet.data, packet.len * sizeof(*packet.data));
    }

    world_ts_delta travel = lanes_travel(packet.speed, packet.src, packet.dst);
    assert(travel > 0);

    lane_push(lane, world_time(lanes->world) + travel, data_index);
}

void lanes_step(struct lanes *lanes)
{
    sys_ts mt = metric_now();
    size_t mn = 0;

    world_ts now = world_time(lanes->world);

    const struct htable_bucket *it = htable_next(&lanes->lanes, NULL);
    for (; it; it = htable_next(&lanes->lanes, it)) {
        struct lane *lane = (void *) it->value;

        for (; lane_peek(lane) <= now; ++mn) {
            heap_ix data_index = lane_pop(lane);
            struct lane_data *data = heap_ptr(&lanes->data, data_index);

            struct coord src = data->forward ? lane->src : lane->dst;
            struct coord dst = data->forward ? lane->dst : lane->src;
            world_lanes_arrive(
                    lanes->world,
                    data->owner, data->item,
                    src, dst,
                    data->data, data->len);

            heap_del(&lanes->data, data_index, lane_data_len(data->len));
        }

        if (!lane->len) {
            lanes_index_del(lanes, lane->src, lane->dst);
            lanes_index_del(lanes, lane->dst, lane->src);
            lane_free(lane);

            struct htable_ret ret = htable_del(&lanes->lanes, it->key);
            assert(ret.ok);
        }
    }

    metric_inc(world_metrics(lanes->world), world.lanes, mn, mt);
}
