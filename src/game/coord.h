/* coord.h
   Rémi Attab (remi.attab@gmail.com), 25 Oct 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "SDL.h"

// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

enum
{
    coord_top_bits = 8,
    coord_top_size = 1 << coord_top_bits,
    coord_top_mask = coord_top_size - 1,

    coord_area_bits = 8,
    coord_area_size = 1 << coord_area_bits,
    coord_area_mask = coord_area_size - 1,

    coord_sector_bits = 16,
    coord_sector_size = 1 << coord_sector_bits,
    coord_sector_mask = coord_sector_size - 1,
};

static_assert(coord_top_bits + coord_area_bits + coord_sector_bits == 32);


legion_packed struct coord
{
    uint32_t x, y;
};

inline bool coord_null(struct coord coord)
{
    return coord.x == 0 && coord.y == 0;
}

inline bool coord_eq(struct coord rhs, struct coord lhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

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

inline uint64_t coord_to_id(struct coord coord)
{
    return (((uint64_t) coord.x) << 32) | coord.y;
}

inline struct coord id_to_coord(uint64_t id)
{
    return (struct coord) {
        .x = id >> 32,
        .y = id & ((uint32_t) -1),
    };
}

enum { coord_str_len = (2+1+2+1+4)*2 + 3 };

void coord_str(struct coord coord, char *str, size_t len);


// -----------------------------------------------------------------------------
// rect
// -----------------------------------------------------------------------------

struct rect
{
    struct coord top, bot;
};

inline bool rect_contains(const struct rect *r, struct coord coord)
{
    return
        (coord.x >= r->top.x && coord.x < r->bot.x) &&
        (coord.y >= r->top.y && coord.y < r->bot.y);
}


inline bool rect_intersect(const struct rect *lhs, struct rect *rhs)
{
    return
        (lhs->top.x >= rhs->bot.x && lhs->bot.x < rhs->top.x) ||
        (lhs->top.y >= rhs->bot.y && lhs->bot.y < rhs->top.y);
}


// -----------------------------------------------------------------------------
// scale
// -----------------------------------------------------------------------------

typedef int64_t scale_t;
enum { scale_base = 1 << 8 };

inline scale_t scale_init() { return scale_base; }

scale_t scale_inc(scale_t scale, int dir);

inline int64_t scale_mult(scale_t scale, int64_t value)
{
    return (value * scale) / scale_base;
}

inline int64_t scale_div(scale_t scale, int64_t value)
{
    return (value * scale_base) / scale;
}

enum { scale_str_len = 1+2+1+2 };
void scale_str(scale_t, char *str, size_t len);


// -----------------------------------------------------------------------------
// project
// -----------------------------------------------------------------------------

struct coord project_coord(
        SDL_Rect rect, struct coord center, scale_t scale, SDL_Point origin);

struct rect project_coord_rect(
        SDL_Rect rect, struct coord center, scale_t scale, const SDL_Rect *origin);

SDL_Point project_sdl(
        SDL_Rect rect, struct coord center, scale_t scale, struct coord origin);
