/* galaxy.c
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/sector.h"
#include "game/chunk.h"
#include "game/gen.h"
#include "utils/rng.h"

// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

bool star_load(struct star *star, struct save *save)
{
    if (!save_read_magic(save, save_magic_star)) return false;
    save_read_into(save, &star->coord);
    save_read_into(save, &star->state);
    save_read_into(save, &star->energy);
    save_read_into(save, &star->elems);
    return save_read_magic(save, save_magic_star);
}

void star_save(struct star *star, struct save *save)
{
    save_write_magic(save, save_magic_star);
    save_write_value(save, star->coord);
    save_write_value(save, star->state);
    save_write_value(save, star->energy);
    save_write_from(save, &star->elems);
    save_write_magic(save, save_magic_star);
}


// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

struct sector *sector_new(struct world *world, size_t stars)
{
    struct sector *sector =
        calloc(1, sizeof(*sector) + (stars * sizeof(sector->stars[0])));

    sector->world = world;
    sector->stars_len = stars;
    htable_reserve(&sector->index, stars);

    return sector;
}

void sector_free(struct sector *sector)
{
    for(struct htable_bucket *it = htable_next(&sector->chunks, NULL);
        it; it = htable_next(&sector->chunks, it))
        chunk_free((void *) it->value);
    htable_reset(&sector->chunks);
    htable_reset(&sector->index);
    free(sector);
}


struct sector *sector_load(struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_sector)) return NULL;

    struct coord coord = save_read_type(save, typeof(coord));
    struct sector *sector = gen_sector(world, coord, world_seed(world));

    size_t chunks = save_read_type(save, uint32_t);
    htable_reserve(&sector->chunks, chunks);
    for (size_t i = 0; i < chunks; ++i) {
        struct chunk *chunk = chunk_load(world, save);
        if (!chunk) goto fail;

        uint64_t id = coord_to_u64(chunk_star(chunk)->coord);

        struct htable_ret ret = htable_get(&sector->index, id);
        if (!ret.ok) goto fail;
        ((struct star *) ret.value)->state = star_active;

        ret = htable_put(&sector->chunks, id, (uintptr_t) chunk);
        assert(ret.ok);
    }

    if (!save_read_magic(save, save_magic_sector)) goto fail;
    return sector;

  fail:
    sector_free(sector);
    return NULL;
}

void sector_save(struct sector *sector, struct save *save)
{
    save_write_magic(save, save_magic_sector);
    save_write_value(save, sector->coord);
    save_write_value(save, (uint32_t) sector->chunks.len);
    struct htable_bucket *it = htable_next(&sector->chunks, NULL);
    for (; it; it = htable_next(&sector->chunks, it)) {
        chunk_save((struct chunk *) it->value, save);
    }

    save_write_magic(save, save_magic_sector);
}

struct chunk *sector_chunk_alloc(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_u64(coord);

    struct chunk *chunk = sector_chunk(sector, coord);
    if (likely(chunk != NULL)) return chunk;

    struct htable_ret ret = htable_get(&sector->index, id);
    if (!ret.ok) return NULL;
    struct star *star = (void *) ret.value;

    chunk = chunk_alloc(sector->world, star);
    ret = htable_put(&sector->chunks, id, (uint64_t) chunk);
    assert(ret.ok);

    star->state = star_active;
    return chunk;
}

struct chunk *sector_chunk(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_u64(coord);
    struct htable_ret ret = htable_get(&sector->chunks, id);
    return ret.ok ? (void *) ret.value : NULL;
}

const struct star *sector_star_in(struct sector *sector, struct rect rect)
{
    for (size_t i = 0; i < sector->stars_len; ++i) {
        struct star *star = &sector->stars[i];
        if (rect_contains(&rect, star->coord)) return star;
    }
    return NULL;
}

const struct star *sector_star_at(struct sector *sector, struct coord coord)
{
    struct htable_ret ret = htable_get(&sector->index, coord_to_u64(coord));
    return ret.ok ? (struct star *) ret.value : NULL;
}

void sector_step(struct sector *sector)
{
    struct htable_bucket *bucket = htable_next(&sector->chunks, NULL);
    for (; bucket; bucket = htable_next(&sector->chunks, bucket))
        chunk_step((void *) bucket->value);
}

ssize_t sector_scan(struct sector *sector, struct coord coord, enum item item)
{
    uint64_t id = coord_to_u64(coord);

    struct htable_ret ret = htable_get(&sector->index, id);
    if (!ret.ok) return -1;

    struct star *star = (void *) ret.value;
    if (star->state != star_active) return star_scan(star, item);

    ret = htable_get(&sector->chunks, id);
    assert(ret.ok);
    return chunk_scan((struct chunk *) ret.value, item);
}



void sector_lanes_arrive(
        struct sector *sector,
        enum item type,
        struct coord src, struct coord dst,
        const word_t *data, size_t len)
{
    struct chunk *chunk = sector_chunk_alloc(sector, dst);
    chunk_lanes_arrive(chunk, type, src, data, len);
}
