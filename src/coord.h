/* coord.h
   Rémi Attab (remi.attab@gmail.com), 25 Oct 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "utils.h"

#include <stdint.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

static const uint32_t coord_area_max = 1 << 8;
static const uint32_t coord_sector_max = 1 << 8;
static const uint32_t coord_system_max = 1 << 16;


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
    return (struct coord) {
        .x = (coord.x >> 22) << 22,
        .y = (coord.y >> 22) << 22,
    };
}

inline struct coord coord_sector(struct coord coord)
{
    return (struct coord) {
        .x = (coord.x >> 12) << 12,
        .y = (coord.y >> 12) << 12,
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

enum { coord_str_len = (2+1+2+1+4)*2 + 1 };

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
        SDL_Rect rect, struct coord center, scale_t scale, SDL_Rect origin);

SDL_Point project_sdl(
        SDL_Rect rect, struct coord center, scale_t scale, struct coord origin);
