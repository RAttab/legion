/* ui_mod.c
   Rémi Attab (remi.attab@gmail.com), 18 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "vm/mod.h"
#include "utils/fs.h"


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

enum ui_mod_action
{
    ui_mod_nil = 0,
    ui_mod_select,
    ui_mod_compile,
    ui_mod_publish,
};

struct ui_mod
{
    mod_t id;
    const struct mod *mod;
    bool disassembly;

    enum ui_mod_action action;
    ip_t select_ip;

    struct ui_panel panel;
    struct ui_button compile, publish;
    struct ui_button mode, indent;
    struct ui_button import, export;
    struct ui_button reset;
    struct ui_code code;
};

static struct font *ui_mod_font(void) { return font_mono6; }

struct ui_mod *ui_mod_new(void)
{
    enum { cols = 80 };
    struct font *font = ui_mod_font();

    struct pos pos = make_pos(
            ui_mods_width(render.ui.mods),
            ui_topbar_height());

    struct dim dim = make_dim(
            (ui_code_num_len+1 + cols + 2) * font->glyph_w,
            render.rect.h - pos.y - ui_status_height());

    struct ui_mod *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_mod) {
        .panel = ui_panel_title(pos, dim, ui_str_v(6 + symbol_cap + 5)),
        .compile = ui_button_new(font, ui_str_c("compile")),
        .publish = ui_button_new(font, ui_str_c("publish")),
        .mode = ui_button_new(font, ui_str_v(4)),
        .indent = ui_button_new(font, ui_str_c("indent")),
        .import = ui_button_new(font, ui_str_c("import")),
        .export = ui_button_new(font, ui_str_c("export")),
        .reset = ui_button_new(font, ui_str_c("reset")),
        .code = ui_code_new(make_dim(ui_layout_inf, ui_layout_inf), font)
    };

    ui_panel_hide(&ui->panel);

    ui->disassembly = false;
    ui_str_setv(&ui->mode.str, "asm", 3);

    return ui;
}

void ui_mod_free(struct ui_mod *ui)
{
    ui_panel_free(&ui->panel);
    ui_button_free(&ui->compile);
    ui_button_free(&ui->publish);
    ui_button_free(&ui->mode);
    ui_button_free(&ui->indent);
    ui_button_free(&ui->import);
    ui_button_free(&ui->export);
    ui_button_free(&ui->reset);
    ui_code_free(&ui->code);
    free(ui);
}

static void ui_mod_mode_swap(struct ui_mod *ui)
{
    ui->disassembly = !ui->disassembly;

    ip_t ip = ui_code_ip(&ui->code);

    if (ui->disassembly) {
        ui_str_setv(&ui->mode.str, "code", 4);
        ui_code_set_disassembly(&ui->code, ui->mod, ip);
        ui->reset.disabled = true;
    }

    else {
        ui_str_setv(&ui->mode.str, "asm", 3);
        ui_code_set_code(&ui->code, ui->mod, ip);
        ui->reset.disabled = false;
    }
}

static void ui_mod_update(struct ui_mod *ui, const struct mod *mod, ip_t ip)
{
    assert(mod);

    ui->mod = mod;
    ui->publish.disabled = mod->errs_len > 0;

    if (!mod->errs_len) ui->mode.disabled = false;
    else {
        ui->mode.disabled = true;
        ui->disassembly = false;
        ui_str_setv(&ui->mode.str, "code", 4);
    }

    if (!ui->disassembly) ui_code_set_code(&ui->code, mod, ip);
    else ui_code_set_disassembly(&ui->code, mod, ip);
}

static void ui_mod_action(struct ui_mod *ui, enum ui_mod_action action)
{
    assert(action);
    // select is the one action that can preempt other actions.
    assert(!ui->action || action == ui->action || action == ui_mod_select);

    ui->action = action;
    ui->compile.disabled = true;
    ui->publish.disabled = true;
}

static void ui_mod_action_clear(struct ui_mod *ui)
{
    ui->action = ui_mod_nil;
    ui->select_ip = 0;

    ui->compile.disabled = false;
    ui->publish.disabled =
        !ui->mod ||
        ui->mod->errs_len ||
        ui->mod->id == ui->id ||
        mod_ver(ui->mod->id);

    struct symbol name = {0};
    bool ok = proxy_mod_name(render.proxy, mod_maj(ui->id), &name);
    ui_str_setf(&ui->panel.title.str, "mod - %s.%x", name.c, mod_ver(ui->id));
    assert(ok);
}

static void ui_mod_action_select(struct ui_mod *ui)
{
    const struct mod *mod = proxy_mod(render.proxy, ui->id);
    if (!mod) { ui_mod_action(ui, ui_mod_select); return; }

    ui_mod_update(ui, mod, ui->select_ip);
    ui_mod_action_clear(ui);
}

static void ui_mod_action_compile(struct ui_mod *ui)
{
    const struct mod *mod = proxy_mod_compile_result(render.proxy);
    if (!mod) return;
    assert(mod_maj(mod->id) == mod_maj(ui->id));

    ui_mod_update(ui, mod, 0);
    ui_mod_action_clear(ui);
}

static void ui_mod_action_publish(struct ui_mod *ui)
{
    mod_t id = proxy_mod_id(render.proxy);
    if (!id) return;

    const struct mod *mod = proxy_mod(render.proxy, id);
    assert(mod_maj(id) == mod_maj(ui->id));
    assert(mod_ver(id) > mod_ver(ui->id));
    assert(mod);

    ui->id = mod->id;
    ui_mod_action_clear(ui);
}

static void ui_mod_import(struct ui_mod *ui)
{
    struct symbol name = {0};
    bool ok = proxy_mod_name(render.proxy, mod_maj(ui->id), &name);
    assert(ok);

    char path[PATH_MAX] = {0};
    sys_path_mod(name.c, path, sizeof(path));

    if (!file_exists(path)) {
        render_log(st_error,
                "unable to import mod '%s': '%s' doesn't exist", name.c, path);
    }

    struct mfile file = mfile_open(path);
    ui_code_set_text(&ui->code, file.ptr, file.len);
    mfile_close(&file);

    render_log(st_info, "mod '%s' imported from '%s'", name.c, path);
}

static void ui_mod_export(struct ui_mod *ui)
{
    struct symbol name = {0};
    bool ok = proxy_mod_name(render.proxy, mod_maj(ui->id), &name);
    assert(ok);

    char path[PATH_MAX] = {0};
    sys_path_mod(name.c, path, sizeof(path));

    struct mfilew file = mfilew_create_tmp(path, ui->code.text.bytes);
    text_to_str(&ui->code.text, file.ptr, file.len);
    mfilew_close(&file);

    file_tmp_swap(path);

    render_log(st_info, "mod '%s' exported to '%s'", name.c, path);
}

static bool ui_mod_event_user(struct ui_mod *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui->id = 0;
        ui->mod = NULL;
        ui->action = ui_mod_nil;
        ui->select_ip = 0;

        ui_code_clear(&ui->code);
        ui_panel_hide(&ui->panel);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel)) return false;

        switch (ui->action) {
        case ui_mod_nil: { break; }
        case ui_mod_select: { ui_mod_action_select(ui); break; }
        case ui_mod_compile: { ui_mod_action_compile(ui); break; }
        case ui_mod_publish: { ui_mod_action_publish(ui); break; }
        default: { assert(false); }
        }
        return false;
    }

    case EV_MOD_SELECT: {
        mod_t id = (uintptr_t) ev->user.data1;
        ip_t ip = (uintptr_t) ev->user.data2;

        if (ui->mod && id == ui->mod->id) {
            ui_code_goto(&ui->code, ip);
            return false;
        }

        ui_code_clear(&ui->code);

        ui->id = mod_ver(id) ? id : proxy_mod_latest(render.proxy, mod_maj(id));
        ui->select_ip = ip;
        ui_mod_action_select(ui);

        ui_panel_show(&ui->panel);
        ui_code_focus(&ui->code);
        return false;
    }

    case EV_TAPES_TOGGLE:
    case EV_TAPE_SELECT:
    case EV_STARS_TOGGLE:
    case EV_MOD_CLEAR:
    case EV_LOG_TOGGLE:
    case EV_LOG_SELECT: {
        if (ui->action) ui_mod_action_clear(ui);
        ui_panel_hide(&ui->panel);
        ui_code_clear(&ui->code);
        ui->mod = NULL;
        ui->id = 0;
        return false;
    }

    default: { return false; }
    }
}

bool ui_mod_event(struct ui_mod *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_mod_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;

    if ((ret = ui_panel_event(&ui->panel, ev))) {
        if (ret == ui_consume && !ui_panel_is_visible(&ui->panel))
            render_push_event(EV_MOD_CLEAR, 0, 0);
        return ret != ui_skip;
    }

    if ((ret = ui_button_event(&ui->compile, ev))) {
        size_t len = ui->code.text.bytes;
        char *buffer = calloc(len, sizeof(*buffer));
        text_to_str(&ui->code.text, buffer, len);

        proxy_mod_compile(render.proxy, mod_maj(ui->id), buffer, len);
        ui_mod_action(ui, ui_mod_compile);
        return true;
    }

    if ((ret = ui_button_event(&ui->publish, ev))) {
        assert(ui->mod->errs_len == 0);
        proxy_mod_publish(render.proxy, mod_maj(ui->id));
        ui_mod_action(ui, ui_mod_publish);
        return true;
    }

    if ((ret = ui_button_event(&ui->mode, ev))) {
        ui_mod_mode_swap(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->indent, ev))) {
        ui_code_indent(&ui->code);
        return true;
    }

    if ((ret = ui_button_event(&ui->import, ev))) {
        ui_mod_import(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->export, ev))) {
        ui_mod_export(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->reset, ev))) {
        assert(ui->id == proxy_mod_id(render.proxy));
        ui_mod_update(ui, proxy_mod(render.proxy, ui->id), 0);
        return true;
    }

    if ((ret = ui_code_event(&ui->code, ev))) return true;

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_mod_render(struct ui_mod *ui, SDL_Renderer *renderer)
{
    struct font *font = ui_mod_font();

    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_button_render(&ui->compile, &layout, renderer);
    ui_button_render(&ui->publish, &layout, renderer);

    ui_layout_sep_x(&layout, 6);

    ui_button_render(&ui->mode, &layout, renderer);
    ui_button_render(&ui->indent, &layout, renderer);

    ui_layout_sep_x(&layout, 6);

    ui_button_render(&ui->import, &layout, renderer);
    ui_button_render(&ui->export, &layout, renderer);

    ui_layout_dir(&layout, ui_layout_left);
    ui_button_render(&ui->reset, &layout, renderer);

    ui_layout_dir(&layout, ui_layout_right);
    ui_layout_next_row(&layout);
    ui_layout_sep_y(&layout, font->glyph_h);

    ui_code_render(&ui->code, &layout, renderer);
}
