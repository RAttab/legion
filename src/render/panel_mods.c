/* pmods.c
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

struct pmods_state
{
    struct layout *layout;
    struct ui_scroll scroll;

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

    p_mods_list_len = vm_atom_cap + ui_toggle_layout_cols,
    p_mods_list_total_len = p_mods_list_len + ui_scroll_layout_cols,
};

static const char p_mods_str[] = "mods:";

static void pmods_render_count(
        struct pmods_state *state, SDL_Renderer *renderer)
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

static void pmods_render_list(
        struct pmods_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_mods_list);

    const size_t first = state->scroll.first;
    const size_t rows = u64_min(state->mods->len, state->scroll.visible);

    for (size_t i = first; i < first + rows; ++i) {
        SDL_Point pos = layout_entry_index_pos(layout, i - first, 0);
        ui_toggle_render(&state->toggles[i], renderer, pos, layout->font);
    }

    ui_scroll_render(&state->scroll, renderer,
            layout_entry_index_pos(layout, 0, p_mods_list_len));
}

static void pmods_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct pmods_state *state = state_;
    (void) rect;

    pmods_render_count(state, renderer);
    pmods_render_list(state, renderer);
}

static void pmods_update(struct pmods_state *state)
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

    ui_scroll_update(&state->scroll, state->mods->len);
}

static bool pmods_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct pmods_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {
        case EV_MODS_SELECT: {
            pmods_update(state);
            panel_show(panel);
            return true;
        }

        case EV_STATE_UPDATE: {
            if (panel->hidden) return false;
            pmods_update(state);
            panel_invalidate(panel);
            return false;
        }

        case EV_MODS_CLEAR: { panel_hide(panel); return true; }

        case EV_CODE_CLEAR: {
            if (panel->hidden) return false;
            state->selected = 0;
            for (size_t i = 0; i < state->mods->len; ++i)
                state->toggles[i].selected = false;
            panel_invalidate(panel);
            return false;
        }

        default: { return false; }
        }
    }

    if (panel->hidden) return false;

    {
        enum ui_ret ret = ui_scroll_events(&state->scroll, event);
        if (ret & ui_invalidate) panel_invalidate(panel);
        if (ret & ui_consume) return true;
    }

    for (size_t i = 0; i < state->mods->len; ++i) {
        struct ui_toggle *toggle = &state->toggles[i];
        enum ui_ret ret = ui_toggle_events(toggle, event);

        if (ret & ui_invalidate) panel_invalidate(panel);
        if (ret & ui_action) {
            mod_t mod = state->mods->items[i].id;

            enum event ev = toggle->selected ? EV_CODE_SELECT : EV_CODE_CLEAR;
            core_push_event(ev, mod, 0);

            state->selected = toggle->selected ? mod : 0;
            for (size_t j = 0; j < state->mods->len; ++j) {
                if (j != i) state->toggles[j].selected = false;
            }
        }
        if (ret & ui_consume) return true;
    }
    return false;
}

static void pmods_free(void *state_)
{
    struct pmods_state *state = state_;
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
    layout_text(layout, p_mods_list, font, p_mods_list_total_len, layout_inf);

    layout_finish(layout, (SDL_Point) { .x = panel_padding, .y = panel_padding });
    layout->pos = (SDL_Point) { .x = 0, .y = menu_h };

    struct pmods_state *state = calloc(1, sizeof(*state));
    state->layout = layout;

    {
        SDL_Rect events = layout_abs(layout, p_mods_list);
        SDL_Rect bar = layout_abs_index(layout, p_mods_list, layout_inf, p_mods_list_len);
        ui_scroll_init(&state->scroll, &bar, &events, 0,
                layout_entry(layout, p_mods_list)->rows);
    }

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = layout->pos.x, .y = layout->pos.y,
                .w = layout->bbox.w + panel_total_padding,
                .h = layout->bbox.h + panel_total_padding });
    panel->hidden = true;
    panel->state = state;
    panel->render = pmods_render;
    panel->events = pmods_events;
    panel->free = pmods_free;
    return panel;
}
