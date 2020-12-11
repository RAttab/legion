/* panel_menu.c
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "panel.h"
#include "render/ui.h"
#include "render/font.h"


// -----------------------------------------------------------------------------
// panel menu
// -----------------------------------------------------------------------------

struct panel_menu_state
{
    struct ui_toggle mods;
    struct ui_toggle code;
};

static struct font *panel_menu_font(void) { return font_mono8; }

static void panel_menu_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_menu_state *state = state_;
    struct font *font = panel_menu_font();

    SDL_Point pos = { .x = rect->x, .y = rect->y };
    ui_toggle_render(&state->mods, renderer, pos, font);

    pos.x += panel_mods_width();
    ui_toggle_render(&state->code, renderer, pos, font);
}

static bool panel_menu_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_menu_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {
        case EV_CODE_SELECT: {
            state->code.disabled = false;
            state->code.selected = true;
            panel_invalidate(panel);
            break;
        }
        case EV_CODE_CLEAR: {
            state->code.disabled = true;
            state->code.selected = false;
            panel_invalidate(panel);
            break;
        }
        default: { return false; }
        }
    }

    {
        enum ui_toggle_ret ret = ui_toggle_events(&state->mods, event);
        if (ret & ui_toggle_invalidate) panel_invalidate(panel);
        if (ret & ui_toggle_flip) {
            enum event ev = state->mods.selected ? EV_MODS_SELECT : EV_MODS_CLEAR;
            core_push_event(ev, NULL);
        }
        if (ret & ui_toggle_consume) return true;
    }

    {
        enum ui_toggle_ret ret = ui_toggle_events(&state->code, event);
        if (ret & ui_toggle_invalidate) panel_invalidate(panel);
        if (ret & ui_toggle_flip) {
            assert(!state->code.selected);
            state->code.disabled = true;
            core_push_event(EV_CODE_CLEAR, NULL);
        }
        if (ret & ui_toggle_consume) return true;
    }

    return false;
}

static void panel_menu_free(void *state)
{
    free(state);
};


size_t panel_menu_height(void)
{
    return panel_menu_font()->glyph_h + panel_total_padding;
}

struct panel *panel_menu_new(void)
{
    struct font *font = panel_menu_font();
    size_t font_w = font->glyph_w, font_h = font->glyph_h;

    int outer_w = 0, outer_h = 0;
    panel_add_borders(font_w, font_h, &outer_w, &outer_h);

    struct panel_menu_state *state = calloc(1, sizeof(*state));

    {
        const char str[] = "mods";
        SDL_Rect rect = {.x = panel_padding, .y = panel_padding };
        ui_toggle_size(font, sizeof(str), &rect.w, &rect.h);
        ui_toggle_init(&state->mods, &rect, str, sizeof(str));
    }

    {
        const char str[] = "code";
        SDL_Rect rect = {.x = panel_mods_width(), .y = panel_padding };
        ui_toggle_size(font, sizeof(str), &rect.w, &rect.h);
        ui_toggle_init(&state->code, &rect, str, sizeof(str));
        state->code.disabled = true;
    }

    SDL_Rect rect = { .x = 0, .y = 0, .w = core.rect.w, .h = outer_h };
    struct panel *panel = panel_new(&rect);
    panel->hidden = false;
    panel->state = state;
    panel->render = panel_menu_render;
    panel->events = panel_menu_events;
    panel->free = panel_menu_free;

    return panel;
}
