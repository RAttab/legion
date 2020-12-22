/* panel_pos.c
   Rémi Attab (remi.attab@gmail.com), 15 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/font.h"
#include "render/map.h"
#include "render/panel.h"
#include "game/coord.h"
#include "utils/log.h"
#include "SDL.h"


// -----------------------------------------------------------------------------
// panel_pos
// -----------------------------------------------------------------------------

struct panel_pos_state
{
    uint64_t time;
    scale_t scale;
    struct coord coord;

    struct SDL_Point time_pos;
    struct SDL_Point coord_pos;
    struct SDL_Point scale_pos;
};

static struct font *panel_pos_font(void) { return font_mono6; }


static void panel_pos_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_pos_state *state = state_;
    struct font *font = panel_pos_font();
    font_reset(font);

    {
        char str[8] = {0};
        str_utoa(core.state.time, str, sizeof(str));
        font_render(font, renderer, str, sizeof(str), (SDL_Point) {
                    .x = rect->x + state->time_pos.x,
                    .y = rect->y + state->time_pos.y });
    }

    {
        char str[coord_str_len+1] = {0};
        coord_str(state->coord, str, sizeof(str));
        font_render(font, renderer, str, coord_str_len, (SDL_Point) {
                    .x = rect->x + state->coord_pos.x,
                    .y = rect->y + state->coord_pos.y });
    }

    {
        char str[scale_str_len+1] = {0};
        scale_str(state->scale, str, sizeof(str));
        font_render(font, renderer, str, scale_str_len, (SDL_Point) {
                    .x = rect->x + state->scale_pos.x,
                    .y = rect->y + state->scale_pos.y });
    }
}

static bool panel_pos_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_pos_state *state = state_;

    if (event->type == core.event) {
        if (event->user.code == EV_STATE_UPDATE) panel_invalidate(panel);
        return false;
    }

    switch (event->type) {

    case SDL_MOUSEMOTION: {
        struct coord coord = map_project_coord(core.ui.map, core.cursor.point);
        if (!coord_eq(coord, state->coord)) {
            state->coord = coord;
            panel_invalidate(panel);
        }
        break;
    }

    case SDL_MOUSEWHEEL: {
        scale_t scale = map_scale(core.ui.map);
        if (scale != state->scale) {
            state->scale = scale;
            panel_invalidate(panel);
        }
        break;
    }
    }

    return false;
}

static void panel_pos_free(void *state)
{
    free(state);
};

struct panel *panel_pos_new(void)
{
    enum { spacing = 10 };
    struct font *font = panel_pos_font();

    size_t time_w = 0, time_h = 0;
    font_text_size(font, 8, &time_w, &time_h);

    size_t coord_w = 0, coord_h = 0;
    font_text_size(font, coord_str_len, &coord_w, &coord_h);

    size_t scale_w = 0, scale_h = 0;
    font_text_size(font, scale_str_len, &scale_w, &scale_h);

    size_t inner_w = time_w + spacing + coord_w + spacing + scale_w;
    size_t inner_h = i64_max(time_h, i64_max(coord_h, scale_h));

    int outer_w = 0, outer_h = 0;
    panel_add_borders(inner_w, inner_h, &outer_w, &outer_h);

    struct panel_pos_state *state = calloc(1, sizeof(*state));
    state->time = core.state.time;
    state->scale = map_scale(core.ui.map);
    state->coord = map_project_coord(core.ui.map, core.cursor.point);
    state->time_pos = (SDL_Point) { .x = 0, .y = 0 };
    state->coord_pos = (SDL_Point) { .x = time_w + spacing, .y = 0 };
    state->scale_pos = (SDL_Point) { .x = state->coord_pos.x + coord_w + spacing, .y = 0 };

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = (core.rect.w / 2)  - (outer_w / 2),
                .y = core.rect.h - outer_h,
                .w = outer_w, .h = outer_h });
    panel->hidden = false;
    panel->state = state;
    panel->render = panel_pos_render;
    panel->events = panel_pos_events;
    panel->free = panel_pos_free;
    return panel;
};
