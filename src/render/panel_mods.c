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

struct panel_mods_state
{
    struct layout *layout;

    mod_t selected;
    struct mods *mods;
    struct ui_toggle *toggles;
};

enum
{
    p_mods_count = 0,
    p_mods_count_sep,
    p_mods_list,
    p_mods_len,
};

enum
{
    p_mods_val_len = 8,
};

static const char p_mods_str[] = "mods:";

static void panel_mods_render_count(
        struct panel_mods_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_mods_count);
    SDL_Point pos = layout_entry_pos(layout);

    font_render(layout->font, renderer, p_mods_str, sizeof(p_mods_str),
            layout_entry_pos(layout));

    char val[p_mods_val_len];
    str_utoa(state->mods->len, val, sizeof(val));
    pos.x = state->layout->bbox.w - (layout->item.w * sizeof(val));
    font_render(layout->font, renderer, val, sizeof(val), pos);
}

static void panel_mods_render_list(
        struct panel_mods_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_mods_list);

    size_t rows = u64_min(state->mods->len, layout->rows);
    for (size_t i = 0; i < rows; ++i) {
        SDL_Point pos = layout_entry_index_pos(layout, i, 0);
        ui_toggle_render(&state->toggles[i], renderer, pos, layout->font);
    }
}

static void panel_mods_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_mods_state *state = state_;
    (void) rect;

    panel_mods_render_count(state, renderer);
    panel_mods_render_list(state, renderer);
}

static void panel_mods_update(struct panel_mods_state *state)
{
    free(state->mods);
    state->mods = mods_list();

    free(state->toggles);
    state->toggles = calloc(state->mods->len, sizeof(*state->toggles));

    struct layout_entry *layout = layout_entry(state->layout, p_mods_list);
    struct SDL_Rect rect = layout_abs(state->layout, p_mods_list);
    rect.h = layout->item.h;

    for (size_t i = 0; i < state->mods->len; ++i) {
        struct mods_item *item = &state->mods->items[i];
        struct ui_toggle *toggle = &state->toggles[i];

        ui_toggle_init(toggle, &rect, item->str, vm_atom_cap);
        toggle->selected = state->mods->items[i].id == state->selected;

        rect.y += layout->item.h;
    }
}

static bool panel_mods_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_mods_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {
        case EV_MODS_SELECT: {
            panel_mods_update(state);
            panel_show(panel);
            return true;
        }

        case EV_STATE_UPDATE: {
            if (panel->hidden) return false;
            panel_mods_update(state);
            panel_invalidate(panel);
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
        struct ui_toggle *toggle = &state->toggles[i];
        enum ui_toggle_ret ret = ui_toggle_events(toggle, event);

        if (ret & ui_toggle_invalidate) panel_invalidate(panel);
        if (ret & ui_toggle_flip) {
            mod_t mod = state->mods->items[i].id;

            enum event ev = toggle->selected ? EV_CODE_SELECT : EV_CODE_CLEAR;
            core_push_event(ev, mod, 0);

            state->selected = toggle->selected ? mod : 0;
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
    layout_free(state->layout);
    free(state->toggles);
    free(state->mods);
    free(state);
};

struct panel *panel_mods_new(void)
{
    struct font *font = font_mono8;
    size_t menu_h = panel_menu_height();

    struct layout *layout = layout_alloc(p_mods_len,
            core.rect.w, core.rect.h - menu_h - panel_total_padding);

    layout_text(layout, p_mods_count, font, sizeof(p_mods_str) + p_mods_val_len, 1);
    layout_sep(layout, p_mods_count_sep);
    layout_text(layout, p_mods_list, font, vm_atom_cap+2, layout_inf);

    layout_finish(layout, (SDL_Point) { .x = panel_padding, .y = panel_padding });
    layout->pos = (SDL_Point) { .x = panel_padding, .y = menu_h + panel_padding };

    struct panel_mods_state *state = calloc(1, sizeof(*state));
    state->layout = layout;

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = layout->pos.x - panel_padding,
                .y = layout->pos.y - panel_padding,
                .w = layout->bbox.w + panel_total_padding,
                .h = core.rect.h - menu_h });
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_mods_render;
    panel->events = panel_mods_events;
    panel->free = panel_mods_free;

    return panel;
}
