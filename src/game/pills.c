/* pills.c
   Rémi Attab (remi.attab@gmail.com), 14 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// pills
// -----------------------------------------------------------------------------

void pills_init(struct pills *pills)
{
    memset(pills, 0, sizeof(*pills));
    bits_init(&pills->free);
    bits_flip(&pills->free);
}

void pills_free(struct pills *pills)
{
    free(pills->coord);
    free(pills->cargo);
}

static void pills_grow(struct pills *pills, size_t len)
{
    if (likely(pills->cap >= len)) return;
    assert(len <= pills_cap);

    size_t old = pills->cap;
    if (!pills->cap) pills->cap = 4;
    if (2U * len > pills_cap) pills->cap = pills_cap;
    else while (pills->cap < len) pills->cap *= 2;

    pills->coord = realloc_zero(pills->coord, old, pills->cap, sizeof(*pills->coord));
    pills->cargo = realloc_zero(pills->cargo, old, pills->cap, sizeof(*pills->cargo));

    bits_grow(&pills->free, pills->cap);
    for (size_t i = old; i < pills->cap; ++i)
        bits_set(&pills->free, i);
}


hash_val pills_hash(struct pills *pills, hash_val hash)
{
    hash = hash_value(hash, pills->count);
    hash = bits_hash(&pills->free, hash);

    for (size_t i = 0; i < pills->cap; ++i) {
        if (!bits_test(&pills->free, i)) continue;
        hash = hash_value(hash, pills->coord[i].x);
        hash = hash_value(hash, pills->coord[i].y);
    }

    for (size_t i = 0; i < pills->cap; ++i) {
        if (!bits_test(&pills->free, i)) continue;
        hash = hash_value(hash, pills->cargo[i].item);
        hash = hash_value(hash, pills->cargo[i].count);
    }

    return hash;
}

bool pills_load(struct pills *pills, struct save *save)
{
    if (!save_read_magic(save, save_magic_pills)) return false;

    save_read_into(save, &pills->count);
    pills_grow(pills, pills->count);

    memset(pills->coord, 0, pills->cap * sizeof(*pills->coord));
    memset(pills->cargo, 0, pills->cap * sizeof(*pills->cargo));
    bits_clear(&pills->free);
    bits_flip(&pills->free);

    for (size_t i = 0; i < pills->count; ++i) {
        save_read_into(save, pills->coord + i);
        save_read_into(save, pills->cargo + i);
        bits_unset(&pills->free, i);
    }

    return save_read_magic(save, save_magic_pills);
}

void pills_save(struct pills *pills, struct save *save)
{
    save_write_magic(save, save_magic_pills);

    save_write_value(save, pills->count);

    bits_copy(&pills->match, &pills->free);
    bits_flip(&pills->match);

    for (size_t i = bits_next(&pills->match, 0);
         i < pills->cap; i = bits_next(&pills->match, i + 1))
    {
        save_write_value(save, pills->coord[i]);
        save_write_value(save, pills->cargo[i]);
    }

    save_write_magic(save, save_magic_pills);
}


size_t pills_count(struct pills *pills)
{
    return pills->count;
}

struct pills_ret pills_next(struct pills *pills, size_t *index)
{
    bits_copy(&pills->match, &pills->free);
    bits_flip(&pills->match);

    *index = bits_next(&pills->match, *index);
    if (*index == pills->match.len)
        return (struct pills_ret) { .ok = false };

    return (struct pills_ret) {
        .ok = true,
        .coord = pills->coord[*index],
        .cargo = pills->cargo[*index],
    };
}

struct pills_ret pills_dock(struct pills *pills, struct coord coord, enum item item)
{
    bits_copy(&pills->match, &pills->free);
    bits_flip(&pills->match);

    if (item) {
        for (size_t i = 0; i < pills->cap; ++i) {
            if (pills->cargo[i].item != item)
                bits_unset(&pills->match, i);
        }
    }

    if (!coord_is_nil(coord)) {
        for (size_t i = 0; i < pills->cap; ++i) {
            if (!coord_eq(pills->coord[i], coord))
                bits_unset(&pills->match, i);
        }
    }

    size_t index = bits_next(&pills->match, 0);
    if (index == pills->match.len)
        return (struct pills_ret) { .ok = false };

    bits_set(&pills->free, index);
    pills->count--;

    return (struct pills_ret) {
        .ok = true,
        .coord = pills->coord[index],
        .cargo = pills->cargo[index],
    };
}

bool pills_arrive(struct pills *pills, struct coord origin, struct cargo cargo)
{
    if (pills->count == pills_cap) return false;
    pills_grow(pills, pills->count + 1);

    size_t index = bits_next(&pills->free, 0);
    assert(index < pills->cap);
    pills->coord[index] = origin;
    pills->cargo[index] = cargo;

    bits_unset(&pills->free, index);
    pills->count++;

    return true;
}
