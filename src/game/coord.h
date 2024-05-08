/* coord.h
   Rémi Attab (remi.attab@gmail.com), 25 Oct 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

enum : uint32_t
{
    coord_sector_bits = 16,
    coord_sector_size = 1 << coord_sector_bits,
    coord_sector_mask = coord_sector_size - 1,

    coord_area_bits = 8,
    coord_area_size = 1 << coord_area_bits,
    coord_area_mask = coord_area_size - 1,
    coord_area_inc = 1 << (coord_area_bits + coord_sector_bits),

    coord_top_bits = 8,
    coord_top_size = 1 << coord_top_bits,
    coord_top_mask = coord_top_size - 1,

    coord_mid = UINT32_MAX / 2,
};

static_assert(coord_top_bits + coord_area_bits + coord_sector_bits == 32);


struct legion_packed coord
{
    uint32_t x, y;
};

static_assert(sizeof(struct coord) == 8);


inline struct coord make_coord(uint32_t x, uint32_t y)
{
    return (struct coord) { .x = x, .y = y };
}

inline struct coord coord_center(void)
{
    return make_coord(UINT32_MAX / 2, UINT32_MAX / 2);
}

inline struct coord coord_nil(void) { return make_coord(0,0); }

inline bool coord_is_nil(struct coord coord)
{
    return coord.x == 0 && coord.y == 0;
}

inline bool coord_eq(struct coord lhs, struct coord rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline int coord_cmp(struct coord lhs, struct coord rhs)
{
    if (coord_eq(lhs, rhs)) return 0;
    if (lhs.x < rhs.x) return -1;
    if (lhs.x > rhs.x) return 1;
    if (lhs.y < rhs.y) return -1;
    if (lhs.y > rhs.y) return 1;
    assert(false);
}

inline bool coord_validate(vm_word word) { return word != 0; }

inline struct coord coord_area(struct coord coord)
{
    const size_t bits = coord_sector_bits + coord_area_bits;
    return (struct coord) {
        .x = (coord.x >> bits) << bits,
        .y = (coord.y >> bits) << bits,
    };
}

inline struct coord coord_sector(struct coord coord)
{
    const size_t bits = coord_sector_bits;
    return (struct coord) {
        .x = (coord.x >> bits) << bits,
        .y = (coord.y >> bits) << bits,
    };
}

inline uint64_t coord_dist_2(struct coord base, struct coord target)
{
    uint64_t dx = target.x < base.x ? base.x - target.x : target.x - base.x;
    uint64_t dy = target.y < base.y ? base.y - target.y : target.y - base.y;
    return dx*dx + dy*dy;
}

inline uint64_t coord_dist(struct coord base, struct coord target)
{
    double dx = target.x < base.x ? base.x - target.x : target.x - base.x;
    double dy = target.y < base.y ? base.y - target.y : target.y - base.y;
    return sqrt(dx*dx + dy*dy);
}

inline uint64_t coord_to_u64(struct coord coord)
{
    return (((uint64_t) coord.x) << 32) | coord.y;
}

inline struct coord coord_from_u64(uint64_t id)
{
    return (struct coord) {
        .x = id >> 32,
        .y = id & ((uint32_t) -1),
    };
}

enum : size_t { coord_str_len = (2+1+2+1+4)*2 + 3 };

size_t coord_str(struct coord coord, char *str, size_t len);


// -----------------------------------------------------------------------------
// rect
// -----------------------------------------------------------------------------

struct coord_rect
{
    struct coord top, bot;
};

inline struct coord_rect make_coord_rect(struct coord a, struct coord b)
{
    return (struct coord_rect) {
        .top = { .x = legion_min(a.x, b.x), .y = legion_min(a.y, b.y) },
        .bot = { .x = legion_max(a.x, b.x), .y = legion_max(a.y, b.y) },
    };
}

inline bool coord_rect_contains(const struct coord_rect r, struct coord coord)
{
    return
        (coord.x >= r.top.x && coord.x < r.bot.x) &&
        (coord.y >= r.top.y && coord.y < r.bot.y);
}


inline bool coord_rect_intersect(
        const struct coord_rect lhs, struct coord_rect rhs)
{
    return
        lhs.top.x <= rhs.bot.x && rhs.top.x <= lhs.bot.x &&
        lhs.top.y <= rhs.bot.y && rhs.top.y <= lhs.bot.y;
}

inline struct coord coord_rect_center(const struct coord_rect r)
{
    return (struct coord) {
        .x = r.top.x + ((r.bot.x - r.top.x) / 2),
        .y = r.top.y + ((r.bot.y - r.top.y) / 2),
    };
}

inline struct coord coord_rect_next_sector(
        struct coord_rect rect, struct coord it)
{
    if (coord_is_nil(it)) return coord_sector(rect.top);

    if (it.x < rect.bot.x)
        return make_coord(u32_saturate_add(it.x, coord_sector_size), it.y);

    if (it.y < rect.bot.y) {
        const size_t bits = coord_sector_bits;
        return make_coord(
                (rect.top.x >> bits) << bits,
                u32_saturate_add(it.y, coord_sector_size));
    }

    return coord_nil();
}

inline struct coord coord_rect_next_area(
        struct coord_rect rect, struct coord it)
{
    if (coord_is_nil(it)) return coord_area(rect.top);

    if (it.x < rect.bot.x)
        return make_coord(u32_saturate_add(it.x, coord_area_inc), it.y);

    if (it.y < rect.bot.y) {
        const size_t bits = coord_sector_bits + coord_area_bits;
        return make_coord(
                (rect.top.x >> bits) << bits,
                u32_saturate_add(it.y, coord_area_inc));
    }

    return coord_nil();
}


// -----------------------------------------------------------------------------
// scale
// -----------------------------------------------------------------------------

typedef int64_t coord_scale;
enum : coord_scale
{
    coord_scale_bits = 4,
    coord_scale_min = 1 << coord_scale_bits,
};

inline coord_scale coord_scale_init() { return coord_scale_min; }

coord_scale coord_scale_inc(coord_scale scale, int dir);

inline int64_t coord_scale_mult(coord_scale scale, int64_t value)
{
    return (value * scale) / coord_scale_min;
}

inline int64_t coord_scale_div(coord_scale scale, int64_t value)
{
    return (value * coord_scale_min) / scale;
}

constexpr size_t coord_scale_str_len = 1+2+1+2;
size_t coord_scale_str(coord_scale, char *str, size_t len);
