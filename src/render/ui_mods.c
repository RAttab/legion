/* ui_mods.c
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

enum { ui_mods_cols = 80 };

struct ui_mods
{
    struct {
        mod_id id;
        const struct mod *mod;
        bool disassembly;
        vm_ip ip;
        bool new;
    } state;

    struct ui_panel *panel;
    int16_t height, list_w, code_w, pad_w;

    struct ui_button new;
    struct ui_input new_val;
    struct ui_list list;

    struct ui_label mod, mod_val;
    struct ui_button compile, publish;
    struct ui_button mode, indent;
    struct ui_button import, export;
    struct ui_button reset;
    struct ui_code code;
};

struct ui_mods *ui_mods_new(void)
{
    struct pos pos = make_pos(0, ui_topbar_height());
    int16_t height = render.rect.h - pos.y - ui_status_height();
    int16_t list_w = (symbol_cap + 4) * ui_st.font.dim.w;
    int16_t code_w = (ui_code_num_len+1 + ui_mods_cols + 2) * ui_st.font.dim.w;
    int16_t pad_w = ui_st.font.dim.w;

    struct ui_mods *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_mods) {
        .panel = ui_panel_title(
                pos, make_dim(list_w + pad_w, height), ui_str_c("mods")),
        .height = height,
        .list_w = list_w,
        .code_w = code_w,
        .pad_w = pad_w,

        .new = ui_button_new(ui_str_c("+")),
        .new_val = ui_input_new(symbol_cap),
        .list = ui_list_new(make_dim(list_w, ui_layout_inf), symbol_cap),

        .mod = ui_label_new(ui_str_c("mod: ")),
        .mod_val = ui_label_new_s(&ui_st.label.bold, ui_str_v(symbol_cap + 1 + 4)),
        .compile = ui_button_new(ui_str_c("compile")),
        .publish = ui_button_new(ui_str_c("publish")),
        .mode = ui_button_new(ui_str_v(4)),
        .indent = ui_button_new(ui_str_c("indent")),
        .import = ui_button_new(ui_str_c("import")),
        .export = ui_button_new(ui_str_c("export")),
        .reset = ui_button_new(ui_str_c("reset")),
        .code = ui_code_new(make_dim(ui_layout_inf, ui_layout_inf))
    };

    ui_panel_hide(ui->panel);

    ui->state.disassembly = false;
    ui_str_setv(&ui->mode.str, "asm", 3);

    return ui;
}

void ui_mods_free(struct ui_mods *ui)
{
    ui_panel_free(ui->panel);

    ui_button_free(&ui->new);
    ui_input_free(&ui->new_val);
    ui_list_free(&ui->list);

    ui_label_free(&ui->mod);
    ui_label_free(&ui->mod_val);
    ui_button_free(&ui->compile);
    ui_button_free(&ui->publish);
    ui_button_free(&ui->mode);
    ui_button_free(&ui->indent);
    ui_button_free(&ui->import);
    ui_button_free(&ui->export);
    ui_button_free(&ui->reset);

    ui_code_free(&ui->code);
    mod_free(ui->state.mod);

    free(ui);
}

static void ui_mods_mode_swap(struct ui_mods *ui)
{
    ui->state.disassembly = !ui->state.disassembly;

    vm_ip ip = ui_code_ip(&ui->code);

    if (ui->state.disassembly) {
        ui_str_setv(&ui->mode.str, "code", 4);
        ui_code_set_disassembly(&ui->code, ui->state.mod, ip);
        ui->reset.disabled = true;
        ui->compile.disabled = true;
        ui->publish.disabled = true;
    }

    else {
        ui_str_setv(&ui->mode.str, "asm", 3);
        ui_code_set_code(&ui->code, ui->state.mod, ip);
        ui->reset.disabled = false;
        ui->compile.disabled = false;
        ui->publish.disabled =
            ui->state.mod->errs_len || mod_version(ui->state.mod->id);
    }
}

static void ui_mods_update(struct ui_mods *ui)
{
    ui_list_reset(&ui->list);

    const struct mods_list *list = proxy_mods(render.proxy);
    for (size_t i = 0; i < list->len; ++i) {
        const struct mods_item *mod = list->items + i;
        ui_str_set_symbol(ui_list_add(&ui->list, mod->maj), &mod->str);
    }


    const struct mod *mod = proxy_mod(render.proxy);
    if (!mod) return;
    if (!ui->state.new && mod_major(mod->id) != mod_major(ui->state.id)) {
        mod_free(mod);
        return;
    }

    ui_panel_resize(ui->panel,
            make_dim(ui->list_w + ui->code_w + ui->pad_w, ui->height));

    mod_free(legion_xchg(&ui->state.mod, mod));
    ui->state.id = mod->id;

    if (ui->state.new) {
        ui->list.selected = mod_major(ui->state.id);
        ui->state.new = false;
    }

    if (!mod->errs_len) ui->mode.disabled = false;
    else {
        ui->mode.disabled = true;
        ui->state.disassembly = false;
        ui_str_setv(&ui->mode.str, "code", 4);
    }

    if (!ui->state.disassembly) {
        ui_code_set_code(&ui->code, mod, ui->state.ip);
        ui->reset.disabled = false;
        ui->compile.disabled = false;
        ui->publish.disabled = mod->errs_len || mod_version(mod->id);
    }
    else {
        ui_code_set_disassembly(&ui->code, mod, ui->state.ip);
        ui->reset.disabled = true;
        ui->compile.disabled = true;
        ui->publish.disabled = true;
    }

    struct symbol name = {0};
    bool ok = proxy_mod_name(render.proxy, mod_major(ui->state.id), &name);
    assert(ok);
    
    if (!mod_version(ui->state.mod->id)) {
        ui_str_setf(&ui->mod_val.str, "%s", name.c);
        ui->mod_val.s.fg = ui_st.rgba.working;
    }
    else {
        ui_str_setf(&ui->mod_val.str, "%s.%x", name.c, mod_version(ui->state.id));
        ui->mod_val.s.fg = ui_st.rgba.active;
    }
}

static void ui_mods_select(struct ui_mods *ui, mod_id id, vm_ip ip)
{
    if (ui->state.mod && id == ui->state.mod->id) {
        ui_code_goto(&ui->code, ip);
        return;
    }

    ui->state.id = mod_version(id) ?
        id : proxy_mod_latest(render.proxy, mod_major(id));
    ui->state.ip = ip;

    proxy_mod_select(render.proxy, ui->state.id);
    ui_code_clear(&ui->code);
    ui_panel_show(ui->panel);
    ui_code_focus(&ui->code);
}

static void ui_mods_import(struct ui_mods *ui)
{
    struct symbol name = {0};
    bool ok = proxy_mod_name(render.proxy, mod_major(ui->state.id), &name);
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

static void ui_mods_export(struct ui_mods *ui)
{
    struct symbol name = {0};
    bool ok = proxy_mod_name(render.proxy, mod_major(ui->state.id), &name);
    assert(ok);

    char path[PATH_MAX] = {0};
    sys_path_mod(name.c, path, sizeof(path));

    struct mfilew file = mfilew_create_tmp(path, ui->code.text.bytes);
    text_to_str(&ui->code.text, file.ptr, file.len);
    mfilew_close(&file);

    file_tmp_swap(path);

    render_log(st_info, "mod '%s' exported to '%s'", name.c, path);
}

static void ui_mods_clear(struct ui_mods *ui)
{
    mod_free(ui->state.mod);
    ui->state.mod = NULL;
    ui->state.id = 0;
    ui->state.ip = 0;

    ui_list_clear(&ui->list);
    ui_code_clear(&ui->code);

    ui_panel_hide(ui->panel);
    ui_panel_resize(ui->panel, make_dim(ui->list_w + ui->pad_w, ui->height));
}

static bool ui_mods_event_user(struct ui_mods *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_mods_clear(ui);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(ui->panel)) return false;
        ui_mods_update(ui);
        return false;
    }

    case EV_MODS_TOGGLE: {
        if (ui_panel_is_visible(ui->panel)) {
            ui_mods_clear(ui);
            return false;
        }

        ui_mods_update(ui);
        ui_panel_show(ui->panel);
        return false;
    }

    case EV_MOD_SELECT: {
        mod_id id = (uintptr_t) ev->user.data1;
        vm_ip ip = (uintptr_t) ev->user.data2;
        ui_list_select(&ui->list, mod_major(id));
        ui_mods_select(ui, id, ip);
        return false;
    }

    case EV_MOD_BREAKPOINT: {
        vm_ip ip = (uintptr_t) ev->user.data1;
        mod_id mod = (uintptr_t) ev->user.data2;
        if (mod != ui->state.id) return true;

        ui_code_breakpoint_ip(&ui->code, ip);
        return true;
    }

    case EV_TAPES_TOGGLE:
    case EV_TAPE_SELECT:
    case EV_STARS_TOGGLE:
    case EV_MOD_CLEAR:
    case EV_LOG_TOGGLE:
    case EV_LOG_SELECT: {
        ui_mods_clear(ui);
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
    ui->state.new = true;
}

bool ui_mods_event(struct ui_mods *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_mods_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;

    if ((ret = ui_panel_event(ui->panel, ev))) {
        if (ret == ui_action)
            render_push_event(EV_MOD_CLEAR, 0, 0);
        return ret != ui_skip;
    }

    if ((ret = ui_input_event(&ui->new_val, ev))) {
        if (ret == ui_action) ui_mods_event_new(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->new, ev))) {
        if (ret != ui_action) return true;
        ui_mods_event_new(ui);
        return ret;
    }

    if ((ret = ui_list_event(&ui->list, ev))) {
        if (ret != ui_action) return true;
        render_push_event(EV_MOD_SELECT, make_mod(ui->list.selected, 0), 0);
        return true;
    }

    if (!ui->state.mod) return ui_panel_event_consume(ui->panel, ev);

    if ((ret = ui_button_event(&ui->compile, ev))) {
        if (ret != ui_action) return true;

        size_t len = ui->code.text.bytes;
        char *buffer = calloc(len, sizeof(*buffer));
        text_to_str(&ui->code.text, buffer, len);

        proxy_mod_compile(render.proxy, mod_major(ui->state.id), buffer, len);

        free(buffer);
        return true;
    }

    if ((ret = ui_button_event(&ui->publish, ev))) {
        if (ret != ui_action) return true;
        assert(ui->state.mod->errs_len == 0);
        proxy_mod_publish(render.proxy, mod_major(ui->state.id));
        ui->publish.disabled = true;
        return true;
    }

    if ((ret = ui_button_event(&ui->mode, ev))) {
        if (ret != ui_action) return true;
        ui_mods_mode_swap(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->indent, ev))) {
        if (ret != ui_action) return true;
        ui_code_indent(&ui->code);
        return true;
    }

    if ((ret = ui_button_event(&ui->import, ev))) {
        if (ret != ui_action) return true;
        ui_mods_import(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->export, ev))) {
        if (ret != ui_action) return true;
        ui_mods_export(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->reset, ev))) {
        if (ret != ui_action) return true;
        proxy_mod_select(render.proxy, ui->state.id);
        return true;
    }

    if ((ret = ui_code_event(&ui->code, ev))) return true;

    return ui_panel_event_consume(ui->panel, ev);
}

void ui_mods_render(struct ui_mods *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    struct ui_layout inner = ui_layout_split_x(&layout, ui->list_w);
    {
        ui_input_render(&ui->new_val, &inner, renderer);
        ui_button_render(&ui->new, &inner, renderer);
        ui_layout_next_row(&inner);

        ui_layout_sep_row(&inner);

        ui_list_render(&ui->list, &inner, renderer);
    }

    if (!ui->state.mod) return;

    ui_label_render(&ui->mod_val, &layout, renderer);
    ui_layout_next_row(&layout);
    ui_layout_sep_y(&layout, 2);

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
