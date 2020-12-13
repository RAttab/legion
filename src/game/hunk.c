/* hunk.c
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "hunk.h"
#include "game/galaxy.h"
#include "game/obj.h"
#include "utils/htable.h"

// -----------------------------------------------------------------------------
// area
// -----------------------------------------------------------------------------

enum { hunk_classes = 8 };

struct hunk_area
{
    size_t len;
    size_t curr, cap;
    legion_pad(8 * (8 - 3));

    uint8_t data[];
};

static_assert(sizeof(struct hunk_area) == s_cache_line);

static size_t area_append(struct hunk_area **area, size_t class)
{
    size_t len = cache_line * class;

    if (area == NULL) {
        const size_t cap = 8;
        *area = alloc_cache(sizeof(**area) + len * cap);
        (*area)->len = len;
        (*area)->curr = 0;
        (*area)->cap = cap;
    }

    if ((*area)->curr == (*area)->cap) {
        const size_t cap = (*area)->cap * 2;
        (*area) = realloc(*area, sizeof(**area) + (*area)->len * cap);
        (*area)->cap = cap;

        // Somewhat likely to trigger so might have to switch to mmap
        assert(((uintptr_t) *area) % cache_line == 0); 
    }

    assert((*area)->curr + (*area)->len <= (*area)->cap);

    size_t off = (*area)->curr;
    memset((*area)->data + off, 0, len);
    (*area)->curr += len;
    return off;
}

static struct obj *area_get(struct hunk_area *area, size_t off)
{
    assert(area);
    assert(off < area->curr);
    return (struct obj *) (area->data + off);
}

static void area_step(struct hunk_area *area, struct hunk *hunk)
{
    if (!area) return;
    
    for (size_t off = 0; off < area->curr; off += area->len) {
        struct obj *obj = area_get(area, off);
        obj_step(obj, hunk);
    }
}


// -----------------------------------------------------------------------------
// hunk
// -----------------------------------------------------------------------------

struct hunk
{
    struct star star;

    id_t ids;
    struct htable index;
    struct hunk_area *areas[hunk_classes];
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
    for (size_t class = 0; class < hunk_classes; ++class)
        free(hunk->areas[class]);

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

    size_t class = ret.value >> 32;
    size_t off = ret.value & ((1UL << 32) - 1);

    return area_get(hunk->areas[class], off);
}

struct obj *hunk_obj_alloc(struct hunk *hunk, item_t type, size_t len)
{
    size_t class = len / cache_line;

    assert(len % cache_line == 0);
    assert(class < hunk_classes);

    size_t off = area_append(&hunk->areas[class], class);
    struct obj *obj = area_get(hunk->areas[class], off);
    obj->id = make_id(type, hunk->ids++);
    
    struct htable_ret ret = htable_put(&hunk->index, obj->id, class << 32 | off);
    assert(ret.ok);

    return obj;
}

void hunk_step(struct hunk *hunk)
{
    for (size_t class = 0; class < hunk_classes; ++class)
        area_step(hunk->areas[class], hunk);
}

size_t hunk_harvest(struct hunk *hunk, item_t type, size_t count)
{
    if (type < elem_natural_first || type > elem_natural_last) return 0;
    
    size_t index = type - elem_natural_first;
    count = i64_min(hunk->star.elements[index], count);
    hunk->star.elements[index] -= count;
    return count;
}
