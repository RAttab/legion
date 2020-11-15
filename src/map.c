/* map.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "map.h"

#include "sector.h"
#include "color.h"
#include "core.h"

// -----------------------------------------------------------------------------
// struct
// -----------------------------------------------------------------------------

struct map
{
    struct sector *sector;
    struct coord pos;
    scale_t scale;

    SDL_Rect rect;
    SDL_Texture* tex;

    bool panning;
};


// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

struct map *map_new()
{
    struct map *map = calloc(1, sizeof(*map));
    *map = (struct map) {
        .sector = core.state.sector,
        .pos = (struct coord) {
            .x = core.state.sector->coord.x + coord_sector_max / 2,
            .y = core.state.sector->coord.y + coord_sector_max / 2,
        },
        .scale = scale_init(),
        .rect = core.rect,
        .tex = NULL,
        .panning = false,
    };

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

struct coord map_project_coord(struct map *map, SDL_Point sdl)
{
    return project_coord(map->rect, map->pos, map->scale, sdl);
}

struct rect map_project_coord_rect(struct map *map, SDL_Rect sdl)
{
    return project_coord_rect(map->rect, map->pos, map->scale, sdl);
}

SDL_Point map_project_sdl(struct map *map, struct coord coord)
{
    return project_sdl(map->rect, map->pos, map->scale, coord);
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

bool map_event(struct map *map, SDL_Event *event)
{
    switch (event->type)
    {

    case SDL_KEYUP: {
        if (event->key.keysym.sym) core_quit();
        break;
    }

    case SDL_MOUSEWHEEL: {
        map->scale = scale_inc(map->scale, -event->wheel.y);
        break;
    }

    case SDL_MOUSEMOTION: {
        if (map->panning) {
            map->pos.x -= scale_mult(map->scale, event->motion.xrel);
            map->pos.y -= scale_mult(map->scale, event->motion.yrel);
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
        if (b->button == SDL_BUTTON_LEFT) map->panning = false;
        break;
    }

    }

    return false;
}

// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

enum { px_star = 20 };

static void render_sector(struct map *map, SDL_Renderer *renderer)
{
    struct rect rect = map_project_coord_rect(map, map->rect);

    for (size_t i = 0; i < core.state.sector->systems_len; ++i) {
        struct system *system = &core.state.sector->systems[i];
        if (!rect_contains(&rect, system->coord)) continue;

        SDL_Point pos = map_project_sdl(map, system->coord);

        size_t px = scale_div(map->scale, px_star);
        SDL_Rect src = { .x = 0, .y = 0, .w = 100, .h = 100 };
        SDL_Rect dst = {
            .x = pos.x - px / 2,
            .y = pos.y - px / 2,
            .h = px, .w = px
        };

        struct rgb rgb = spectrum_rgb(32 - bits_log2(system->star), 32);
        rgb = SDL_PointInRect(&core.cursor.point, &dst) ?
            desaturate(rgb, .5) : desaturate(rgb, .8);

        sdl_err(SDL_SetTextureBlendMode(map->tex, SDL_BLENDMODE_ADD));
        sdl_err(SDL_SetTextureColorMod(map->tex, rgb.r, rgb.g, rgb.b));
        sdl_err(SDL_RenderCopy(renderer, map->tex, &src, &dst));
    }
}
        
void map_render(struct map *map, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &core.rect);

    render_sector(map, renderer);
}
