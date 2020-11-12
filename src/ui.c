/* ui.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui.h"

#include "coord.h"
#include "render.h"
#include "sector.h"
#include "panel.h"

enum { px_star = 20 };

// -----------------------------------------------------------------------------
// cursor
// -----------------------------------------------------------------------------

struct ui_cursor
{
    SDL_Point pos;
    uint8_t button;
    SDL_Texture *tex;
};

struct ui_cursor *ui_cursor_init(SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct ui_cursor *cursor = calloc(1, sizeof(*cursor));
    *cursor = (struct ui_cursor) {
        .pos = (SDL_Point){ .x = rect->w / 2, .y = rect->h / 2 },
        .button = 0,
        .tex = NULL,
    };

    SDL_Surface *surface = SDL_LoadBMP("./res/cursor.bmp");
    if (surface == NULL) {
        SDL_Log("unable to load cursor texture: %s", SDL_GetError());
        goto fail_bmp;
    }

    cursor->tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (cursor->tex == NULL) {
        SDL_Log("unable create cursor texture: %s", SDL_GetError());
        goto fail_tex;
    }

    if (SDL_SetRelativeMouseMode(true) < 0) {
        SDL_Log("unable to set relative mouse mode: %s", SDL_GetError());
        goto fail_mouse;
    }

    SDL_SetTextureBlendMode(cursor->tex, SDL_BLENDMODE_ADD);
    SDL_SetTextureColorMod(cursor->tex, 0xFF, 0xFF, 0xFF);

    return cursor;

  fail_mouse:
    SDL_DestroyTexture(cursor->tex);
  fail_tex:
    SDL_FreeSurface(surface);
  fail_bmp:
    free(cursor);
    return NULL;
}

static void ui_cursor_free(struct ui_cursor *cursor)
{
    SDL_DestroyTexture(cursor->tex);
    free(cursor);
}

static void cursor_render(struct ui_cursor *cursor, SDL_Renderer *renderer)
{
    SDL_RenderCopy(renderer, cursor->tex,
            &(SDL_Rect){ .x = 0, .y = 0, .w = 50, .h = 50 },
            &(SDL_Rect){ .x = cursor->pos.x, .y = cursor->pos.y, .w = 20, .h = 20 });
}

static void cursor_events(struct ui_cursor *cursor, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_MOUSEMOTION: {
        cursor->pos.x += event->motion.xrel;
        cursor->pos.y += event->motion.yrel;
        break;
    }

    case SDL_MOUSEBUTTONDOWN: {
        cursor->button = event->button.button;
        break;
    }

    case SDL_MOUSEBUTTONUP: {
        cursor->button = 0;
        break;
    }
    }
}


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct ui_core *
ui_core_init(SDL_Renderer *renderer, struct sector *sector, SDL_Rect *rect)
{
    struct ui_core *core = calloc(1, sizeof(*core));
    *core = (struct ui_core) {
        .sector = sector,
        .pos = (struct coord) {
            .x = sector->coord.x + coord_sector_max / 2,
            .y = sector->coord.y + coord_sector_max / 2,
        },
        .scale = scale_init(),
        .rect = *rect,
        .tex = NULL,
        .cursor = ui_cursor_init(renderer, rect),
        .p_coord = NULL,
        .selected = NULL,
    };


    SDL_Surface *surface = SDL_LoadBMP("./res/core.bmp");
    if (surface == NULL) {
        SDL_Log("unable to load core texture: %s", SDL_GetError());
        goto fail_bmp;
    }

    core->tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (core->tex == NULL) {
        SDL_Log("unable create core texture: %s", SDL_GetError());
        goto fail_tex;
    }

    core->p_coord = panel_coord_new(renderer, core);

    return core;

    SDL_DestroyTexture(core->tex);
  fail_tex:
    SDL_FreeSurface(surface);
  fail_bmp:
    free(core);
    return NULL;
}

void ui_core_free(struct ui_core *core)
{
    ui_cursor_free(core->cursor);

    SDL_DestroyTexture(core->tex);
    free(core);
}


static void core_render_galaxy(struct ui_core *core, SDL_Renderer *renderer)
{
    struct rect rect = project_coord_rect(
            core->rect, core->pos, core->scale, core->rect);

    for (size_t i = 0; i < core->sector->systems_len; ++i) {
        struct system *system = &core->sector->systems[i];
        if (!rect_contains(&rect, system->coord)) continue;

        SDL_Point pos = project_ui(core->rect, core->pos, core->scale, system->coord);

        size_t px = scale_div(core->scale, px_star);
        SDL_Rect src = { .x = 0, .y = 0, .w = 100, .h = 100 };
        SDL_Rect dst = {
            .x = pos.x - px / 2,
            .y = pos.y - px / 2,
            .h = px, .w = px
        };

        struct rgb rgb = spectrum_rgb(32 - bits_log2(system->star), 32);
        rgb = SDL_PointInRect(&core->cursor->pos, &dst) ?
            desaturate(rgb, .5) : desaturate(rgb, .8);

        SDL_SetTextureBlendMode(core->tex, SDL_BLENDMODE_ADD);
        SDL_SetTextureColorMod(core->tex, rgb.r, rgb.g, rgb.b);
        SDL_RenderCopy(renderer, core->tex, &src, &dst);
    }
}

void ui_core_render(struct ui_core *core, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &core->rect);

    core_render_galaxy(core, renderer);
    cursor_render(core->cursor, renderer);
    panel_render(core->p_coord, renderer);
}

void ui_core_events(struct ui_core *core, SDL_Event *event)
{
    cursor_events(core->cursor, event);
    panel_event(core->p_coord, event);

    switch (event->type)
    {

    case SDL_KEYUP: {

        switch (event->key.keysym.sym)
        {
        case SDLK_q:
            SDL_PushEvent(&(SDL_Event){.type = SDL_QUIT });
            break;
        }
        break;
    }

    case SDL_MOUSEWHEEL: {
        core->scale = scale_inc(core->scale, -event->wheel.y);
        break;
    }

    case SDL_MOUSEMOTION: {
        if (core->cursor->button == SDL_BUTTON_LEFT) {
            core->pos.x -= scale_mult(core->scale, event->motion.xrel);
            core->pos.y -= scale_mult(core->scale, event->motion.yrel);
        }
        break;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_MouseButtonEvent *b = &event->button;
        if (b->clicks == 1 && b->button == SDL_BUTTON_LEFT) {
            struct rect rect = project_coord_rect(
                    core->rect, core->pos, core->scale, (SDL_Rect) {
                        .x = b->x - px_star / 2,
                        .y = b->y - px_star / 2,
                        .w = px_star, .h = px_star,
                    });
            core->selected = sector_lookup(core->sector, &rect);
        }
    }

    }
}
