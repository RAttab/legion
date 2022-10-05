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
    struct ui_button new;
    struct ui_input new_val;
    struct ui_list mods;
};

struct ui_mods *ui_mods_new(void)
{
    struct pos pos = make_pos(0, ui_topbar_height());
    struct dim dim = make_dim(
            (symbol_cap+5) * ui_st.font.dim.w,
            render.rect.h - pos.y - ui_status_height());

    struct ui_mods *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_mods) {
        .panel = ui_panel_title(pos, dim, ui_str_v(12)),
        .new = ui_button_new(ui_str_c("+")),
        .new_val = ui_input_new(symbol_cap),
        .mods = ui_list_new(make_dim(ui_layout_inf, ui_layout_inf), symbol_cap),
    };

    ui_panel_hide(&ui->panel);
    return ui;
}


void ui_mods_free(struct ui_mods *ui) {
    ui_panel_free(&ui->panel);
    ui_button_free(&ui->new);
    ui_input_free(&ui->new_val);
    ui_list_free(&ui->mods);
    free(ui);
}

int16_t ui_mods_width(const struct ui_mods *ui)
{
    return ui->panel.w.dim.w;
}

static void ui_mods_update(struct ui_mods *ui)
{
    ui_list_reset(&ui->mods);

    const struct mods_list *list = proxy_mods(render.proxy);
    for (size_t i = 0; i < list->len; ++i) {
        const struct mods_item *mod = list->items + i;
        ui_str_set_symbol(ui_list_add(&ui->mods, mod->maj), &mod->str);
    }

    ui_str_setf(&ui->panel.title.str, "mods (%u)", list->len);
}

static bool ui_mods_event_user(struct ui_mods *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_panel_hide(&ui->panel);
        ui_list_clear(&ui->mods);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        ui_mods_update(ui);
        return false;
    }

    case EV_MODS_TOGGLE: {
        if (ui_panel_is_visible(&ui->panel)) {
            ui_panel_hide(&ui->panel);
            ui_list_clear(&ui->mods);
        }
        else {
            ui_mods_update(ui);
            ui_panel_show(&ui->panel);
        }
        return false;
    }

    case EV_MOD_SELECT: {
        mod_id mod = (uintptr_t) ev->user.data1;
        ui_list_select(&ui->mods, mod_major(mod));

        if (!ui_panel_is_visible(&ui->panel)) {
            ui_panel_show(&ui->panel);
            ui_mods_update(ui);
        }

        return false;
    }

    case EV_MOD_CLEAR: {
        ui_list_clear(&ui->mods);
        return false;
    }

    case EV_TAPES_TOGGLE:
    case EV_TAPE_SELECT:
    case EV_STARS_TOGGLE:
    case EV_LOG_TOGGLE:
    case EV_LOG_SELECT: {
        ui_panel_hide(&ui->panel);
        ui_list_clear(&ui->mods);
        return false;
    }

    default: { return false; }
    }
}

static void ui_mods_event_new(struct ui_mods *ui)
{
    struct symbol name = {0};
    if (!ui_input_get_symbol(&ui->new_val, &name)) {
        render_log(st_error, "Invalid module name: '%s'", name.c);
        return;
    }

    proxy_mod_register(render.proxy, name);

    // \todo due to async nature, opening the mod right away is tricky. Defered
    // to later.
}

bool ui_mods_event(struct ui_mods *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_mods_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret != ui_skip;

    if ((ret = ui_input_event(&ui->new_val, ev))) {
        if (ret != ui_action) ui_mods_event_new(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->new, ev))) {
        if (ret != ui_action) return true;
        ui_mods_event_new(ui);
        return ret;
    }

    if ((ret = ui_list_event(&ui->mods, ev))) {
        if (ret != ui_action) return true;
        render_push_event(EV_MOD_SELECT, make_mod(ui->mods.selected, 0), 0);
        return true;
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_mods_render(struct ui_mods *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_input_render(&ui->new_val, &layout, renderer);
    ui_button_render(&ui->new, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_layout_sep_row(&layout);

    ui_list_render(&ui->mods, &layout, renderer);
}
