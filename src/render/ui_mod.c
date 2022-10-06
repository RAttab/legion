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

struct ui_mod
{
    mod_id id;
    const struct mod *mod;
    bool disassembly;
    vm_ip ip;

    struct ui_panel panel;
    struct ui_button compile, publish;
    struct ui_button mode, indent;
    struct ui_button import, export;
    struct ui_button reset;
    struct ui_code code;
};

struct ui_mod *ui_mod_new(void)
{
    enum { cols = 80 };
    struct pos pos = make_pos(
            ui_mods_width(render.ui.mods),
            ui_topbar_height());

    struct dim dim = make_dim(
            (ui_code_num_len+1 + cols + 2) * ui_st.font.dim.w,
            render.rect.h - pos.y - ui_status_height());

    struct ui_mod *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_mod) {
        .panel = ui_panel_title(pos, dim, ui_str_v(6 + symbol_cap + 5)),
        .compile = ui_button_new(ui_str_c("compile")),
        .publish = ui_button_new(ui_str_c("publish")),
        .mode = ui_button_new(ui_str_v(4)),
        .indent = ui_button_new(ui_str_c("indent")),
        .import = ui_button_new(ui_str_c("import")),
        .export = ui_button_new(ui_str_c("export")),
        .reset = ui_button_new(ui_str_c("reset")),
        .code = ui_code_new(make_dim(ui_layout_inf, ui_layout_inf))
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
    mod_free(ui->mod);
    free(ui);
}

static void ui_mod_mode_swap(struct ui_mod *ui)
{
    ui->disassembly = !ui->disassembly;

    vm_ip ip = ui_code_ip(&ui->code);

    if (ui->disassembly) {
        ui_str_setv(&ui->mode.str, "code", 4);
        ui_code_set_disassembly(&ui->code, ui->mod, ip);
        ui->reset.disabled = true;
        ui->compile.disabled = true;
        ui->publish.disabled = true;
    }

    else {
        ui_str_setv(&ui->mode.str, "asm", 3);
        ui_code_set_code(&ui->code, ui->mod, ip);
        ui->reset.disabled = false;
        ui->compile.disabled = false;
        ui->publish.disabled = ui->mod->errs_len || mod_version(ui->mod->id);
    }
}

static void ui_mod_update(struct ui_mod *ui)
{
    const struct mod *mod = proxy_mod(render.proxy);
    if (!mod) return;
    if (mod_major(mod->id) != mod_major(ui->id)) {
        mod_free(mod);
        return;
    }

    mod_free(legion_xchg(&ui->mod, mod));
    ui->id = mod->id;

    if (!mod->errs_len) ui->mode.disabled = false;
    else {
        ui->mode.disabled = true;
        ui->disassembly = false;
        ui_str_setv(&ui->mode.str, "code", 4);
    }

    if (!ui->disassembly) {
        ui_code_set_code(&ui->code, mod, ui->ip);
        ui->reset.disabled = false;
        ui->compile.disabled = false;
        ui->publish.disabled = mod->errs_len || mod_version(mod->id);
    }
    else {
        ui_code_set_disassembly(&ui->code, mod, ui->ip);
        ui->reset.disabled = true;
        ui->compile.disabled = true;
        ui->publish.disabled = true;
    }

    struct symbol name = {0};
    bool ok = proxy_mod_name(render.proxy, mod_major(ui->id), &name);
    ui_str_setf(&ui->panel.title.str, "mod - %s.%x", name.c, mod_version(ui->id));
    assert(ok);
}

static void ui_mod_select(struct ui_mod *ui, mod_id id, vm_ip ip)
{
    if (ui->mod && id == ui->mod->id) {
        ui_code_goto(&ui->code, ip);
        return;
    }

    ui->id = mod_version(id) ?
        id : proxy_mod_latest(render.proxy, mod_major(id));
    ui->ip = ip;

    proxy_mod_select(render.proxy, ui->id);
    ui_code_clear(&ui->code);
    ui_panel_show(&ui->panel);
    ui_code_focus(&ui->code);
}

static void ui_mod_import(struct ui_mod *ui)
{
    struct symbol name = {0};
    bool ok = proxy_mod_name(render.proxy, mod_major(ui->id), &name);
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
    bool ok = proxy_mod_name(render.proxy, mod_major(ui->id), &name);
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
        mod_free(ui->mod);
        ui->mod = NULL;
        ui->id = 0;
        ui->ip = 0;

        ui_code_clear(&ui->code);
        ui_panel_hide(&ui->panel);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        ui_mod_update(ui);
        return false;
    }

    case EV_MOD_SELECT: {
        mod_id id = (uintptr_t) ev->user.data1;
        vm_ip ip = (uintptr_t) ev->user.data2;
        ui_mod_select(ui, id, ip);
        return false;
    }

    case EV_MOD_BREAKPOINT: {
        vm_ip ip = (uintptr_t) ev->user.data1;
        mod_id mod = (uintptr_t) ev->user.data2;
        if (mod != ui->id) return true;

        ui_code_breakpoint_ip(&ui->code, ip);
        return true;
    }

    case EV_TAPES_TOGGLE:
    case EV_TAPE_SELECT:
    case EV_STARS_TOGGLE:
    case EV_MOD_CLEAR:
    case EV_MODS_TOGGLE:
    case EV_LOG_TOGGLE:
    case EV_LOG_SELECT: {
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
        if (ret == ui_action)
            render_push_event(EV_MOD_CLEAR, 0, 0);
        return ret != ui_skip;
    }

    if ((ret = ui_button_event(&ui->compile, ev))) {
        if (ret != ui_action) return true;

        size_t len = ui->code.text.bytes;
        char *buffer = calloc(len, sizeof(*buffer));
        text_to_str(&ui->code.text, buffer, len);

        proxy_mod_compile(render.proxy, mod_major(ui->id), buffer, len);

        free(buffer);
        return true;
    }

    if ((ret = ui_button_event(&ui->publish, ev))) {
        if (ret != ui_action) return true;
        assert(ui->mod->errs_len == 0);
        proxy_mod_publish(render.proxy, mod_major(ui->id));
        ui->publish.disabled = true;
        return true;
    }

    if ((ret = ui_button_event(&ui->mode, ev))) {
        if (ret != ui_action) return true;
        ui_mod_mode_swap(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->indent, ev))) {
        if (ret != ui_action) return true;
        ui_code_indent(&ui->code);
        return true;
    }

    if ((ret = ui_button_event(&ui->import, ev))) {
        if (ret != ui_action) return true;
        ui_mod_import(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->export, ev))) {
        if (ret != ui_action) return true;
        ui_mod_export(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->reset, ev))) {
        if (ret != ui_action) return true;
        proxy_mod_select(render.proxy, ui->id);
        return true;
    }

    if ((ret = ui_code_event(&ui->code, ev))) return true;

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_mod_render(struct ui_mod *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    ui_button_render(&ui->compile, &layout, renderer);
    ui_button_render(&ui->publish, &layout, renderer);

    ui_layout_sep_col(&layout);

    ui_button_render(&ui->mode, &layout, renderer);
    ui_button_render(&ui->indent, &layout, renderer);

    ui_layout_sep_col(&layout);

    ui_button_render(&ui->import, &layout, renderer);
    ui_button_render(&ui->export, &layout, renderer);

    ui_layout_dir(&layout, ui_layout_left);
    ui_button_render(&ui->reset, &layout, renderer);

    ui_layout_dir(&layout, ui_layout_right);
    ui_layout_next_row(&layout);
    ui_layout_sep_row(&layout);

    ui_code_render(&ui->code, &layout, renderer);
}
