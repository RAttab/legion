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
    struct ui_scroll scroll;
    struct ui_toggles toggles;
};

static struct font *ui_mods_font(void) { return font_mono6; }

struct ui_mods *ui_mods_new(void)
{
    struct font *font = ui_mods_font();
    struct pos pos = make_pos(0, ui_topbar_height());
    struct dim dim = make_dim(
            (symbol_cap+5) * font->glyph_w,
            core.rect.h - pos.y - ui_status_height());

    struct ui_mods *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_mods) {
        .panel = ui_panel_title(pos, dim, ui_str_v(12)),
        .new = ui_button_new(font, ui_str_c("+")),
        .new_val = ui_input_new(font, symbol_cap),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .toggles = ui_toggles_new(font, ui_str_v(symbol_cap)),
    };

    ui->panel.state = ui_panel_hidden;
    return ui;
}


void ui_mods_free(struct ui_mods *ui) {
    ui_panel_free(&ui->panel);
    ui_button_free(&ui->new);
    ui_input_free(&ui->new_val);
    ui_scroll_free(&ui->scroll);
    ui_toggles_free(&ui->toggles);
    free(ui);
}

int16_t ui_mods_width(const struct ui_mods *ui)
{
    return ui->panel.w.dim.w;
}

static void ui_mods_update(struct ui_mods *ui)
{
    struct mods_list *list = mods_list(world_mods(core.state.world));
    ui_toggles_resize(&ui->toggles, list->len);
    ui_scroll_update(&ui->scroll, list->len);

    for (size_t i = 0; i < list->len; ++i) {
        struct ui_toggle *toggle = &ui->toggles.items[i];
        ui_str_set_symbol(&toggle->str, &list->items[i].str);
        toggle->user = list->items[i].maj;
    }

    ui_str_setf(&ui->panel.title.str, "mods(%zu)", list->len);
    free(list);
}

static bool ui_mods_event_user(struct ui_mods *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui->panel.state = ui_panel_hidden;
        return false;
    }

    case EV_MODS_TOGGLE: {
        if (ui->panel.state == ui_panel_hidden) {
            ui_mods_update(ui);
            ui->panel.state = ui_panel_visible;
            core_push_event(EV_FOCUS_PANEL, (uintptr_t) &ui->panel, 0);
        }
        else {
            if (ui->panel.state == ui_panel_focused)
                core_push_event(EV_FOCUS_PANEL, 0, 0);
            ui->panel.state = ui_panel_hidden;
        }
        return false;
    }

    case EV_MOD_SELECT: {
        mod_t mod = (uintptr_t) ev->user.data1;
        ui_toggles_select(&ui->toggles, mod_maj(mod));

        if (ui->panel.state == ui_panel_hidden) {
            ui->panel.state = ui_panel_visible;
            ui_mods_update(ui);
        }

        return false;
    }

    case EV_MOD_CLEAR: {
        ui_toggles_clear(&ui->toggles);
        return false;
    }

    case EV_TAPES_TOGGLE:
    case EV_TAPE_SELECT:
    case EV_STARS_TOGGLE:
    case EV_LOG_TOGGLE:
    case EV_LOG_SELECT: {
        ui->panel.state = ui_panel_hidden;
        return false;
    }

    default: { return false; }
    }
}

bool ui_mods_event(struct ui_mods *ui, SDL_Event *ev)
{
    if (ev->type == core.event && ui_mods_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret == ui_consume;

    if ((ret = ui_input_event(&ui->new_val, ev))) return ret == ui_consume;
    if ((ret = ui_button_event(&ui->new, ev))) {
        struct symbol name = {0};
        if (ui_input_get_symbol(&ui->new_val, &name)) {
            mod_t mod = mods_register(world_mods(core.state.world), &name);
            if (mod) {
                ui_mods_update(ui);
                core_push_event(EV_MOD_SELECT, mod, 0);
            }
        }
        return ret;
    }

    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;

    struct ui_toggle *toggle = NULL;
    if ((ret = ui_toggles_event(&ui->toggles, ev, &ui->scroll, &toggle, NULL))) {
        enum event type = toggle->state == ui_toggle_selected ? EV_MOD_SELECT : EV_MOD_CLEAR;
        core_push_event(type, make_mod(toggle->user, 0), 0);
        return true;
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_mods_render(struct ui_mods *ui, SDL_Renderer *renderer)
{
    struct font *font = ui_mods_font();

    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_input_render(&ui->new_val, &layout, renderer);
    ui_button_render(&ui->new, &layout, renderer);
    ui_layout_next_row(&layout);
    ui_layout_sep_y(&layout, font->glyph_h);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, &layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    ui_toggles_render(&ui->toggles, &inner, renderer, &ui->scroll);
}
