/* panel.h
   Rémi Attab (remi.attab@gmail.com), 11 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

enum {
    panel_margin = 2,
    panel_border = 1,
    panel_padding = panel_border + panel_margin,
    panel_total_padding = panel_padding * 2,
};


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
SDL_Point panel_relative_point(struct panel *, const SDL_Point *);

void panel_render(struct panel *, SDL_Renderer *);
bool panel_event(struct panel *, SDL_Event *);


// -----------------------------------------------------------------------------
// panels
// -----------------------------------------------------------------------------

struct panel *panel_menu_new(void);
size_t panel_menu_height(void);

struct panel *panel_mods_new(void);
size_t panel_mods_width(void);

struct panel *panel_code_new(void);
struct panel *panel_pos_new(void);

struct panel *panel_star_new(void);
size_t panel_star_width(void);

struct panel *panel_obj_new(void);
