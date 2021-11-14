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
    save_read_into(save, &star->hue);
    save_read_into(save, &star->energy);
    save_read_into(save, &star->elems);
    return save_read_magic(save, save_magic_star);
}

void star_save(const struct star *star, struct save *save)
{
    save_write_magic(save, save_magic_star);
    save_write_value(save, star->coord);
    save_write_value(save, star->hue);
    save_write_value(save, star->energy);
    save_write_from(save, &star->elems);
    save_write_magic(save, save_magic_star);
}


// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

struct sector *sector_new(size_t stars)
{
    struct sector *sector =
        calloc(1, sizeof(*sector) + (stars * sizeof(sector->stars[0])));

    sector->stars_len = stars;
    htable_reserve(&sector->index, stars);

    return sector;
}

void sector_free(struct sector *sector)
{
    htable_reset(&sector->index);
    free(sector);
}

const struct star *sector_star_in(const struct sector *sector, struct rect rect)
{
    for (size_t i = 0; i < sector->stars_len; ++i) {
        const struct star *star = &sector->stars[i];
        if (rect_contains(&rect, star->coord)) return star;
    }
    return NULL;
}

const struct star *sector_star_at(
        const struct sector *sector, struct coord coord)
{
    struct htable_ret ret = htable_get(&sector->index, coord_to_u64(coord));
    return ret.ok ? (struct star *) ret.value : NULL;
}

ssize_t sector_scan(
        const struct sector *sector, struct coord coord, enum item item)
{
    uint64_t id = coord_to_u64(coord);

    struct htable_ret ret = htable_get(&sector->index, id);
    if (!ret.ok) return -1;

    struct star *star = (void *) ret.value;
    return star_scan(star, item);
}
