/* map.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "map.h"

#include "render/color.h"
#include "render/core.h"
#include "game/world.h"
#include "utils/log.h"


// -----------------------------------------------------------------------------
// struct
// -----------------------------------------------------------------------------

struct map
{
    struct coord pos;
    scale_t scale;

    SDL_Texture* tex;
    SDL_Rect tex_star;
    SDL_Rect tex_active;

    bool panning;
    bool panned;
};

enum { px_star = 1 << 10 };

enum {
    map_scale_default = scale_base << 6,
    map_thresh_stars = scale_base << 0x8,
    map_thresh_sector_low = scale_base << 0x7,
    map_thresh_sector_high = scale_base << 0xE,
    map_thresh_sector_active = scale_base << 0xA,
};


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

struct map *map_new(void)
{
    struct map *map = calloc(1, sizeof(*map));
    *map = (struct map) {
        .pos = core.state.home,
        .scale = scale_init(),
        .tex = NULL,
        .panning = false,
        .panned = false,
    };

    map->scale = map_scale_default;

    char path[PATH_MAX];
    core_path_res("map.bmp", path, sizeof(path));

    SDL_Surface *bmp = sdl_ptr(SDL_LoadBMP(path));
    map->tex = sdl_ptr(SDL_CreateTextureFromSurface(core.renderer, bmp));
    SDL_FreeSurface(bmp);

    map->tex_star = (SDL_Rect) { .x = 0, .y = 0, .w = 100, .h = 100 };
    map->tex_active = (SDL_Rect) { .x = 100, .y = 0, .w = 100, .h = 100 };

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

static bool map_event_user(struct map *map, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_MAP_GOTO: {
        map->pos = id_to_coord((uintptr_t) ev->user.data1);
        map->scale = map_scale_default;
        return true;
    }

    default: { return false; }
    }
}

bool map_event(struct map *map, SDL_Event *event)
{
    if (event->type == core.event) return map_event_user(map, event);

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

        if (map->scale < map_thresh_stars) {
            SDL_Point point = core.cursor.point;
            size_t px = scale_div(map->scale, px_star);
            struct rect rect = map_project_coord_rect(map, &(SDL_Rect) {
                        .x = point.x - px / 2,
                        .y = point.y - px / 2,
                        .h = px, .w = px,
                    });

            const struct star *star = world_star(core.state.world, rect);
            if (star) core_push_event(EV_STAR_SELECT, (uintptr_t) star, 0);
        }

        break;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_MouseButtonEvent *b = &event->button;
        if (b->button == SDL_BUTTON_LEFT) {
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

static void map_render_areas(struct map *map, SDL_Renderer *renderer)
{
    struct rect rect = map_project_coord_rect(map, &core.rect);

    struct coord it = rect_next_area(rect, coord_nil());
    for (; !coord_is_nil(it); it = rect_next_area(rect, it)) {
        SDL_Point top = map_project_sdl(map, it);
        SDL_Point bot = map_project_sdl(map, make_coord(
                it.x + coord_area_inc,
                it.y + coord_area_inc));

        SDL_Rect sdl_rect = {
            .x = top.x, .y = top.y,
            .w = bot.x - top.x,
            .h = bot.y - top.y,
        };

        rgba_render(make_rgba(0x00, 0x00, 0x33, 0x88), renderer);
        sdl_err(SDL_RenderDrawRect(renderer, &sdl_rect));
    }
}

static void map_render_sectors(struct map *map, SDL_Renderer *renderer)
{
    struct rect rect = map_project_coord_rect(map, &core.rect);

    struct coord it = rect_next_sector(rect, coord_nil());
    for (; !coord_is_nil(it); it = rect_next_sector(rect, it)) {
        SDL_Point top = map_project_sdl(map, it);
        SDL_Point bot = map_project_sdl(map, make_coord(
                it.x + coord_sector_size,
                it.y + coord_sector_size));

        SDL_Rect sdl_rect = {
            .x = top.x, .y = top.y,
            .w = bot.x - top.x,
            .h = bot.y - top.y,
        };

        if (map->scale < map_thresh_sector_active) {
            struct sector *sector = world_sector(core.state.world, it);
            if (sector->chunks.len) {
                uint8_t alpha = 0xFF;
                if (map->scale < map_thresh_stars)
                    alpha = alpha * u64_log2(map->scale) / u64_log2(map_thresh_sector_low);
                rgba_render(make_rgba(0x00, 0x33, 0x00, alpha), renderer);
                sdl_err(SDL_RenderFillRect(renderer, &sdl_rect));
            }
        }

        rgba_render(make_rgba(0x00, 0x33, 0x00, 0x88), renderer);
        sdl_err(SDL_RenderDrawRect(renderer, &sdl_rect));
    }
}

static void map_render_lanes(
        struct map *map, SDL_Renderer *renderer, struct coord star)
{
    struct vec64 *lanes = world_lanes_list(core.state.world, coord);
    if (!lanes || !lanes->len) return;

    rgba_render(rgba_white(), renderer);

    SDL_Point src = map_project_coord(map, coord);
    for (size_t i = 0; i < lanes->len; ++i) {
        SDL_Point dst = map_project_coord(map, id_to_coord(lanes->data[i]));
        sdl_err(SDL_RenderDrawLine(renderer, src.x, src.y, dst.x, dst.y);
    }
}

static void map_render_stars(struct map *map, SDL_Renderer *renderer)
{
    struct rect rect = map_project_coord_rect(map, &core.rect);

    struct world_render_it it = world_render_it(core.state.world, rect);

    const struct star *star = NULL;
    while ((star = world_render_next(core.state.world, &it))) {
        SDL_Point pos = map_project_sdl(map, star->coord);

        size_t px = scale_div(map->scale, px_star);
        SDL_Rect dst = {
            .x = pos.x - px / 2,
            .y = pos.y - px / 2,
            .h = px, .w = px
        };

        struct rgb rgb = spectrum_rgb(32 - u64_log2(star->power), 32);
        if (!SDL_PointInRect(&core.cursor.point, &dst)) {
            rgb = desaturate(rgb, .8);
        }

        sdl_err(SDL_SetTextureAlphaMod(map->tex, 0xFF));
        sdl_err(SDL_SetTextureBlendMode(map->tex, SDL_BLENDMODE_ADD));
        sdl_err(SDL_SetTextureColorMod(map->tex, rgb.r, rgb.g, rgb.b));
        sdl_err(SDL_RenderCopy(renderer, map->tex, &map->tex_star, &dst));

        if (star->state == star_active) {
            map_render_lanes(map, renderer, star->coord);

            dst = (SDL_Rect) {
                .x = pos.x - px / 2,
                .y = pos.y - px,
                .h = px / 2, .w = px,
            };
            sdl_err(SDL_SetTextureAlphaMod(map->tex, 0x88));
            sdl_err(SDL_SetTextureColorMod(map->tex, 0x88, 0xFF, 0x88));
            sdl_err(SDL_RenderCopy(renderer, map->tex, &map->tex_active, &dst));
        }
    }
}

static void map_render_sectors(struct map *map, SDL_Renderer *renderer);

void map_render(struct map *map, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &core.rect);

    if (map->scale >= map_thresh_sector_low && map->scale < map_thresh_sector_high)
        map_render_sectors(map, renderer);

    if (map->scale < map_thresh_stars)
        map_render_stars(map, renderer);
    else map_render_areas(map, renderer);
}
