/* coord.h
   Rémi Attab (remi.attab@gmail.com), 25 Oct 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "utils.h"

#include <stdint.h>

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
    return coord.x == 0 && coord.y == 0
}

inline struct coord coord_area(struct coord coord)
{
    return (struct coord) {
        .x = (coord.x >> 22) << 22;
        .y = (coord.y >> 22) << 22;
    };
}

inline struct coord coord_sector(struct coord coord)
{
    return (struct coord) {
        .x = (coord.x >> 12) << 12;
        .y = (coord.y >> 12) << 12;
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
        .y = id & ((uint32_t) -1);
    };
}
