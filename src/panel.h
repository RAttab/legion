/* panel.h
   Rémi Attab (remi.attab@gmail.com), 11 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "utils.h"

struct panel;
typedef void (*render_fn) (void *state, SDL_Renderer *, SDL_Rect *);
typedef bool (*events_fn) (void *state, struct panel *, SDL_Event *);
typedef void (*free_fn) (void *state);

struct panel
{
    SDL_Rect rect;
    SDL_Rect inner_rect;
    SDL_Texture *tex;

    bool redraw;
    bool hidden;

    void *state;
    render_fn render;
    events_fn events;
    free_fn free;
};

struct panel *panel_new(const SDL_Rect *rect);
void panel_free(struct panel *);

void panel_invalidate(struct panel *);
void panel_hide(struct panel *);
void panel_show(struct panel *);

void panel_add_borders(int width, int height, int *dst_width, int *dst_height);

void panel_render(struct panel *, SDL_Renderer *);
bool panel_event(struct panel *, SDL_Event *);


// -----------------------------------------------------------------------------
// panels
// -----------------------------------------------------------------------------

struct panel *panel_pos_new();
struct panel *panel_system_new();