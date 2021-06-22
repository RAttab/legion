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
    struct ui_panel panel;
    struct ui_scroll scroll;
    struct ui_toggles toggles;
};

static struct font *ui_mods_font(void) { return font_mono6; }

struct ui_mods *ui_mods_new(void)
{
    struct font *font = ui_mods_font();
    struct pos pos = make_pos(0, ui_topbar_height(core.ui.topbar));
    struct dim dim = make_dim(
            (vm_atom_cap+2) * font->glyph_w,
            core.rect.h - pos.y);

    struct ui_mods *mods = calloc(1, sizeof(*mods));
    *mods = (struct ui_mods) {
        .panel = ui_panel_title(pos, dim, ui_str_v(12)),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .toggles = ui_toggles_new(font, ui_str_v(vm_atom_cap)),
    };

    mods->panel.state = ui_panel_hidden;
    return mods;
}


void ui_mods_free(struct ui_mods *mods) {
    ui_panel_free(&mods->panel);
    ui_scroll_free(&mods->scroll);
    ui_toggles_free(&mods->toggles);
    free(mods);
}

int16_t ui_mods_width(const struct ui_mods *mods)
{
    return mods->panel.w.dim.w;
}

static void ui_mods_update(struct ui_mods *mods)
{
    struct mods *list = mods_list();
    ui_toggles_resize(&mods->toggles, list->len);
    ui_scroll_update(&mods->scroll, mods->toggles.len);

    for (size_t i = 0; i < list->len; ++i) {
        struct ui_toggle *toggle = &mods->toggles.items[i];
        ui_str_setv(&toggle->str, list->items[i].str, vm_atom_cap);
        toggle->user = list->items[i].id;
    }

    ui_str_setf(&mods->panel.title.str, "mods(%zu)", list->len);
    free(list);
}

static bool ui_mods_event_user(struct ui_mods *mods, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_MODS_TOGGLE: {
        if (mods->panel.state == ui_panel_hidden) {
            ui_mods_update(mods);
            mods->panel.state = ui_panel_visible;
            core_push_event(EV_FOCUS, (uintptr_t) &mods->panel, 0);
        }
        else {
            if (mods->panel.state == ui_panel_focused)
                core_push_event(EV_FOCUS, 0, 0);
            mods->panel.state = ui_panel_hidden;
        }
        return true;
    }

    case EV_MOD_SELECT: {
        mod_t mod = (uintptr_t) ev->user.data1;
        for (size_t i = 0; i < mods->toggles.len; ++i) {
            struct ui_toggle *toggle = &mods->toggles.items[i];
            toggle->state = toggle->user == mod ? ui_toggle_selected : ui_toggle_idle;
        }
        return false;
    }

    case EV_MOD_CLEAR: {
        for (size_t i = 0; i < mods->toggles.len; ++i)
            mods->toggles.items[i].state = ui_toggle_idle;
        return false;
    }

    default: { return false; }
    }
}

bool ui_mods_event(struct ui_mods *mods, SDL_Event *ev)
{
    if (ev->type == core.event && ui_mods_event_user(mods, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&mods->panel, ev))) return ret == ui_consume;
    if ((ret = ui_scroll_event(&mods->scroll, ev))) return ret == ui_consume;

    struct ui_toggle *toggle = NULL;
    if ((ret = ui_toggles_event(&mods->toggles, ev, &mods->scroll, &toggle, NULL))) {
        enum event type = toggle->state == ui_toggle_selected ? EV_MOD_SELECT : EV_MOD_CLEAR;
        core_push_event(type, toggle->user, 0);
        return true;
    }

    return false;
}

void ui_mods_render(struct ui_mods *mods, SDL_Renderer *renderer)
{
    struct ui_layout outer = ui_panel_render(&mods->panel, renderer);
    if (ui_layout_is_nil(&outer)) return;

    struct ui_layout inner = ui_scroll_render(&mods->scroll, &outer, renderer);
    if (ui_layout_is_nil(&inner)) return;

    ui_toggles_render(&mods->toggles, &inner, renderer, &mods->scroll);
}
