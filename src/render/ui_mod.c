/* ui_mod.c
   Rémi Attab (remi.attab@gmail.com), 18 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "vm/mod.h"


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

struct ui_mod
{
    mod_t id;
    const struct mod *mod;

    struct ui_panel panel;
    struct ui_button compile, publish, reset;
    struct ui_code code;
};

static struct font *ui_mod_font(void) { return font_mono6; }

struct ui_mod *ui_mod_new(void)
{
    enum { cols = 80 };
    struct font *font = ui_mod_font();

    struct pos pos = make_pos(
            ui_mods_width(core.ui.mods),
            ui_topbar_height(core.ui.topbar));

    struct dim dim = make_dim(
            (ui_code_num_len+1 + cols + 2) * font->glyph_w,
            core.rect.h - pos.y);

    struct ui_mod *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_mod) {
        .panel = ui_panel_title(pos, dim, ui_str_v(6 + symbol_cap + 5)),
        .compile = ui_button_new(font, ui_str_c("compile")),
        .publish = ui_button_new(font, ui_str_c("publish")),
        .reset = ui_button_new(font, ui_str_c("reset")),
        .code = ui_code_new(make_dim(ui_layout_inf, ui_layout_inf), font)
    };
    ui->panel.state = ui_panel_hidden;
    return ui;
}

void ui_mod_free(struct ui_mod *ui)
{
    ui_panel_free(&ui->panel);
    ui_button_free(&ui->compile);
    ui_button_free(&ui->publish);
    ui_button_free(&ui->reset);
    ui_code_free(&ui->code);
    free(ui);
}

static void ui_mod_update(struct ui_mod *ui, const struct mod *mod, ip_t ip)
{
    assert(mod);

    ui->mod = mod;
    ui_code_set(&ui->code, mod, ip);
    ui->publish.disabled = mod->errs_len > 0;
}


static void ui_mod_title(struct ui_mod *ui)
{
    struct symbol name = {0};
    mods_name(world_mods(core.state.world), mod_id(ui->id), &name);
    ui_str_setf(&ui->panel.title.str, "mod - %s.%x", name.c, mod_ver(ui->id));
}

static const struct mod *ui_mod_compile(struct ui_mod *ui)
{
    size_t len = ui->code.text.bytes;
    char *buffer = calloc(len, sizeof(*buffer));
    text_to_str(&ui->code.text, buffer, len);

    const struct mod *mod = mod_compile(
            len, buffer,
            world_mods(core.state.world),
            world_atoms(core.state.world));

    free(buffer);
    return mod;
}

static bool ui_mod_event_user(struct ui_mod *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui->panel.state = ui_panel_hidden;
        ui->id = 0;
        ui->mod = NULL;
        ui_code_clear(&ui->code);
        return false;
    }

    case EV_MOD_SELECT: {
        mod_t id = (uintptr_t) ev->user.data1;
        ip_t ip = (uintptr_t) ev->user.data2;

        const struct mod *mod = mods_get(world_mods(core.state.world), id);
        assert(mod);

        ui->id = mod->id;
        ui_mod_update(ui, mod, ip);
        ui->publish.disabled = true;
        ui_mod_title(ui);

        ui->panel.state = ui_panel_visible;
        core_push_event(EV_FOCUS_PANEL, (uintptr_t) &ui->panel, 0);
        core_push_event(EV_FOCUS_INPUT, (uintptr_t) &ui->code, 0);

        return false;
    }

    case EV_MOD_CLEAR: {
        ui_code_clear(&ui->code);
        ui->mod = NULL;
        ui->id = 0;
        ui->panel.state = ui_panel_hidden;
        return false;
    }

    default: { return false; }
    }
}

bool ui_mod_event(struct ui_mod *ui, SDL_Event *ev)
{
    if (ev->type == core.event && ui_mod_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;

    if ((ret = ui_panel_event(&ui->panel, ev))) {
        if (ret == ui_consume && ui->panel.state == ui_panel_hidden)
            core_push_event(EV_MOD_CLEAR, 0, 0);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->compile, ev))) {
        ui_mod_update(ui, ui_mod_compile(ui), 0);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->publish, ev))) {
        assert(ui->mod->errs_len == 0);
        ui->id = mods_set(world_mods(core.state.world), mod_id(ui->id), ui->mod);
        ui->publish.disabled = true;
        ui_mod_title(ui);
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->reset, ev))) {
        ui_mod_update(ui, mods_get(world_mods(core.state.world), ui->id), 0);
        return ret == ui_consume;
    }

    if ((ret = ui_code_event(&ui->code, ev))) return ret == ui_consume;

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_mod_render(struct ui_mod *ui, SDL_Renderer *renderer)
{
    struct font *font = ui_mod_font();

    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_button_render(&ui->compile, &layout, renderer);
    ui_button_render(&ui->publish, &layout, renderer);
    ui_layout_right(&layout, &ui->reset.w);
    ui_button_render(&ui->reset, &layout, renderer);
    ui_layout_next_row(&layout);
    ui_layout_sep_y(&layout, font->glyph_h);

    ui_code_render(&ui->code, &layout, renderer);
}
