/* map.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "render/ui.h"
#include "render/render.h"
#include "game/world.h"
#include "utils/color.h"
#include "utils/hset.h"


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

    bool active;
    bool panning, panned;
};

enum
{
    // Number of pixels per star at scale_base (not
    // map_scale_default). Basically it needs to be tweaked to a number that's
    // big enough to see and click on but not too big that there are overlaps
    // between stars during gen.
    map_star_px = 650,

    // Tweaked in relation to map_star_px so that our default view isn't
    // useless.
    map_scale_default = scale_base << 5,

    // As we zoom out we need to pull more nad more sector data which becomes
    // too expansive. These determine at which zoom threshold do we stop pulling
    // some data and displaying things like the sector or area grid.
    map_thresh_stars = scale_base << 0x8,
    map_thresh_sector_low = scale_base << 0x7,
    map_thresh_sector_high = scale_base << 0xE,
};


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

struct map *map_new(void)
{
    struct map *map = calloc(1, sizeof(*map));
    *map = (struct map) {
        .pos = proxy_home(render.proxy),
        .scale = map_scale_default,
        .tex = NULL,
        .active = true,
        .panning = false,
        .panned = false,
    };

    char path[PATH_MAX];
    sys_path_res("map.bmp", path, sizeof(path));

    SDL_Surface *bmp = sdl_ptr(SDL_LoadBMP(path));
    map->tex = sdl_ptr(SDL_CreateTextureFromSurface(render.renderer, bmp));
    SDL_FreeSurface(bmp);

    map->tex_star = (SDL_Rect) { .x = 0, .y = 0, .w = 100, .h = 100 };
    map->tex_active = (SDL_Rect) { .x = 100, .y = 0, .w = 100, .h = 100 };

    return map;
}

void map_free(struct map *map)
{
    SDL_DestroyTexture(map->tex);
}

bool map_active(struct map *map)
{
    return map->active;
}

scale_t map_scale(struct map *map)
{
    return map->scale;
}


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

static struct coord map_project_coord(struct map *map, SDL_Point sdl)
{
    SDL_Rect rect = render.rect;
    int64_t x = sdl.x, y = sdl.y; // needed as a signed int

    int64_t rel_x = scale_mult(map->scale, x - rect.x - rect.w / 2);
    int64_t rel_y = scale_mult(map->scale, y - rect.y - rect.h / 2);

    return (struct coord) {
        .x = map->pos.x + rel_x,
        .y = map->pos.y + rel_y,
    };

}

static struct rect map_project_coord_rect(struct map *map, const SDL_Rect *sdl)
{
    return (struct rect) {
        .top = map_project_coord(map, (SDL_Point) {
                    .x = sdl->x,
                    .y = sdl->y
                }),
        .bot = map_project_coord(map, (SDL_Point) {
                    .x = sdl->x + sdl->w,
                    .y = sdl->y + sdl->h
                }),
    };
}

static SDL_Point map_project_sdl(struct map *map, struct coord coord)
{
    SDL_Rect rect = render.rect;
    int64_t x = coord.x, y = coord.y; // needed as a signed int

    int64_t rel_x = scale_div(map->scale, x - map->pos.x);
    int64_t rel_y = scale_div(map->scale, y - map->pos.y);

    return (SDL_Point) {
        .x = rel_x + rect.w / 2 + rect.x,
        .y = rel_y + rect.h / 2 + rect.y,
    };
}

struct coord map_coord(struct map *map)
{
    return map_project_coord(map, render.cursor.point);
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static bool map_event_user(struct map *map, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_MAP_GOTO: {
        map->active = true;
        map->pos = coord_from_u64((uintptr_t) ev->user.data1);
        map->scale = map_scale_default;
        return false;
    }

    case EV_STATE_UPDATE: {
        if (coord_is_nil(map->pos))
            map->pos = proxy_home(render.proxy);
        return false;
    }

    case EV_STATE_LOAD: {
        map->active = true;
        map->pos = proxy_home(render.proxy);
        return false;
    }

    case EV_FACTORY_CLOSE: { map->active = true; return false; }
    case EV_FACTORY_SELECT: { map->active = false; return false; }

    default: { return false; }
    }
}

bool map_event(struct map *map, SDL_Event *event)
{
    if (event->type == render.event) return map_event_user(map, event);
    if (!map->active) return false;

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
            SDL_Point point = render.cursor.point;
            size_t px = scale_div(map->scale, map_star_px);
            struct rect rect = map_project_coord_rect(map, &(SDL_Rect) {
                        .x = point.x - px / 2,
                        .y = point.y - px / 2,
                        .h = px, .w = px,
                    });

            const struct star *star = proxy_star_in(render.proxy, rect);
            if (star) render_push_event(EV_STAR_SELECT, coord_to_u64(star->coord), 0);
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
    struct rect rect = map_project_coord_rect(map, &render.rect);

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
    struct rect rect = map_project_coord_rect(map, &render.rect);

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

        if (proxy_active_sector(render.proxy, it)) {
            uint8_t alpha = 0xFF;
            if (map->scale < map_thresh_stars)
                alpha = alpha * u64_log2(map->scale) / u64_log2(map_thresh_sector_low);
            rgba_render(make_rgba(0x00, 0x33, 0x00, alpha), renderer);
            sdl_err(SDL_RenderFillRect(renderer, &sdl_rect));
        }

        rgba_render(make_rgba(0x00, 0x33, 0x00, 0x88), renderer);
        sdl_err(SDL_RenderDrawRect(renderer, &sdl_rect));
    }
}

static void map_render_lanes(
        struct map *map, SDL_Renderer *renderer, struct coord star)
{
    const struct hset *lanes = proxy_lanes_for(render.proxy, star);
    if (!lanes || !lanes->len) return;

    SDL_Point src = map_project_sdl(map, star);
    rgba_render(rgba_gray_a(0xAA, 0xAA), renderer);

    for (hset_it_t it = hset_next(lanes, NULL); it; it = hset_next(lanes, it)) {
        SDL_Point dst = map_project_sdl(map, coord_from_u64(*it));
        sdl_err(SDL_RenderDrawLine(renderer, src.x, src.y, dst.x, dst.y));
    }
}

static void map_render_stars(struct map *map, SDL_Renderer *renderer)
{
    struct rect rect = map_project_coord_rect(map, &render.rect);
    struct proxy_render_it it = proxy_render_it(render.proxy, rect);

    const struct star *star = NULL;
    while ((star = proxy_render_next(render.proxy, &it))) {
        SDL_Point pos = map_project_sdl(map, star->coord);

        size_t px = scale_div(map->scale, map_star_px);
        SDL_Rect dst = {
            .x = pos.x - px / 2,
            .y = pos.y - px / 2,
            .h = px, .w = px
        };

        struct hsv hsv = {
            .h = ((double) star->hue) / 360,
            .s = 1.0 - (((double) star->energy) / UINT16_MAX),
            .v = SDL_PointInRect(&render.cursor.point, &dst) ? 0.8 : 0.5,
        };
        struct rgba rgb = hsv_to_rgb(hsv);

        sdl_err(SDL_SetTextureAlphaMod(map->tex, 0xFF));
        sdl_err(SDL_SetTextureBlendMode(map->tex, SDL_BLENDMODE_ADD));
        sdl_err(SDL_SetTextureColorMod(map->tex, rgb.r, rgb.g, rgb.b));
        sdl_err(SDL_RenderCopy(renderer, map->tex, &map->tex_star, &dst));

        if (proxy_active_star(render.proxy, star->coord)) {
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
    if (!map->active) return;

    rgba_render(rgba_black(), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &render.rect));

    if (map->scale >= map_thresh_sector_low && map->scale < map_thresh_sector_high)
        map_render_sectors(map, renderer);

    if (map->scale < map_thresh_stars)
        map_render_stars(map, renderer);
    else map_render_areas(map, renderer);
}
