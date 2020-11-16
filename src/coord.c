/* coord.c
   Rémi Attab (remi.attab@gmail.com), 12 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "coord.h"


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

static inline char hexchar(uint64_t v, size_t shift, size_t mask) {
    v = (v >> shift) & ((1 << mask) - 1);
    return v < 10 ? '0' + v : (v - 10) + 'a';
}

void coord_str(struct coord coord, char *str, size_t len)
{
    assert(len >= coord_str_len);

    size_t i = 0;
    str[i++] = hexchar(coord.x, 28, 4);
    str[i++] = hexchar(coord.x, 24, 4);
    str[i++] = '.';
    str[i++] = hexchar(coord.x, 20, 4);
    str[i++] = hexchar(coord.x, 16, 4);
    str[i++] = '.';
    str[i++] = hexchar(coord.x, 12, 4);
    str[i++] = hexchar(coord.x, 8, 4);
    str[i++] = hexchar(coord.x, 4, 4);
    str[i++] = hexchar(coord.x, 0, 4);
    str[i++] = 'x';
    str[i++] = hexchar(coord.y, 28, 4);
    str[i++] = hexchar(coord.y, 24, 4);
    str[i++] = '.';
    str[i++] = hexchar(coord.y, 20, 4);
    str[i++] = hexchar(coord.y, 16, 4);
    str[i++] = '.';
    str[i++] = hexchar(coord.y, 12, 4);
    str[i++] = hexchar(coord.y, 8, 4);
    str[i++] = hexchar(coord.y, 4, 4);
    str[i++] = hexchar(coord.y, 0, 4);

    assert(i == coord_str_len);
}

// -----------------------------------------------------------------------------
// scale
// -----------------------------------------------------------------------------

inline scale_t scale_inc(scale_t scale, int dir)
{
    if (scale == 1 && dir < 0) return scale;
    if (scale < (1 << 4)) return scale + dir;

    uint64_t delta = (1 << (bits_log2(scale) - 4));
    return dir < 0 ? scale - delta : scale + delta;
}


void scale_str(scale_t scale, char *str, size_t len)
{
    assert(len >= scale_str_len);

    size_t i = 0;
    size_t msb = bits_log2(scale);
    uint64_t top = scale < scale_base ? 0 : msb - 8;
    uint64_t bot = scale < scale_base ? (uint64_t)scale :
        (((uint64_t) scale) & ((1 << msb) - 1)) >> (msb - 8);

    str[i++] = 'x';
    str[i++] = hexchar(top, 4, 4);
    str[i++] = hexchar(top, 0, 4);
    str[i++] = '.';
    str[i++] = hexchar(bot, 4, 4);
    str[i++] = hexchar(bot, 0, 4);

    assert(i == scale_str_len);
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
