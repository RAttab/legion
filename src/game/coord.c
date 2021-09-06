/* coord.c
   Rémi Attab (remi.attab@gmail.com), 12 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "coord.h"
#include "utils/str.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

size_t coord_str(struct coord coord, char *str, size_t len)
{
    assert(len >= coord_str_len);

    size_t i = 0;
    str[i++] = str_hexchar(coord.x >> 28);
    str[i++] = str_hexchar(coord.x >> 24);
    str[i++] = '.';
    str[i++] = str_hexchar(coord.x >> 20);
    str[i++] = str_hexchar(coord.x >> 16);
    str[i++] = '.';
    str[i++] = str_hexchar(coord.x >> 12);
    str[i++] = str_hexchar(coord.x >> 8);
    str[i++] = str_hexchar(coord.x >> 4);
    str[i++] = str_hexchar(coord.x >> 0);
    str[i++] = ' ';
    str[i++] = 'x';
    str[i++] = ' ';
    str[i++] = str_hexchar(coord.y >> 28);
    str[i++] = str_hexchar(coord.y >> 24);
    str[i++] = '.';
    str[i++] = str_hexchar(coord.y >> 20);
    str[i++] = str_hexchar(coord.y >> 16);
    str[i++] = '.';
    str[i++] = str_hexchar(coord.y >> 12);
    str[i++] = str_hexchar(coord.y >> 8);
    str[i++] = str_hexchar(coord.y >> 4);
    str[i++] = str_hexchar(coord.y >> 0);

    assert(i == coord_str_len);
    return coord_str_len;
}

// -----------------------------------------------------------------------------
// scale
// -----------------------------------------------------------------------------

inline scale_t scale_inc(scale_t scale, int dir)
{
    if (scale == scale_base && dir < 0) return scale;
    if (scale < (1 << 4)) return scale + dir;

    uint64_t delta = (1 << (u64_log2(scale) - 4));
    return dir < 0 ? scale - delta : scale + delta;
}


size_t scale_str(scale_t scale, char *str, size_t len)
{
    assert(len >= scale_str_len);

    size_t i = 0;
    size_t msb = u64_log2(scale);
    uint64_t top = scale < scale_base ? 0 : msb - 8;
    uint64_t bot = scale < scale_base ? (uint64_t)scale :
        (((uint64_t) scale) & ((1 << msb) - 1)) >> (msb - 8);

    str[i++] = 'x';
    str[i++] = str_hexchar(top >> 4);
    str[i++] = str_hexchar(top >> 0);
    str[i++] = '.';
    str[i++] = str_hexchar(bot >> 4);
    str[i++] = str_hexchar(bot >> 0);

    assert(i == scale_str_len);
    return scale_str_len;
}


// -----------------------------------------------------------------------------
// project
// -----------------------------------------------------------------------------


struct coord project_coord(
        SDL_Rect rect, struct coord center, scale_t scale, SDL_Point origin)
{
    int64_t x = origin.x, y = origin.y;

    int64_t rel_x = scale_mult(scale, x - rect.x - rect.w / 2);
    int64_t rel_y = scale_mult(scale, y - rect.y - rect.h / 2);

    return (struct coord) {
        .x = i64_clamp(center.x + rel_x, 0, UINT32_MAX),
        .y = i64_clamp(center.y + rel_y, 0, UINT32_MAX),
    };
}

struct rect project_coord_rect(
        SDL_Rect rect, struct coord center, scale_t scale, const SDL_Rect *origin)
{
    return (struct rect) {
        .top = project_coord(rect, center, scale,
                (SDL_Point){.x = origin->x, .y = origin->y }),

        .bot = project_coord(rect, center, scale, (SDL_Point){
                    .x = origin->x + origin->w,
                    .y = origin->y + origin->h }),
    };
}

SDL_Point project_sdl(
        SDL_Rect rect, struct coord center, scale_t scale, struct coord origin)
{
    int64_t x = origin.x, y = origin.y;

    int64_t rel_x = scale_div(scale, x - center.x);
    int64_t rel_y = scale_div(scale, y - center.y);

    return (SDL_Point) {
        .x = i64_clamp(rel_x + rect.w / 2 + rect.x, rect.x, rect.x + rect.w),
        .y = i64_clamp(rel_y + rect.h / 2 + rect.y, rect.y, rect.y + rect.h),
    };
}
