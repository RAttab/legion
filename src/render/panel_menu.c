/* panel_menu.c
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "panel.h"

#include "utils/sdl.h"
#include "utils/text.h"

// -----------------------------------------------------------------------------
// panel menu
// -----------------------------------------------------------------------------

struct panel_menu_state
{
    size_t font_w, font_h;

    struct {
        SDL_Rect rect;
        bool hover;
        bool selected;
    } mods;
};

static void panel_menu_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_menu_state *state = state_;
    font_reset(font_mono6);

    if (state->mods.hover) {
        sdl_err(SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, SDL_ALPHA_OPAQUE));
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = rect->x, .y = rect->y,
                            .w = state->mods.rect.w, .h = state->mods.rect.h}));
    }

    const char *prefix = state->mods.selected ? "- modules" : "+ modules";
    font_render(font_mono6, renderer, prefix, 9, (SDL_Point) { rect->x, rect->y });
}

static bool panel_menu_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_menu_state *state = state_;

    switch (event->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = panel_relative_point(panel, &core.cursor.point);
        if (!sdl_rect_contains(&state->mods.rect, &point)) {
            if (state->mods.hover) {
                state->mods.hover = false;
                panel_invalidate(panel);
            }
            break;
        }

        if (!state->mods.hover) {
            state->mods.hover = true;
            panel_invalidate(panel);
        }
        break;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = panel_relative_point(panel, &core.cursor.point);
        if (!sdl_rect_contains(&state->mods.rect, &point)) break;

        state->mods.selected = !state->mods.selected;
        core_push_event(state->mods.selected ? EV_MODS_SELECT : EV_MODS_CLEAR, NULL);
        panel_invalidate(panel);
        return true;
    }

    }
    return false;
}

static void panel_menu_free(void *state)
{
    free(state);
};


struct panel *panel_menu_new(void)
{
    size_t font_w = 0, font_h = 0;
    font_text_size(font_mono6, 1, &font_w, &font_h);

    int outer_w = 0, outer_h = 0;
    panel_add_borders(font_w, font_h, &outer_w, &outer_h);

    struct panel_menu_state *state = calloc(1, sizeof(*state));
    state->font_w = font_w;
    state->font_h = font_h;
    state->mods.rect = (SDL_Rect) {
        .x = 0, .y = 0, .w = font_w * 10, .h = font_h };

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = 0, .y = 0, .w = core.rect.w, .h = outer_h });

    panel->hidden = false;
    panel->state = state;
    panel->render = panel_menu_render;
    panel->events = panel_menu_events;
    panel->free = panel_menu_free;

    return panel;
}
