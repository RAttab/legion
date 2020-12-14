/* hunk.c
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "hunk.h"
#include "game/galaxy.h"
#include "game/obj.h"
#include "utils/vec.h"
#include "utils/htable.h"

// -----------------------------------------------------------------------------
// arena
// -----------------------------------------------------------------------------

enum { hunk_arenas = 8 };

struct hunk_arena
{
    size_t len;
    size_t curr, cap;
    legion_pad(8 * (8 - 3));

    uint8_t data[];
};

static_assert(sizeof(struct hunk_arena) == s_cache_line);

static size_t hunk_arena_append(struct hunk_arena **arena, size_t index)
{
    size_t len = cache_line * index;

    if (arena == NULL) {
        const size_t cap = 8;
        *arena = alloc_cache(sizeof(**arena) + len * cap);
        (*arena)->len = len;
        (*arena)->curr = 0;
        (*arena)->cap = cap;
    }

    if ((*arena)->curr == (*arena)->cap) {
        const size_t cap = (*arena)->cap * 2;
        (*arena) = realloc(*arena, sizeof(**arena) + (*arena)->len * cap);
        (*arena)->cap = cap;

        // Somewhat likely to trigger so might have to switch to mmap
        assert(((uintptr_t) *arena) % cache_line == 0);
    }

    assert((*arena)->curr + (*arena)->len <= (*arena)->cap);

    size_t off = (*arena)->curr;
    memset((*arena)->data + off, 0, len);
    (*arena)->curr += len;
    return off;
}

static struct obj *hunk_arena_get(struct hunk_arena *arena, size_t off)
{
    assert(arena);
    assert(off < arena->curr);
    return (struct obj *) (arena->data + off);
}

static void hunk_arena_step(struct hunk_arena *arena, struct hunk *hunk)
{
    if (!arena) return;

    // \todo prefetch the next object.
    for (size_t off = 0; off < arena->curr; off += arena->len) {
        struct obj *obj = hunk_arena_get(arena, off);
        obj_step(obj, hunk);
    }
}

static struct vec64 *hunk_arena_list(struct hunk_arena *arena, struct vec64 *list)
{
    if (!arena) return list;

    // \todo prefetch the next object.
    for (size_t off = 0; off < arena->curr; off += arena->len) {
        struct obj *obj = hunk_arena_get(arena, off);
        list = vec64_append(list, obj->id);
    }

    return list;
}


// -----------------------------------------------------------------------------
// hunk
// -----------------------------------------------------------------------------

struct hunk
{
    struct star star;

    id_t ids;
    struct htable index;
    struct hunk_arena *arenas[hunk_arenas];
};

struct hunk *hunk_alloc(const struct star *star)
{
    struct hunk *hunk = calloc(1, sizeof(*hunk));
    hunk->ids = 1;
    hunk->star = *star;
    return hunk;
}

void hunk_free(struct hunk *hunk)
{
    for (size_t arena = 0; arena < hunk_arenas; ++arena)
        free(hunk->arenas[arena]);

    htable_reset(&hunk->index);
    free(hunk);
}

struct star *hunk_star(struct hunk *hunk)
{
    return &hunk->star;
}

struct obj *hunk_obj(struct hunk *hunk, id_t id)
{
    struct htable_ret ret = htable_get(&hunk->index, id);
    if (!ret.ok) return NULL;

    size_t arena = ret.value >> 32;
    size_t off = ret.value & ((1UL << 32) - 1);

    return hunk_arena_get(hunk->arenas[arena], off);
}

struct obj *hunk_obj_alloc(struct hunk *hunk, item_t type, size_t len)
{
    size_t arena = len / cache_line;

    assert(len % cache_line == 0);
    assert(arena < hunk_arenas);

    size_t off = hunk_arena_append(&hunk->arenas[arena], arena);
    struct obj *obj = hunk_arena_get(hunk->arenas[arena], off);
    obj->id = make_id(type, hunk->ids++);

    struct htable_ret ret = htable_put(&hunk->index, obj->id, arena << 32 | off);
    assert(ret.ok);

    return obj;
}

void hunk_step(struct hunk *hunk)
{
    for (size_t arena = 0; arena < hunk_arenas; ++arena)
        hunk_arena_step(hunk->arenas[arena], hunk);
}

size_t hunk_harvest(struct hunk *hunk, item_t type, size_t count)
{
    if (type < elem_natural_first || type > elem_natural_last) return 0;

    size_t index = type - elem_natural_first;
    count = i64_min(hunk->star.elements[index], count);
    hunk->star.elements[index] -= count;
    return count;
}

struct vec64 *hunk_list(struct hunk *hunk)
{
    size_t len = 0;
    for (size_t i = 0; i < hunk_arenas; ++i) {
        struct hunk_arena *arena = hunk->arenas[i];
        if (arena) len += arena->len;
    }

    struct vec64 *list = vec64_reserve(len);
    for (size_t i = 0; i < hunk_arenas; ++i) {
        struct hunk_arena *arena = hunk->arenas[i];
        list = hunk_arena_list(arena, list);
    }

    return list;
}
