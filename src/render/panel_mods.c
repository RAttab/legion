/* panel_mods.c
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "panel.h"
#include "render/ui.h"
#include "render/font.h"
#include "vm/mod.h"

// -----------------------------------------------------------------------------
// panel mods
// -----------------------------------------------------------------------------

struct panel_mods_item
{
    mod_t id;
    atom_t str;
    struct ui_toggle ui;
};

struct panel_mods_state
{
    int toggle_h;
    struct SDL_Rect rect;

    struct mods *mods;
    struct ui_toggle *toggles;
};

static struct font *panel_mods_font(void) { return font_mono8; }

size_t panel_mods_width(void)
{
    int width = 0;
    struct font *font = panel_mods_font();
    ui_toggle_size(font, vm_atom_cap, &width, NULL);
    return width;
}

static void panel_mods_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_mods_state *state = state_;
    struct font *font = panel_mods_font();

    SDL_Point pos = { .x = rect->x, .y = rect->y };
    for (size_t i = 0; i < state->mods->len; ++i) {
        ui_toggle_render(&state->toggles[i], renderer, pos, font);
        pos.y += state->toggle_h;
    }
}

static void panel_mods_update(struct panel_mods_state *state)
{
    free(state->mods);
    state->mods = mods_list();

    free(state->toggles);
    state->toggles = calloc(state->mods->len, sizeof(*state->toggles));

    struct SDL_Rect rect = state->rect;
    rect.h = state->toggle_h;

    for (size_t i = 0; i < state->mods->len; ++i) {
        struct mods_item *item = &state->mods->items[i];
        ui_toggle_init(&state->toggles[i], &rect, item->str, vm_atom_cap);
        rect.y += state->toggle_h;
    }
}

static bool panel_mods_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_mods_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {
        case EV_MODS_SELECT: // FALLTHROUGH
        case EV_MODS_UPDATE: {
            panel_mods_update(state);
            panel_show(panel);
            return true;
        }
        case EV_MODS_CLEAR: { panel_hide(panel); return true; }

        case EV_CODE_CLEAR: {
            for (size_t i = 0; i < state->mods->len; ++i)
                state->toggles[i].selected = false;
            panel_invalidate(panel);
            return false;
        }

        default: { return false; }
        }
    }

    if (panel->hidden) return false;

    for (size_t i = 0; i < state->mods->len; ++i) {
        enum ui_toggle_ret ret = ui_toggle_events(&state->toggles[i], event);
        if (ret & ui_toggle_invalidate) panel_invalidate(panel);
        if (ret & ui_toggle_flip) {
            enum event ev = state->toggles[i].selected ? EV_CODE_SELECT : EV_CODE_CLEAR;
            uint64_t data = state->mods->items[i].id;
            core_push_event(ev, (void *) data);

            for (size_t j = 0; j < state->mods->len; ++j) {
                if (j != i) state->toggles[j].selected = false;
            }
        }
        if (ret & ui_toggle_consume) return true;
    }
    return false;
}

static void panel_mods_free(void *state_)
{
    struct panel_mods_state *state = state_;
    free(state->toggles);
    free(state->mods);
    free(state);
};

struct panel *panel_mods_new(void)
{
    size_t menu_h = panel_menu_height();

    struct font *font = panel_mods_font();
    size_t inner_w = panel_mods_width();
    size_t inner_h = core.rect.h - menu_h - panel_total_padding;

    int outer_w = 0, outer_h = 0;
    panel_add_borders(inner_w, inner_h, &outer_w, &outer_h);

    struct panel_mods_state *state = calloc(1, sizeof(*state));
    state->rect = (SDL_Rect) {
        .x = 0, .y = menu_h + panel_padding,
        .w = inner_w, .h = inner_h };
    ui_toggle_size(font, vm_atom_cap, NULL, &state->toggle_h);

    SDL_Rect rect = { .x = 0, .y = menu_h, .w = outer_w, .h = outer_h };
    struct panel *panel = panel_new(&rect);
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_mods_render;
    panel->events = panel_mods_events;
    panel->free = panel_mods_free;

    return panel;
}
