/* map.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "map.h"

#include "render/color.h"
#include "render/core.h"
#include "game/galaxy.h"
#include "utils/log.h"


// -----------------------------------------------------------------------------
// struct
// -----------------------------------------------------------------------------

struct map
{
    struct coord pos;
    scale_t scale;

    SDL_Texture* tex;

    bool panning;
    bool panned;
};

enum { px_star = 1 << 10 };


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

struct map *map_new()
{
    struct map *map = calloc(1, sizeof(*map));
    *map = (struct map) {
        .pos = (struct coord) {
            .x = core.state.sector->coord.x + coord_sector_size / 2,
            .y = core.state.sector->coord.y + coord_sector_size / 2,
        },
        .scale = scale_init(),
        .tex = NULL,
        .panning = false,
        .panned = false,
    };

    map->scale <<= 6;

    char path[PATH_MAX];
    core_path_res("map.bmp", path, sizeof(path));

    SDL_Surface *bmp = sdl_ptr(SDL_LoadBMP(path));
    map->tex = sdl_ptr(SDL_CreateTextureFromSurface(core.renderer, bmp));
    SDL_FreeSurface(bmp);

    return map;
}

void map_free(struct map *map)
{
    SDL_DestroyTexture(map->tex);
}


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

scale_t map_scale(struct map *map)
{
    return map->scale;
}

struct coord map_project_coord(struct map *map, SDL_Point sdl)
{
    return project_coord(core.rect, map->pos, map->scale, sdl);
}

struct rect map_project_coord_rect(struct map *map, const SDL_Rect *sdl)
{
    return project_coord_rect(core.rect, map->pos, map->scale, sdl);
}

SDL_Point map_project_sdl(struct map *map, struct coord coord)
{
    return project_sdl(core.rect, map->pos, map->scale, coord);
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

bool map_event(struct map *map, SDL_Event *event)
{
    switch (event->type)
    {

    case SDL_MOUSEWHEEL: {
        map->scale = scale_inc(map->scale, -event->wheel.y);
        break;
    }

    case SDL_MOUSEMOTION: {
        if (map->panning) {
            int64_t xrel = scale_mult(map->scale, event->motion.xrel);
            map->pos.x = i64_clamp(map->pos.x - xrel, 0, UINT32_MAX);

            int64_t yrel = scale_mult(map->scale, event->motion.yrel);
            map->pos.y = i64_clamp(map->pos.y - yrel, 0, UINT32_MAX);

            map->panned = true;
        }
        break;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_MouseButtonEvent *b = &event->button;
        if (b->button == SDL_BUTTON_LEFT) map->panning = true;
        break;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_MouseButtonEvent *b = &event->button;
        if (b->button == SDL_BUTTON_LEFT) {
            if (!map->panned) {
                SDL_Point point = core.cursor.point;
                size_t px = scale_div(map->scale, px_star);
                struct rect rect = map_project_coord_rect(map, &(SDL_Rect) {
                            .x = point.x - px / 2,
                            .y = point.y - px / 2,
                            .h = px, .w = px,
                        });

                const struct star *star = sector_star(core.state.sector, &rect);
                if (star) core_push_event(EV_STAR_SELECT, (void *) star);
            }

            map->panning = false;
            map->panned = false;
        }
        break;
    }

    }

    return false;
}

// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void map_render_sector(struct map *map, SDL_Renderer *renderer)
{
    struct rect rect = map_project_coord_rect(map, &core.rect);

    for (size_t i = 0; i < core.state.sector->stars_len; ++i) {
        struct star *star = &core.state.sector->stars[i];
        if (!rect_contains(&rect, star->coord)) continue;

        SDL_Point pos = map_project_sdl(map, star->coord);

        size_t px = scale_div(map->scale, px_star);
        SDL_Rect src = { .x = 0, .y = 0, .w = 100, .h = 100 };
        SDL_Rect dst = {
            .x = pos.x - px / 2,
            .y = pos.y - px / 2,
            .h = px, .w = px
        };

        struct rgb rgb = spectrum_rgb(32 - u64_log2(star->power), 32);
        if (!SDL_PointInRect(&core.cursor.point, &dst)) {
            rgb = desaturate(rgb, .8);
        }

        sdl_err(SDL_SetTextureBlendMode(map->tex, SDL_BLENDMODE_ADD));
        sdl_err(SDL_SetTextureColorMod(map->tex, rgb.r, rgb.g, rgb.b));
        sdl_err(SDL_RenderCopy(renderer, map->tex, &src, &dst));
    }
}

void map_render(struct map *map, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &core.rect);

    map_render_sector(map, renderer);
}
