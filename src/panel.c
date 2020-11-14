/* panel.c
   Rémi Attab (remi.attab@gmail.com), 11 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "panel.h"

#include "coord.h"
#include "ui.h"
#include "font.h"

// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

enum {
    margin = 2,
    border = 1,
    panel_padding = (margin + border) * 2
};

void panel_add_borders(int width, int height, int *dst_width, int *dst_height)
{
    *dst_width = width + panel_padding * 2;
    *dst_height = height + panel_padding * 2;
}

struct panel *panel_new(SDL_Renderer *renderer, const SDL_Rect *rect)
{
    struct panel *panel = calloc(1, sizeof(*panel));
    panel->redraw = true;
    panel->hidden = true;
    panel->rect = *rect;
    panel->inner_rect = (SDL_Rect) {
        .x = panel_padding, .y = panel_padding,
        .w = rect->w - (panel_padding * 2), .h = rect->h - (panel_padding * 2),
    };

    panel->tex = sdl_ptr(SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            panel->rect.w, panel->rect.h));

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

        sdl_err(SDL_SetRenderTarget(renderer, panel->tex));

        sdl_err(SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE));
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = 0, .y = 0,
                            .w = panel->rect.w,
                            .h = panel->rect.h }));

        sdl_err(SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xCC));
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = border, .y = border,
                            .w = panel->rect.w - border * 2,
                            .h = panel->rect.h - border * 2}));

        if (panel->render) {
            panel->render(panel->state, renderer, &panel->inner_rect);
        }

        sdl_err(SDL_SetRenderTarget(renderer, NULL));
    }

    sdl_err(SDL_RenderCopy(renderer, panel->tex, NULL, &panel->rect));
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

    char str[coord_str_len+1] = {0};
    coord_str(state->coord, str, sizeof(str));

    SDL_Point pos = { .x = rect->x, .y = rect->y };
    font_render(font_mono6, renderer, str, coord_str_len, pos);
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
    size_t inner_w = 0, inner_h = 0;
    font_text_size(font_mono6, coord_str_len, &inner_w, &inner_h);

    int outer_w = 0, outer_h = 0;
    panel_add_borders(inner_w, inner_h, &outer_w, &outer_h);

    struct panel_coord_state *state = calloc(1, sizeof(*state));
    state->core = core;

    struct panel *panel = panel_new(renderer, &(SDL_Rect) {
                .x = core->rect.w - outer_w, .y = 0,
                .w = outer_w, .h = outer_h });
    panel->hidden = false;
    panel->state = state;
    panel->render = panel_coord_render;
    panel->events = panel_coord_events;

    return panel;
};
