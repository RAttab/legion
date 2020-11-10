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

static const uint32_t coord_area_max = 1 << 6;
static const uint32_t coord_sector_max = 1 << 10;
static const uint32_t coord_star_max = 1 << 16;


legion_packed struct coord
{
    uint32_t x, y;
};

inline bool coord_null(struct coord coord)
{
    return coord.x == 0 && coord.y == 0;
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

typedef  uint16_t scale_t;
enum { scale_base = 128 };

inline scale_t scale_init() { return scale_base; }
inline scale_t scale_inc(scale_t scale, int delta)
{
    scale += delta;
    return
        scale == 0 ? 1 :
        scale == UINT16_MAX ? UINT16_MAX -1 :
        scale;
}

inline int64_t scale_mult(scale_t scale, int64_t value)
{
    return (value * scale) / scale_base;
}
inline int64_t scale_div(scale_t scale, int64_t value)
{
    return (value * scale_base) / scale;
}

// -----------------------------------------------------------------------------
// project
// -----------------------------------------------------------------------------

inline struct coord
project_coord(SDL_Rect rect, struct coord center, scale_t scale, SDL_Point origin)
{
    int64_t x = origin.x, y = origin.y;

    int64_t rel_x = scale_mult(scale, x - rect.x - rect.w / 2);
    int64_t rel_y = scale_mult(scale, y - rect.y - rect.h / 2);

    return (struct coord) {
        .x = i64_clamp(center.x + rel_x, 0, UINT32_MAX),
        .y = i64_clamp(center.y + rel_y, 0, UINT32_MAX),
    };
}

inline struct rect
project_rect_coord(SDL_Rect rect, struct coord center, scale_t scale, SDL_Rect origin)
{
    return (struct rect) {
        .top = project_coord(rect, center, scale,
                (SDL_Point){.x = origin.x, .y = origin.y }),

        .bot = project_coord(rect, center, scale, (SDL_Point){
                    .x = origin.x + origin.w,
                    .y = origin.y + origin.h }),
    };
};

inline SDL_Point
project_ui(SDL_Rect rect, struct coord center, scale_t scale, struct coord origin)
{
    int64_t x = origin.x, y = origin.y;

    int64_t rel_x = scale_div(scale, x - center.x);
    int64_t rel_y = scale_div(scale, y - center.y);

    return (SDL_Point) {
        .x = i64_clamp(rel_x + rect.w / 2 + rect.x, rect.x, rect.x + rect.w),
        .y = i64_clamp(rel_y + rect.h / 2 + rect.y, rect.y, rect.y + rect.h),
    };
}
