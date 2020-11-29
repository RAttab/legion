/* hunk.c
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "hunk.h"

// -----------------------------------------------------------------------------
// area
// -----------------------------------------------------------------------------

enum { hunk_classes = 4; };

struct hunk_area
{
    size_t len;
    size_t curr, cap;
    uint64_t __pad__[8 - 2];

    uint8_t data[];
};

static_assert(sizeof(struct hunk_area) == 64);

static size_t area_append(struct hunk_area **area, size_t class)
{
    size_t len = 64 * class;

    if (area == NULL) {
        const size_t cap = 8;
        *area = malloc(sizeof(**area) + len * cap);
        *area->len = len;
        *area->curr = 0;
        *area->cap = cap;
    }

    if (*area->curr == *area->cap) {
        const size_t cap = *area->cap * 2;
        *area = realloc(*area, sizeof(**area) + *area->len * cap);
        *area->cap = cap;
    }

    assert(*area->curr + *area->len <= *area->cap);

    size_t off = *area->curr;
    memset(*area->data + off, 0, len);
    *area->curr += len;
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
    struct system system;

    id_t ids;
    struct htable index;
    struct hunk_area *areas[hunk_classes]
};

struct hunk *hunk_alloc(struct coord coord)
{
    struct hunk *hunk = calloc(1, sizeof(*hunk));
    hunk->ids = 1;

    struct system_desc *system = system_gen(coord);
    hunk->system = system->s;
    free(system);
}

void hunk_free(struct hunk *hunk)
{
    for (size_t class = 0; class < hunk_classes; ++class)
        free(hunk->areas[class]);

    htable_reset(&hunk->index);
    free(hunk);
}

struct obj *hunk_obj(struct hunk *hunk, id_t id)
{
    struct htable_ret ret = htable_get(&hunk->id, id);
    if (!ret.ok) return NULL;

    size_t class = ret.value >> 32;
    size_t off = ret.value & ((1U << 32) - 1);

    return area_get(hunk->areas[class], off);
}

struct obj *hunk_obj_alloc(struct hunk *hunk, size_t len)
{
    size_t class = len / 64;

    assert(len % 64 == 0);
    assert(class < area_hunk_cap);

    size_t off = area_append(&hunk->areas[class], class);
    struct obj *obj = area_get(hunk->areas[class], off);
    obj->id = make_id(type, hunk->ids++);
    
    struct htable_ret ret = htable_put(&hunk->id, obj->id, class << 32 | off);
    assert(ret.ok);

    return obj;
}

void hunk_step(struct hunk *hunk)
{
    for (size_t class = 0; class < hunk_classes; ++class)
        area_step(hunk->areas[class], hunk);
}
