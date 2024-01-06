/* world.c
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------

struct world_user
{
    bool active;
    user_id id;
    struct coord home;
    struct tech tech;
    struct log *log;
    struct user_io io;
};

struct world
{
    world_seed seed;
    world_ts time;

    struct mods *mods;
    struct atoms *atoms;

    struct htable sectors;
    struct htable chunks;
    struct lanes lanes;
    struct world_user users[user_max];

    struct shards *shards;
    struct metrics *metrics;
};


// -----------------------------------------------------------------------------
// user
// -----------------------------------------------------------------------------

static struct world_user *world_user(struct world *world, user_id id)
{
    assert(id < array_len(world->users));
    struct world_user *user = world->users + id;
    assert(user->active);
    return user;
}

static struct world_user *world_user_next(
        struct world *world, struct world_user *it)
{
    it = it ? it + 1 : world->users;
    for (; it < world->users + array_len(world->users); it++) {
        if (!it->active) continue;
        return it;
    }
    return NULL;
}


// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------

struct world *world_new(world_seed seed, struct metrics *metrics)
{
    struct world *world = calloc(1, sizeof(*world));

    world->seed = seed;
    world->atoms = atoms_new();
    world->mods = mods_new();
    lanes_init(&world->lanes, world);
    htable_reset(&world->sectors);
    world->metrics = metrics;
    world->shards = shards_alloc(world);

    return world;
}

void world_free(struct world *world)
{
    shards_free(world->shards);
    atoms_free(world->atoms);
    mods_free(world->mods);
    lanes_free(&world->lanes);

    for(const struct htable_bucket *it = htable_next(&world->chunks, NULL);
        it; it = htable_next(&world->chunks, it))
        chunk_free((void *) it->value);
    htable_reset(&world->chunks);

    for (const struct htable_bucket *it = htable_next(&world->sectors, NULL);
         it; it = htable_next(&world->sectors, it))
        sector_free((struct sector *) it->value);
    htable_reset(&world->sectors);

    for (struct world_user *it = world_user_next(world, NULL);
         it; it = world_user_next(world, it))
    {
        tech_free(&it->tech);
        log_free(it->log);
    }

    free(world);
}


// -----------------------------------------------------------------------------
// save/load
// -----------------------------------------------------------------------------

static void world_save_users(struct world *world, struct save *save)
{
    for (struct world_user *it = world_user_next(world, NULL);
         it; it = world_user_next(world, it))
    {
        save_write_value(save, it->id);
        save_write_value(save, coord_to_u64(it->home));
        tech_save(&it->tech, save);
        log_save(it->log, save);
    }

    save_write_value(save, (user_id) 0xFF);
}

static bool world_load_users(struct world *world, struct save *save)
{
    for (struct world_user *it = world_user_next(world, NULL);
         it; it = world_user_next(world, it))
    {
        log_free(it->log);
        tech_free(&it->tech);
    }
    memset(world->users, 0, sizeof(world->users));

    while (true) {
        user_id id = save_read_type(save, typeof(id));
        if (id == 0xFF) break;
        assert(id < array_len(world->users));

        struct world_user *user = world->users + id;
        user->active = true;
        user->id = id;
        user->home = coord_from_u64(save_read_type(save, uint64_t));

        if (!tech_load(&user->tech, save)) return false;
        if (!(user->log = log_load(save))) return false;
    }

    return true;
}


void world_save(struct world *world, struct save *save)
{
    save_write_magic(save, save_magic_world);

    save_write_value(save, world->seed);
    save_write_value(save, world->time);

    atoms_save(world->atoms, save);
    mods_save(world->mods, save);
    lanes_save(&world->lanes, save);
    world_save_users(world, save);
    shards_save(world->shards, save);

    save_write_value(save, (uint32_t) world->chunks.len);
    for (const struct htable_bucket *it = htable_next(&world->chunks, NULL);
         it; it = htable_next(&world->chunks, it))
    {
        struct chunk *chunk = (struct chunk *) it->value;
        save_write_value(save, chunk_star(chunk)->coord);
        chunk_save(chunk, save);
    }

    save_write_magic(save, save_magic_world);
}

struct world *world_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_world)) return NULL;

    struct metrics metrics = {0};
    struct world *world = world_new(0, &metrics);
    save_read_into(save, &world->seed);
    save_read_into(save, &world->time);

    if (world->atoms) atoms_free(world->atoms);
    if (!(world->atoms = atoms_load(save))) goto fail;

    if (world->mods) mods_free(world->mods);
    if (!(world->mods = mods_load(save))) goto fail;

    if (!lanes_load(&world->lanes, world, save)) goto fail;
    if (!world_load_users(world, save)) goto fail;

    if (world->shards) shards_free(world->shards);
    if (!(world->shards = shards_load(world, save))) goto fail;

    size_t chunks = save_read_type(save, uint32_t);
    htable_reserve(&world->chunks, chunks);
    for (size_t i = 0; i < chunks; ++i) {
        struct coord coord = save_read_type(save, typeof(coord));
        struct shard *shard = shards_get(world->shards, coord);

        struct chunk *chunk = chunk_load(save, shard);
        if (!chunk) goto fail;

        shards_register(world->shards, chunk);

        struct htable_ret ret = htable_put(
                &world->chunks, coord_to_u64(coord), (uintptr_t) chunk);
        assert(ret.ok);
    }

    if (!save_read_magic(save, save_magic_world)) goto fail;
    return world;

  fail:
    world_free(world);
    return NULL;
}


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

void world_step(struct world *world)
{
    world->time++;
    lanes_step(&world->lanes);
    shards_step(world->shards);
}

world_seed world_gen_seed(const struct world *world)
{
    return world->seed;
}

world_ts world_time(const struct world *world)
{
    return world->time;
}

struct coord world_home(struct world *world, user_id user)
{
    return world_user(world, user)->home;
}

struct tech *world_tech(struct world *world, user_id user)
{
    return &world_user(world, user)->tech;
}

struct atoms *world_atoms(struct world *world)
{
    return world->atoms;
}

struct mods *world_mods(struct world *world)
{
    return world->mods;
}

struct chunk *world_chunk_alloc(
        struct world *world, struct coord coord, user_id user)
{
    if (unlikely(coord_is_nil(coord))) return NULL;

    struct chunk *chunk = world_chunk(world, coord);
    if (chunk) return chunk;

    const struct sector *sector = world_sector(world, coord);
    assert(sector);

    const struct star *star = sector_star_at(sector, coord);
    assert(star);

    vm_word name = star_name(coord, world->seed, world->atoms);
    chunk = chunk_alloc(star, user, name);
    shards_register(world->shards, chunk);

    uint64_t key = coord_to_u64(coord);
    struct htable_ret ret = htable_put(&world->chunks, key, (uintptr_t) chunk);
    assert(ret.ok);

    return chunk;
}

struct chunk *world_chunk(struct world *world, struct coord coord)
{
    if (unlikely(coord_is_nil(coord))) return NULL;

    struct htable_ret ret = htable_get(&world->chunks, coord_to_u64(coord));
    return ret.ok ? (void *) ret.value : NULL;
}

const struct sector *world_sector(struct world *world, struct coord sector)
{
    if (unlikely(coord_is_nil(sector))) return NULL;

    struct coord coord = coord_sector(sector);
    uint64_t id = coord_to_u64(coord);

    struct htable_ret ret = htable_get(&world->sectors, id);
    if (ret.ok) return (struct sector *) ret.value;

    struct sector *value = sector_gen(coord, world->seed);
    ret = htable_put(&world->sectors, id, (uintptr_t) value);
    assert(ret.ok);

    return value;
}

vm_word world_star_name(struct world *world, struct coord coord)
{
    struct chunk *chunk = world_chunk(world, coord);
    if (chunk) return chunk_name(chunk);

    return star_name(coord, world->seed, world->atoms);
}

bool world_user_access(struct world *world, user_set access, struct coord coord)
{
    struct chunk *chunk = world_chunk(world, coord);
    return !chunk ? false : user_set_test(access, chunk_owner(chunk));
}


struct log *world_log(struct world *world, user_id id)
{
    return world_user(world, id)->log;
}

struct user_io *world_user_io(struct world *world, user_id user_id)
{
    return &world_user(world, user_id)->io;
}

void world_user_io_clear(struct world *world, user_id user_id)
{
    struct user_io *io = &world_user(world, user_id)->io;
    memset(io, 0, sizeof(*io));
}

struct metrics *world_metrics(struct world *world)
{
    return world->metrics;
}


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------
// This is user agnostic and should not be filtered.

ssize_t world_probe(struct world *world, struct coord coord, enum item item)
{
    struct chunk *chunk = world_chunk(world, coord);
    if (chunk) return chunk_count(chunk, item);

    const struct sector *sector = world_sector(world, coord);
    assert(sector);

    return sector_scan(sector, coord, item);
}

struct coord world_scan(struct world *world, struct scan_it it)
{
    const struct sector *sector = world_sector(world, it.coord);
    if (it.index >= sector->stars_len) return coord_nil();
    return sector->stars[it.index].coord;
}



// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

// \todo Only used in one test so...
struct vec64 *world_chunk_list(struct world *world)
{
    struct vec64 *vec = vec64_reserve(world->chunks.len);
    for (const struct htable_bucket *it = htable_next(&world->chunks, NULL);
         it; it = htable_next(&world->chunks, it))
    {
        vec = vec64_append(vec, it->key);
    }
    return vec;
}

struct world_chunk_it world_chunk_it(struct world *world, user_set filter)
{
    (void) world;
    return (struct world_chunk_it) { .filter = filter, .it = NULL };
}

struct chunk *world_chunk_next(struct world *world, struct world_chunk_it *it)
{
    while ((it->it = htable_next(&world->chunks, it->it))) {
        struct chunk *chunk = (void *) it->it->value;
        if (user_set_test(it->filter, chunk_owner(chunk))) return chunk;
    }
    return NULL;
}


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

struct lanes *world_lanes(struct world *world)
{
    return &world->lanes;
}

const struct hset *world_lanes_list(struct world *world, struct coord key)
{
    return lanes_list(&world->lanes, key);
}

void world_lanes_list_save(struct world *world, struct save *save, user_set filter)
{
    lanes_list_save(&world->lanes, save, world, filter);
}

void world_lanes_arrive(
        struct world *world,
        user_id owner, enum item type,
        struct coord src, struct coord dst,
        const vm_word *data, size_t len)
{
    struct chunk *chunk = world_chunk_alloc(world, dst, owner);
    assert(chunk);

    chunk_lanes_arrive(chunk, type, src, data, len);
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

static struct coord world_populate_home(struct sector *sector)
{
    for (size_t i = 0; i < sector->stars_len; ++i) {
        const struct star *star = &sector->stars[i];

        if (star_scan(star, item_energy) < 10000) continue;
        if (star_scan(star, item_elem_a) < 50000) continue;
        if (star_scan(star, item_elem_b) < 50000) continue;
        if (star_scan(star, item_elem_c) < 50000) continue;
        if (star_scan(star, item_elem_d) < 20000) continue;
        if (star_scan(star, item_elem_e) < 20000) continue;
        if (star_scan(star, item_elem_f) < 20000) continue;
        return star->coord;
    }
    return coord_nil();
}

void world_populate_user(struct world *world, user_id id)
{
    assert(id < array_len(world->users));

    struct world_user *user = world->users + id;
    if (user->active) return;

    user->active = true;
    user->id = id;
    user->log = log_new(world_log_cap);
    tech_populate(&user->tech);

    struct rng rng = rng_make(make_user_token());
    struct sector *sector = NULL;

    while (true) {
        if (sector) sector_free(sector);

        sector = sector_gen(coord_from_u64(rng_step(&rng)), world->seed);
        if (sector->stars_len < 100) continue;

        user->home = world_populate_home(sector);
        if (coord_is_nil(user->home)) continue;

        sector_free(sector);

        struct chunk *chunk = world_chunk_alloc(world, user->home, user->id);
        assert(chunk);

        for (const enum item *it = im_legion_cargo(item_legion); *it; it++) {
            bool ok = chunk_create(chunk, *it);
            assert(ok);
        }

        break;
    }
}


void world_populate(struct world *world)
{
    im_populate_atoms(world->atoms);
    io_populate_atoms(world->atoms);
    specs_populate_atoms(world->atoms);
    mods_populate(world->mods, world->atoms);
    world_populate_user(world, user_admin);
}
