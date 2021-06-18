/* ui_mods.c
   Rémi Attab (remi.attab@gmail.com), 16 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"


// -----------------------------------------------------------------------------
// mods
// -----------------------------------------------------------------------------

struct ui_mods
{
    struct panel *panel;
    struct scroll *scroll;

    struct mods *list;
    struct toggle **toggles;
};

static struct font *ui_mods_font(void) { return font_mono6; }

struct ui_mods *ui_mods_new(void)
{
    struct pos pos = make_pos(0, ui_topbar_height(core.ui.topbar));
    struct dim dim = make_dim(
            (vm_atom_cap+2) * ui_mods_font()->glyph_w,
            core.rect.h - pos.y);

    struct ui_mods *mods = calloc(1, sizeof(*mods));
    *mods = (struct ui_mods) {
        .panel = panel_var(pos, dim, 12),
        .scroll = scroll_new(make_dim(layout_inf, layout_inf), 0, 1),
        .list = NULL,
        .toggles = NULL,
    };

    mods->panel->state = panel_hidden;
    return mods;
}


void ui_mods_free(struct ui_mods *mods) {
    panel_free(mods->panel);
    scroll_free(mods->scroll);

    for (size_t i = 0; i < mods->list->len; ++i)
        toggle_free(mods->toggles[i]);

    free(mods->toggles);
    free(mods->list);
}

int16_t ui_mods_width(const struct ui_mods *mods)
{
    return mods->panel->w.dim.w;
}

static void ui_mods_update(struct ui_mods *mods)
{
    struct mods *list = mods_list();

    if (!mods->list || mods->list->len < list->len) {
        mods->toggles = realloc(mods->toggles, sizeof(*mods->toggles) * list->len);
        for (size_t i = mods->list ? mods->list->len : 0;  i < list->len; ++i)
            mods->toggles[i] = toggle_var(ui_mods_font(), vm_atom_cap);
    }
    else if (mods->list && mods->list->len > list->len) {
        for (size_t i = list->len; i < mods->list->len; ++i)
            toggle_free(mods->toggles[i]);
        mods->toggles = realloc(mods->toggles, sizeof(*mods->toggles) * list->len);
    }

    free(mods->list);
    mods->list = list;
    scroll_update(mods->scroll, mods->list->len);

    for (size_t i = 0; i < mods->list->len; ++i)
        toggle_set(mods->toggles[i], mods->list->items[i].str, vm_atom_cap);

    label_setf(mods->panel->title, "mods(%zu)", mods->list->len);
}

static bool ui_mods_event_user(struct ui_mods *mods, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_MODS_TOGGLE: {
        if (mods->panel->state == panel_hidden) {
            ui_mods_update(mods);
            mods->panel->state = panel_visible;
            core_push_event(EV_FOCUS, (uintptr_t) mods->panel, 0);
        }
        else {
            if (mods->panel->state == panel_focused)
                core_push_event(EV_FOCUS, 0, 0);
            mods->panel->state = panel_hidden;
        }
        return true;
    }

    case EV_MOD_SELECT: {
        mod_t mod = (uintptr_t) ev->user.data1;
        for (size_t i = 0; i < mods->list->len; ++i) {
            mods->toggles[i]->state = mods->list->items[i].id == mod ?
                toggle_selected : toggle_idle;
        }
        return false;
    }

    case EV_MOD_CLEAR: {
        for (size_t i = 0; i < mods->list->len; ++i)
            mods->toggles[i]->state = toggle_idle;
        return false;
    }

    default: { return false; }
    }
}

bool ui_mods_event(struct ui_mods *mods, SDL_Event *ev)
{
    if (ev->type == core.event && ui_mods_event_user(mods, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = panel_event(mods->panel, ev))) return ret == ui_consume;
    if ((ret = scroll_event(mods->scroll, ev))) return ret == ui_consume;

    for (size_t i = 0; i < mods->list->len; ++i) {
        struct toggle *toggle = mods->toggles[i];
        if ((ret = toggle_event(toggle, ev))) {
            enum event type = toggle->state == toggle_selected ? EV_MOD_SELECT : EV_MOD_CLEAR;
            core_push_event(type, mods->list->items[i].id, 0);
            return true;
        }
    }

    return false;
}

void ui_mods_render(struct ui_mods *mods, SDL_Renderer *renderer)
{
    struct layout outer = panel_render(mods->panel, renderer);
    if (layout_is_nil(&outer)) return;

    struct layout inner = scroll_render(mods->scroll, &outer, renderer);
    if (layout_is_nil(&inner)) return;

    mods->scroll->visible = inner.dim.h / ui_mods_font()->glyph_h;

    for (size_t i = scroll_first(mods->scroll); i < scroll_last(mods->scroll); ++i) {
        toggle_render(mods->toggles[i], &inner, renderer);
        layout_next_row(&inner);
    }
}
