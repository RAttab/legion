/* map.c
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "render/ui.h"
#include "render/render.h"
#include "game/world.h"
#include "db/res.h"
#include "utils/color.h"
#include "utils/hset.h"

static void ui_map_free(void *);
static void ui_map_update(void *, struct proxy *);
static bool ui_map_event(void *, SDL_Event *);
static void ui_map_render(void *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// struct
// -----------------------------------------------------------------------------

struct ui_map
{
    struct coord pos;
    coord_scale scale;

    SDL_Texture* tex;
    SDL_Rect tex_star;
    SDL_Rect tex_active;

    bool panning, panned;
};


enum
{
    // Number of pixels per star at scale_base (not
    // map_scale_default). Basically it needs to be tweaked to a number that's
    // big enough to see and click on but not too big that there are overlaps
    // between stars during gen.
    ui_map_star_px = 650,

    // Tweaked in relation to map_star_px so that our default view isn't
    // useless.
    ui_map_scale_default = scale_base << 5,

    // As we zoom out we need to pull more nad more sector data which becomes
    // too expansive. These determine at which zoom threshold do we stop pulling
    // some data and displaying things like the sector or area grid.
    ui_map_thresh_stars = scale_base << 0x8,
    ui_map_thresh_sector_low = scale_base << 0x7,
    ui_map_thresh_sector_high = scale_base << 0xE,
};



// -----------------------------------------------------------------------------
// basics
// -----------------------------------------------------------------------------

void ui_map_alloc(struct ui_view_state *state)
{
    struct ui_map *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_map) {
        .pos = proxy_home(render.proxy),
        .scale = ui_map_scale_default,
        .tex = NULL,
        .panning = false,
        .panned = false,
    };

    ui->tex = db_img_map(render.renderer);
    ui->tex_star = (SDL_Rect) { .x = 0, .y = 0, .w = 100, .h = 100 };
    ui->tex_active = (SDL_Rect) { .x = 100, .y = 0, .w = 100, .h = 100 };
    
    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_map,
        .slots = ui_slot_back,
        .fn = {
            .free = ui_map_free,
            .update_frame = ui_map_update,
            .event = ui_map_event,
            .render = ui_map_render,
        },
    };
}

static void ui_map_free(void *state)
{
    struct ui_map *ui = state;
    SDL_DestroyTexture(ui->tex);
}

void ui_map_show(struct coord coord)
{
    struct ui_map *ui = ui_state(ui_view_map);

    if (!coord_is_nil(coord)) ui->pos = coord;
    if (coord_is_nil(ui->pos)) ui->pos = proxy_home(render.proxy);
    ui->scale = ui_map_scale_default;

    ui_show(ui_view_map);
}

void ui_map_goto(struct coord coord)
{
    if (ui_slot(ui_slot_back) == ui_view_map)
        ui_map_show(coord);
}

static void ui_map_update(void *state, struct proxy *proxy)
{
    struct ui_map *ui = state;
    if (coord_is_nil(ui->pos)) ui->pos = proxy_home(proxy);
}

coord_scale ui_map_scale(void)
{
    struct ui_map *ui = ui_state(ui_view_map);
    return ui->scale;
}


// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

static struct coord ui_map_project_coord(struct ui_map *ui, SDL_Point sdl)
{
    SDL_Rect rect = render.rect;
    int64_t x = sdl.x, y = sdl.y; // needed as a signed int

    int64_t rel_x = scale_mult(ui->scale, x - rect.x - rect.w / 2);
    int64_t rel_y = scale_mult(ui->scale, y - rect.y - rect.h / 2);

    return (struct coord) {
        .x = ui->pos.x + rel_x,
        .y = ui->pos.y + rel_y,
    };

}

static struct rect ui_map_project_coord_rect(
        struct ui_map *ui, const SDL_Rect *sdl)
{
    return (struct rect) {
        .top = ui_map_project_coord(ui, (SDL_Point) {
                    .x = sdl->x,
                    .y = sdl->y
                }),
        .bot = ui_map_project_coord(ui, (SDL_Point) {
                    .x = sdl->x + sdl->w,
                    .y = sdl->y + sdl->h
                }),
    };
}

static SDL_Point ui_map_project_sdl(struct ui_map *ui, struct coord coord)
{
    SDL_Rect rect = render.rect;
    int64_t x = coord.x, y = coord.y; // needed as a signed int

    int64_t rel_x = scale_div(ui->scale, x - ui->pos.x);
    int64_t rel_y = scale_div(ui->scale, y - ui->pos.y);

    return (SDL_Point) {
        .x = rel_x + rect.w / 2 + rect.x,
        .y = rel_y + rect.h / 2 + rect.y,
    };
}

struct coord ui_map_coord(void)
{
    struct ui_map *ui = ui_state(ui_view_map);
    return ui_map_project_coord(ui, ui_cursor_point());
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static void ui_map_event_user(struct ui_map *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case ev_state_load: {
        ui->pos = proxy_home(render.proxy);
        return;
    }

    default: { return; }
    }
}

static bool ui_map_event(void *state, SDL_Event *event)
{
    struct ui_map *ui = state;
    
    if (event->type == render.event)
        ui_map_event_user(ui, event);

    switch (event->type)
    {

    case SDL_MOUSEWHEEL: {
        ui->scale = scale_inc(ui->scale, -event->wheel.y);
        break;
    }

    case SDL_MOUSEMOTION: {
        if (!ui->panning)  break;

        int64_t xrel = scale_mult(ui->scale, event->motion.xrel);
        ui->pos.x = i64_clamp(ui->pos.x - xrel, 0, UINT32_MAX);

        int64_t yrel = scale_mult(ui->scale, event->motion.yrel);
        ui->pos.y = i64_clamp(ui->pos.y - yrel, 0, UINT32_MAX);

        ui->panned = true;
        break;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_MouseButtonEvent *b = &event->button;
        if (b->button == SDL_BUTTON_LEFT) ui->panning = true;
        break;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_MouseButtonEvent *b = &event->button;
        if (b->button != SDL_BUTTON_LEFT) break;

        ui->panning = false;
        if (ui->panned) { ui->panned = false; break; }
        if (ui->scale >= ui_map_thresh_stars) break;

        SDL_Point point = ui_cursor_point();
        size_t px = scale_div(ui->scale, ui_map_star_px);
        struct rect rect = ui_map_project_coord_rect(ui, &(SDL_Rect) {
                    .x = point.x - px / 2,
                    .y = point.y - px / 2,
                    .h = px, .w = px,
                });

        const struct star *star = proxy_star_in(render.proxy, rect);
        if (star) ui_star_show(star->coord);

        break;
    }

    }

    return false;
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void ui_map_render_areas(struct ui_map *ui, SDL_Renderer *renderer)
{
    struct rect rect = ui_map_project_coord_rect(ui, &render.rect);

    struct coord it = rect_next_area(rect, coord_nil());
    for (; !coord_is_nil(it); it = rect_next_area(rect, it)) {
        SDL_Point top = ui_map_project_sdl(ui, it);
        SDL_Point bot = ui_map_project_sdl(ui, make_coord(
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

static void ui_map_render_sectors(struct ui_map *ui, SDL_Renderer *renderer)
{
    struct rect rect = ui_map_project_coord_rect(ui, &render.rect);

    struct coord it = rect_next_sector(rect, coord_nil());
    for (; !coord_is_nil(it); it = rect_next_sector(rect, it)) {
        SDL_Point top = ui_map_project_sdl(ui, it);
        SDL_Point bot = ui_map_project_sdl(ui, make_coord(
                it.x + coord_sector_size,
                it.y + coord_sector_size));

        SDL_Rect sdl_rect = {
            .x = top.x, .y = top.y,
            .w = bot.x - top.x,
            .h = bot.y - top.y,
        };

        if (proxy_active_sector(render.proxy, it)) {
            uint8_t alpha = 0xFF;
            if (ui->scale < ui_map_thresh_stars)
                alpha = alpha * u64_log2(ui->scale) / u64_log2(ui_map_thresh_sector_low);
            rgba_render(make_rgba(0x00, 0x33, 0x00, alpha), renderer);
            sdl_err(SDL_RenderFillRect(renderer, &sdl_rect));
        }

        rgba_render(make_rgba(0x00, 0x33, 0x00, 0x88), renderer);
        sdl_err(SDL_RenderDrawRect(renderer, &sdl_rect));
    }
}

static void ui_map_render_lanes(
        struct ui_map *ui, SDL_Renderer *renderer, struct coord star)
{
    const struct hset *lanes = proxy_lanes_for(render.proxy, star);
    if (!lanes || !lanes->len) return;

    SDL_Point src = ui_map_project_sdl(ui, star);
    rgba_render(rgba_gray_a(0xAA, 0xAA), renderer);

    for (hset_it it = hset_next(lanes, NULL); it; it = hset_next(lanes, it)) {
        SDL_Point dst = ui_map_project_sdl(ui, coord_from_u64(*it));
        sdl_err(SDL_RenderDrawLine(renderer, src.x, src.y, dst.x, dst.y));
    }
}

static void ui_map_render_stars(struct ui_map *ui, SDL_Renderer *renderer)
{
    struct rect rect = ui_map_project_coord_rect(ui, &render.rect);
    struct proxy_render_it it = proxy_render_it(render.proxy, rect);

    const struct star *star = NULL;
    while ((star = proxy_render_next(render.proxy, &it))) {
        SDL_Point pos = ui_map_project_sdl(ui, star->coord);

        size_t px = scale_div(ui->scale, ui_map_star_px);
        SDL_Rect dst = {
            .x = pos.x - px / 2,
            .y = pos.y - px / 2,
            .h = px, .w = px
        };

        struct hsv hsv = {
            .h = ((double) star->hue) / 360,
            .s = 1.0 - (((double) star->energy) / UINT16_MAX),
            .v = ui_cursor_in(&dst) ? 0.8 : 0.5,
        };
        struct rgba rgb = hsv_to_rgb(hsv);

        sdl_err(SDL_SetTextureAlphaMod(ui->tex, 0xFF));
        sdl_err(SDL_SetTextureBlendMode(ui->tex, SDL_BLENDMODE_ADD));
        sdl_err(SDL_SetTextureColorMod(ui->tex, rgb.r, rgb.g, rgb.b));
        sdl_err(SDL_RenderCopy(renderer, ui->tex, &ui->tex_star, &dst));

        if (proxy_active_star(render.proxy, star->coord)) {
            ui_map_render_lanes(ui, renderer, star->coord);

            dst = (SDL_Rect) {
                .x = pos.x - px / 2,
                .y = pos.y - px,
                .h = px / 2, .w = px,
            };
            sdl_err(SDL_SetTextureAlphaMod(ui->tex, 0x88));
            sdl_err(SDL_SetTextureColorMod(ui->tex, 0x88, 0xFF, 0x88));
            sdl_err(SDL_RenderCopy(renderer, ui->tex, &ui->tex_active, &dst));
        }
    }
}

static void ui_map_render(void *state, struct ui_layout *, SDL_Renderer *renderer)
{
    struct ui_map *ui = state;
    
    rgba_render(rgba_black(), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &render.rect));

    if (ui->scale >= ui_map_thresh_sector_low && ui->scale < ui_map_thresh_sector_high)
        ui_map_render_sectors(ui, renderer);

    if (ui->scale < ui_map_thresh_stars)
        ui_map_render_stars(ui, renderer);
    else ui_map_render_areas(ui, renderer);
}
