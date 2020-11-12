/* panel.c
   Rémi Attab (remi.attab@gmail.com), 11 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "panel.h"

#include "coord.h"
#include "ui.h"

// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

enum { margin = 4, border = 2 };

struct panel *panel_new(SDL_Renderer *renderer, SDL_Rect rect)
{
    struct panel *panel = calloc(1, sizeof(*panel));
    panel->rect = rect;
    panel->redraw = true;
    panel->hidden = true;

    panel->tex = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            rect.w, rect.h);

    return panel;
}

void panel_free(struct panel *panel)
{
    SDL_DestroyTexture(panel->tex);
    free(panel);
}

void panel_invalidate(struct panel *panel)
{
    panel->redraw = true;
}

void panel_hide(struct panel *panel)
{
    panel->hidden = true;
}

void panel_show(struct panel *panel)
{
    panel->hidden = false;
    panel->redraw = true;
}

void panel_update(struct panel *panel, int type, void *data)
{
    if (panel->update)
        panel->update(panel->state, panel, type, data);
}

void panel_event(struct panel *panel, SDL_Event *event)
{
    if (panel->events)
        panel->events(panel->state, panel, event);
}

void panel_render(struct panel *panel, SDL_Renderer *renderer)
{
    if (panel->hidden) return;
    if (panel->redraw) {
        panel->redraw = false;

        SDL_SetRenderTarget(renderer, panel->tex);

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &panel->rect);

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xCC);
        SDL_RenderFillRect(renderer, &(SDL_Rect) {
                    .x = border, .y = border,
                    .w = panel->rect.w - border * 2,
                    .h = panel->rect.h - border * 2});

        if (panel->render) {
            panel->render(panel->state, renderer, &(SDL_Rect) {
                        .x = margin, .y = margin,
                        .w = panel->rect.w - margin * 2,
                        .h = panel->rect.h - margin * 2});
        }

        SDL_SetRenderTarget(renderer, NULL);
    }

    SDL_Rect src = { .x = 0, .y = 0, .w = panel->rect.w, .h = panel->rect.h };
    SDL_RenderCopy(renderer, panel->tex, &src, &panel->rect);
}

// -----------------------------------------------------------------------------
// coord
// -----------------------------------------------------------------------------

struct panel_coord_state
{
    struct ui_core *core;
    struct coord coord;
};

static void panel_coord_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_coord_state *state = state_;

    uint16_t x = state->coord.x & (coord_system_max - 1);
    uint16_t y = state->coord.y & (coord_system_max - 1);
    
    SDL_SetRenderDrawColor(renderer, x & 0xFF, x >> 8, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &(SDL_Rect) {
                .x = 0, .y = 0, .w = rect->w / 2, .h = rect->h});

    SDL_SetRenderDrawColor(renderer, y & 0xFF, y >> 8, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &(SDL_Rect) {
                .x = rect->w / 2, .y = 0, .w = rect->w / 2, .h = rect->h});
}

static void panel_coord_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_coord_state *state = state_;
    struct ui_core *core = state->core;

    if (event->type != SDL_MOUSEMOTION) return;

    struct coord coord = project_coord(core->rect, core->pos, core->scale, (SDL_Point) {
                .x = event->motion.x, .y = event->motion.y});
    if (coord_eq(coord, state->coord)) return;

    state->coord = coord;
    panel_invalidate(panel);
}

struct panel *panel_coord_new(SDL_Renderer *renderer, struct ui_core *core)
{
    size_t padding = (margin + border) * 2;
    size_t w = (12 * 2 + 1) * 10 + padding;
    size_t h = 16 + padding;
    SDL_Rect rect = {
        .x = core->rect.w - w,
        .y = core->rect.h - h,
        .w = w, .h = h,
    };

    struct panel_coord_state *state = calloc(1, sizeof(*state));
    state->core = core;

    struct panel *panel = panel_new(renderer, rect);
    panel->hidden = false;
    panel->state = state;
    panel->render = panel_coord_render;
    panel->events = panel_coord_events;

    return panel;
};
