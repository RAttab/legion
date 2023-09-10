/* ui_mods.c
   Rémi Attab (remi.attab@gmail.com), 18 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "vm/mod.h"
#include "vm/code.h"
#include "utils/fs.h"

static void ui_mods_free(void *);
static void ui_mods_update(void *);
static bool ui_mods_event(void *, SDL_Event *);
static void ui_mods_render(void *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

constexpr uint32_t ui_mods_cols = 80;
enum ui_mods_mode { ui_mods_code = 0, ui_mods_asm = 1 };

struct ui_mods_tab
{
    uint64_t user;
    bool init, write;

    mod_id id;
    const struct mod *mod;

    struct { bool loaded; bool attached; vm_ip ip; } debug;

    enum ui_mods_mode mode;
    struct ui_code code;
    struct ui_asm as;
};

struct ui_mods
{
    struct ui_panel *panel;
    int16_t tree_w, code_w, pad_w;

    struct ui_button new;
    struct ui_input new_val;
    struct ui_tree tree;

    struct ui_button mode;
    struct ui_button build, publish;
    struct ui_button import, export;
    struct ui_button load, attach, step;
    struct ui_button reset;

    struct
    {
        struct ui_tabs ui;

        size_t len, cap;
        struct ui_mods_tab *list;
        struct ui_mods_tab **order;
    } tabs;

    struct {
        bool new;
        mod_id id; bool write;
        vm_ip ip;
    } request;
};

void ui_mods_alloc(struct ui_view_state *state)
{
    int16_t tree_w = (symbol_cap + 4) * ui_st.font.dim.w;
    int16_t code_w = (5 + ui_mods_cols + 2) * ui_st.font.dim.w;
    int16_t pad_w = ui_st.font.dim.w;

    struct ui_mods *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_mods) {
        .panel = ui_panel_title(make_dim(tree_w, ui_layout_inf), ui_str_c("mods")),
        .tree_w = tree_w,
        .code_w = code_w,
        .pad_w = pad_w,

        .new = ui_button_new(ui_str_c("+")),
        .new_val = ui_input_new(symbol_cap),
        .tree = ui_tree_new(make_dim(tree_w, ui_layout_inf), symbol_cap),

        .mode = ui_button_new(ui_str_c("code")),
        .build = ui_button_new(ui_str_c("build")),
        .publish = ui_button_new(ui_str_c("publish")),
        .import = ui_button_new(ui_str_c("import")),
        .export = ui_button_new(ui_str_c("export")),
        .load = ui_button_new(ui_str_c("load")),
        .attach = ui_button_new(ui_str_c("attach")),
        .step = ui_button_new(ui_str_c("step")),
        .reset = ui_button_new(ui_str_c("reset")),

        .tabs = { .ui = ui_tabs_new(symbol_cap + 4, true) },
        .request = { .ip = vm_ip_nil },
    };

    ui->mode.s.align = ui_align_center;
    ui->attach.s.align = ui_align_center;

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_mods,
        .slots = ui_slot_left,
        .panel = ui->panel,
        .fn = {
            .free = ui_mods_free,
            .update_frame = ui_mods_update,
            .event = ui_mods_event,
            .render = ui_mods_render,
        },
    };
}

static void ui_mods_free(void *state)
{
    struct ui_mods *ui = state;

    ui_panel_free(ui->panel);

    ui_button_free(&ui->new);
    ui_input_free(&ui->new_val);
    ui_tree_free(&ui->tree);

    ui_button_free(&ui->mode);
    ui_button_free(&ui->build);
    ui_button_free(&ui->publish);
    ui_button_free(&ui->import);
    ui_button_free(&ui->export);
    ui_button_free(&ui->load);
    ui_button_free(&ui->attach);
    ui_button_free(&ui->step);
    ui_button_free(&ui->reset);

    for (size_t i = 0; i < ui->tabs.cap; ++i) {
        struct ui_mods_tab *tab = ui->tabs.list + i;
        if (!tab->init) continue;
        ui_code_free(&tab->code);
        ui_asm_free(&tab->as);
        mod_free(tab->mod);
    }
    free(ui->tabs.list);
    free(ui->tabs.order);
    ui_tabs_free(&ui->tabs.ui);

    free(ui);
}


// -----------------------------------------------------------------------------
// tabs
// -----------------------------------------------------------------------------

static size_t ui_mods_tabs_len(struct ui_mods *ui)
{
    size_t len = 0;
    for (size_t i = 0; i < ui->tabs.len; ++i)
        len += !!ui->tabs.list[i].user;
    return len;
}

static struct ui_mods_tab *ui_mods_tabs_user(struct ui_mods *ui, uint64_t user)
{
    for (size_t i = 0; i < ui->tabs.len; ++i) {
        struct ui_mods_tab *tab = ui->tabs.list + i;
        if (user == tab->user) return tab;
    }
    return nullptr;
}

static struct ui_mods_tab *ui_mods_tabs_find(
        struct ui_mods *ui, mod_id id, bool write)
{
    return ui_mods_tabs_user(ui, vm_pack(id, write));
}

static struct ui_mods_tab *ui_mods_tabs_selected(struct ui_mods *ui)
{
    return ui_mods_tabs_user(ui, ui_tabs_selected(&ui->tabs.ui));
}

static struct ui_mods_tab *ui_mods_tabs_alloc(
        struct ui_mods *ui, mod_id id, bool write)
{
    struct ui_mods_tab *tab = nullptr;
    for (size_t i = 0; i < ui->tabs.len; ++i) {
        struct ui_mods_tab *it = ui->tabs.list + i;
        if (!it->user) { tab = it; break; }
    }

    if (!tab) {
        if (ui->tabs.len == ui->tabs.cap) {
            size_t old = legion_xchg(&ui->tabs.cap, ui->tabs.cap ? ui->tabs.cap * 2 : 8);
            ui->tabs.list = realloc_zero(
                    ui->tabs.list, old, ui->tabs.cap, sizeof(*ui->tabs.list));
            ui->tabs.order = realloc_zero(
                    ui->tabs.order, old, ui->tabs.cap, sizeof(*ui->tabs.order));
        }

        tab = ui->tabs.list + ui->tabs.len;
        ui->tabs.order[ui->tabs.len] = tab;
        ui->tabs.len++;
    }

    tab->id = id;
    tab->write = write;
    tab->user = vm_pack(id, write);

    tab->mode = ui_mods_code;
    tab->mod = nullptr;

    tab->debug.ip = vm_ip_nil;

    if (!tab->init) {
        tab->code = ui_code_new(make_dim(ui_layout_inf, ui_layout_inf));
        tab->as = ui_asm_new(make_dim(ui_layout_inf, ui_layout_inf));
    }
    else {
        ui_code_reset(&tab->code);
        ui_asm_reset(&tab->as);
    }

    tab->code.writable = tab->write;
    tab->init = true;
    return tab;
}

static void ui_mods_tabs_close(struct ui_mods *ui, uint64_t user)
{
    struct ui_mods_tab *tab = ui_mods_tabs_user(ui, user);
    assert(tab);

    if (tab->mod) mod_free(tab->mod);

    tab->user = 0;
    tab->id = 0;
    tab->mod = nullptr;
}

static void ui_mods_tabs_update(struct ui_mods *ui)
{
    int cmp(const void *l, const void *r)
    {
        const struct ui_mods_tab *const *lhs = l;
        const struct ui_mods_tab *const *rhs = r;
        return
            (*lhs)->user < (*rhs)->user ? -1 :
            (*lhs)->user > (*rhs)->user ? +1 : 0;
    }
    qsort(ui->tabs.order, ui->tabs.len, sizeof(*ui->tabs.order), &cmp);

    ui_tabs_reset(&ui->tabs.ui);
    for (size_t i = 0; i < ui->tabs.len; ++i) {
        struct ui_mods_tab *tab = ui->tabs.order[i];
        if (!tab->user || !tab->mod) continue;

        struct rgba fg = ui_st.rgba.code.read;
        if (tab->write)
            fg = ui_code_modified(&tab->code) ?
                ui_st.rgba.code.modified : ui_st.rgba.code.write;
        struct ui_str *str = ui_tabs_add_s(&ui->tabs.ui, tab->user, fg);

        struct symbol name = {0};
        bool ok = proxy_mod_name(mod_major(tab->id), &name);
        assert(ok);

        mod_ver version = mod_version(tab->id);
        if (!version) ui_str_setf(str, "%s", name.c);
        else ui_str_setf(str, "%s.%u", name.c, version);
    }
}

static bool ui_mods_tabs_set_write(
        struct ui_mods *ui, struct ui_mods_tab *tab, bool write)
{
    if (tab->write == write) return true;
    if (ui_mods_tabs_find(ui, tab->id, write)) return false;
    if (!write && ui_code_modified(&tab->code)) return false;

    tab->write = tab->code.writable = write;
    uint64_t old = legion_xchg(&tab->user, vm_pack(tab->id, tab->write));

    if (old == ui_tabs_selected(&ui->tabs.ui))
        ui_tabs_select(&ui->tabs.ui, tab->user);

    ui_mods_tabs_update(ui);
    return true;
}


// -----------------------------------------------------------------------------
// mode
// -----------------------------------------------------------------------------

static void ui_mods_mode_update(struct ui_mods *ui, struct ui_mods_tab *tab)
{
    switch (tab->mode)
    {

    case ui_mods_code: {
        ui_code_focus(&tab->code);
        ui_str_setc(&ui->mode.str, "asm");
        break;
    }

    case ui_mods_asm: {
        ui_asm_focus(&tab->as);
        ui_str_setc(&ui->mode.str, "code");
        break;
    }

    default: { assert(false); }
    }
}

static void ui_mods_mode_set(
        struct ui_mods *ui, struct ui_mods_tab *tab, enum ui_mods_mode mode)
{
    tab->mode = mode;
    ui_mods_mode_update(ui, tab);
}

static void ui_mods_mode_toggle(struct ui_mods *ui, struct ui_mods_tab *tab)
{
    switch (tab->mode)
    {

    case ui_mods_code: {
        ui_mods_mode_set(ui, tab, ui_mods_asm);
        ui_asm_goto(&tab->as, ui_code_ip(&tab->code));
        break;
    }

    case ui_mods_asm: {
        ui_mods_mode_set(ui, tab, ui_mods_code);
        ui_code_goto(&tab->code, ui_asm_ip(&tab->as));
        break;
    }

    default: { assert(false); }
    }
}

static void ui_mods_mode_goto(
        struct ui_mods *, struct ui_mods_tab *tab, vm_ip ip)
{
    if (ip == vm_ip_nil) return;

    switch (tab->mode)
    {
    case ui_mods_code: { ui_code_goto(&tab->code, ip); break; }
    case ui_mods_asm: { ui_asm_goto(&tab->as, ip); break; }
    default: { assert(false); }
    }
}

static void ui_mods_mode_debug(
        struct ui_mods *ui, struct ui_mods_tab *tab,
        bool debug, vm_ip ip, vm_ip bp)
{
    tab->debug.attached = debug;
    ui_str_setc(&ui->attach.str, debug ? "detach" : "attach");

    ui->attach.disabled = false;
    ui->step.disabled = !tab->debug.attached;
    if (debug && ip != tab->debug.ip) ui_mods_mode_goto(ui, tab, ip);
    tab->debug.ip = ip;

    ui_code_breakpoint(&tab->code, bp);
    ui_asm_breakpoint(&tab->as, bp);

    if (!debug) ui_mods_tabs_set_write(ui, tab, true);
}


// -----------------------------------------------------------------------------
// requests
// -----------------------------------------------------------------------------

static void ui_mods_request_reset(struct ui_mods *ui)
{
    ui->request.id = 0;
    ui->request.write = false;
    ui->request.new = false;
    ui->request.ip = vm_ip_nil;
}

static struct ui_mods_tab *ui_mods_select(
        struct ui_mods *ui, mod_id id, bool write)
{
    struct ui_mods_tab *tab = ui_mods_tabs_find(ui, id, write);
    if (!tab) return nullptr;

    ui_tree_select(&ui->tree, id);
    if (tab->user != ui_tabs_selected(&ui->tabs.ui)) {
        ui_tabs_select(&ui->tabs.ui, tab->user);
        ui_mods_mode_update(ui, tab);
    }

    return tab;
}

static void ui_mods_load(mod_id id, bool write, vm_ip ip)
{
    struct ui_mods *ui = ui_state(ui_view_mods);
    if (!mod_version(id)) id = proxy_mod_latest(mod_major(id));

    struct ui_mods_tab *tab = ui_mods_select(ui, id, write);

    if (!tab && (tab = ui_mods_tabs_find(ui, id, !write))) {
        if (!ui_mods_tabs_set_write(ui, tab, write))
            tab = nullptr;
    }

    if (tab) { ui_mods_mode_goto(ui, tab, ip); return; }

    ui->request.id = id;
    ui->request.write = write;
    ui->request.ip = ip;
    proxy_mod_select(id);
}

void ui_mods_show(mod_id id, vm_ip ip)
{
    if (!id) return;

    ui_mods_load(id, false, ip);
    ui_show(ui_view_mods);
}

void ui_mods_debug(mod_id id, bool debug, vm_ip ip, vm_ip bp)
{
    struct ui_mods *ui = ui_state(ui_view_mods);
    if (!ui_panel_is_visible(ui->panel)) return;

    struct ui_mods_tab *tab = ui_mods_tabs_selected(ui);
    if (!tab || tab->id != id) {
        ui->attach.disabled = true;
        ui_str_setc(&ui->attach.str, "attach");
        return;
    }

    ui_mods_mode_debug(ui, tab, debug, ip, bp);
}


// -----------------------------------------------------------------------------
// update
// -----------------------------------------------------------------------------

static void ui_mods_update_tree(struct ui_mods *ui)
{
    ui_tree_reset(&ui->tree);
    const struct mods_list *list = proxy_mods();
    for (size_t i = 0; i < list->len; ++i) {
        const struct mods_item *mod = list->items + i;

        uint64_t user = make_mod(mod->maj, 0);
        ui_node parent = ui_tree_index(&ui->tree);
        struct ui_str *str = ui_tree_add(&ui->tree, ui_node_nil, user);
        ui_str_set_symbol(str, &mod->str);

        for (mod_ver ver = mod->ver; ver > 0; --ver) {
            uint64_t user = make_mod(mod->maj, ver);
            struct ui_str *str = ui_tree_add(&ui->tree, parent, user);
            ui_str_setf(str, "%u", ver);
        }
    }
}

static void ui_mods_update_mod(struct ui_mods *ui)
{
    if (!ui->request.new && !ui->request.id) return;

    const struct mod *mod = proxy_mod();
    if (!mod) return;

    assert(ui->request.new || mod_major(mod->id) == mod_major(ui->request.id));

    struct ui_mods_tab *tab = ui->request.new ? nullptr :
        ui_mods_tabs_find(ui, ui->request.id, ui->request.write);

    if (!tab) tab = ui_mods_tabs_alloc(ui, mod->id, ui->request.write);

    if (tab->mod) mod_free(tab->mod);
    tab->mod = mod;

    tab->id = mod->id;
    tab->user = vm_pack(tab->id, tab->write);

    ui_code_set_mod(&tab->code, tab->mod);
    ui_asm_set_mod(&tab->as, tab->mod);
    ui_mods_select(ui, tab->id, tab->write);

    if (ui->request.ip) ui_mods_mode_goto(ui, tab, ui->request.ip);

    ui->reset.disabled = !tab->write;
    ui->build.disabled = !tab->write;
    ui->publish.disabled = !tab->write || mod->errs_len || mod_version(tab->id);

    ui_mods_request_reset(ui);
}

static void ui_mods_update(void *state)
{
    struct ui_mods *ui = state;

    ui_mods_update_tree(ui);
    ui_mods_update_mod(ui);
    ui_mods_tabs_update(ui);

    struct ui_mods_tab *tab = ui_mods_tabs_selected(ui);
    bool disabled =
        im_id_item(ui_item_selected()) != item_brain ||
        !tab || ui_code_modified(&tab->code);
    if (disabled) ui->attach.disabled = ui->step.disabled = true;
    ui->load.disabled = disabled;

    struct dim dim = make_dim(ui->tree_w, ui_layout_inf);
    if (ui_mods_tabs_len(ui)) dim.w += ui->pad_w + ui->code_w;
    ui_panel_resize(ui->panel, dim);
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static void ui_mods_new(struct ui_mods *ui)
{
    struct symbol name = {0};
    if (!ui_input_get_symbol(&ui->new_val, &name)) {
        ui_log(st_error, "invalid module name: '%s'", name.c);
        return;
    }

    ui->request.new = true;
    ui->request.write = true;
    proxy_mod_register(name);
}

static bool ui_mods_build(struct ui_mods *ui, struct ui_mods_tab *tab)
{
    if (!tab) { ui_log(st_error, "no mod selected"); return false; }
    if (!tab->mod) { ui_log(st_warn, "mod is loading"); return false; }
    if (!tab->write) { ui_log(st_error, "unable to build read-only mod"); return false; }

    size_t len = code_len(tab->code.code);
    char *buffer = calloc(len, sizeof(*buffer));
    (void) code_write(tab->code.code, buffer, len);

    ui->request.id = tab->id;
    ui->request.write = tab->write;
    proxy_mod_compile(mod_major(tab->id), buffer, len);

    free(buffer);
    return true;
}

static bool ui_mods_publish(struct ui_mods *ui, struct ui_mods_tab *tab)
{
    if (!tab) { ui_log(st_error, "no mod selected"); return false; }
    if (!tab->mod) { ui_log(st_warn, "mod is loading"); return false; }
    if (!tab->write) { ui_log(st_error, "unable to publish read-only mod"); return false; }
    if (tab->mod->errs_len) {
        ui_log(st_error, "unable to publish mod with errors");
        return false;
    }

    ui->request.id = tab->id;
    ui->request.write = tab->write;
    proxy_mod_publish(mod_major(tab->id));

    ui->publish.disabled = true;
    return true;
}

static bool ui_mods_import(struct ui_mods *, struct ui_mods_tab *tab)
{
    if (!tab) { ui_log(st_error, "no mod selected"); return false; }
    if (!tab->mod) { ui_log(st_warn, "mod is loading"); return false; }
    if (!tab->write) {
        ui_log(st_error, "unable to import into read-only mod");
        return false;
    }

    struct symbol name = {0};
    bool ok = proxy_mod_name(mod_major(tab->id), &name);
    assert(ok);

    char path[PATH_MAX] = {0};
    sys_path_mod(name.c, path, sizeof(path));

    if (!file_exists(path)) {
        ui_log(st_error,
                "unable to import mod '%s': '%s' doesn't exist", name.c, path);
    }

    struct mfile file = mfile_open(path);
    ui_code_set_text(&tab->code, file.ptr, file.len);
    mfile_close(&file);

    ui_log(st_info, "mod '%s' imported from '%s'", name.c, path);
    return true;
}

static bool ui_mods_export(struct ui_mods *, struct ui_mods_tab *tab)
{
    if (!tab) { ui_log(st_error, "no mod selected"); return false; }
    if (!tab->mod) { ui_log(st_warn, "mod is loading"); return false; }

    struct symbol name = {0};
    bool ok = proxy_mod_name(mod_major(tab->id), &name);
    assert(ok);

    char path[PATH_MAX] = {0};
    sys_path_mod(name.c, path, sizeof(path));

    struct mfilew file = mfilew_create_tmp(path, code_len(tab->code.code));
    (void) code_write(tab->code.code, file.ptr, file.len);
    mfilew_close(&file);

    file_tmp_swap(path);

    ui_log(st_info, "mod '%s' exported to '%s'", name.c, path);
    return true;
}

static bool ui_mods_reset(struct ui_mods *ui, struct ui_mods_tab *tab)
{
    if (!tab) { ui_log(st_error, "no mod selected"); return false; }
    if (!tab->mod) { ui_log(st_warn, "mod is loading"); return false; }
    if (!tab->write) { ui_log(st_error, "unable to reset read-only mod"); return false; }

    ui->request.id = tab->id;
    ui->request.write = tab->write;
    proxy_mod_select(tab->id);
    return true;
}

static bool ui_mods_shortcuts(struct ui_mods *ui, SDL_Event *ev)
{
    if (ui->panel->state != ui_panel_focused) return false;

    uint16_t mod = ev->key.keysym.mod;
    if (!(mod & KMOD_CTRL)) return false;

    struct ui_mods_tab *tab = ui_mods_tabs_selected(ui);

    switch (ev->key.keysym.sym)
    {
    case 'n': { ui_input_focus(&ui->new_val); return true; }
    case 'm': { if (tab) ui_mods_mode_toggle(ui, tab); return !!tab; }
    case 'b': { return ui_mods_build(ui, tab); }
    case 'p': { return ui_mods_publish(ui, tab); }
    case 'i': { return ui_mods_import(ui, tab); }
    case 'e': { return ui_mods_export(ui, tab); }
    case 's': { ui_item_io(io_dbg_step, item_brain, nullptr, 0); return true; }
    default: { return false; }
    }
}

static bool ui_mods_event(void *state, SDL_Event *ev)
{
    struct ui_mods *ui = state;

    if (render_user_event_is(ev, ev_state_load)) {
        ui->tabs.len = 0;
        ui_tabs_clear(&ui->tabs.ui);
        ui_tree_reset(&ui->tree);
        ui_mods_request_reset(ui);
    }

    if (ev->type == SDL_KEYDOWN && ui_mods_shortcuts(ui, ev))
        return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_input_event(&ui->new_val, ev))) {
        if (ret == ui_action) ui_mods_new(ui);
        return true;
    }

    if ((ret = ui_button_event(&ui->new, ev))) {
        if (ret != ui_action) return true;
        ui_mods_new(ui);
        return ret;
    }

    if ((ret = ui_tree_event(&ui->tree, ev))) {
        if (ret != ui_action) return true;
        ui_mods_load(ui->tree.selected, true, vm_ip_nil);
        return true;
    }

    if (!ui_mods_tabs_len(ui)) return false;

    if ((ret = ui_tabs_event(&ui->tabs.ui, ev))) {
        if (ret != ui_action) return true;

        uint64_t closed = ui_tabs_closed(&ui->tabs.ui);
        if (closed) ui_mods_tabs_close(ui, closed);

        struct ui_mods_tab *tab = ui_mods_tabs_selected(ui);
        if (tab) ui_tree_select(&ui->tree, tab->id);

        return true;
    }

    struct ui_mods_tab *tab = ui_mods_tabs_selected(ui);
    if (!tab) return false;

    if ((ret = ui_button_event(&ui->mode, ev))) {
        if (ret != ui_action) return true;
        ui_mods_mode_toggle(ui, tab);
        return true;
    }

    if ((ret = ui_button_event(&ui->build, ev))) {
        if (ret != ui_action) return true;
        ui_mods_build(ui, tab);
        return true;
    }

    if ((ret = ui_button_event(&ui->publish, ev))) {
        if (ret != ui_action) return true;
        ui_mods_publish(ui, tab);
        return true;
    }

    if ((ret = ui_button_event(&ui->import, ev))) {
        if (ret != ui_action) return true;
        ui_mods_import(ui, tab);
        return true;
    }

    if ((ret = ui_button_event(&ui->export, ev))) {
        if (ret != ui_action) return true;
        ui_mods_export(ui, tab);
        return true;
    }


    if ((ret = ui_button_event(&ui->load, ev))) {
        if (ret != ui_action) return true;
        vm_word args = tab->id;
        ui_item_io(io_mod, item_brain, &args, 1);
        return true;
    }

    if ((ret = ui_button_event(&ui->attach, ev))) {
        if (ret != ui_action) return true;
        enum io io = tab->debug.attached ? io_dbg_detach : io_dbg_attach;
        ui_item_io(io, item_brain, nullptr, 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->step, ev))) {
        if (ret != ui_action) return true;
        ui_item_io(io_dbg_step, item_brain, nullptr, 0);
        return true;
    }

    if ((ret = ui_button_event(&ui->reset, ev))) {
        if (ret != ui_action) return true;
        ui_mods_reset(ui, tab);
        return true;
    }

    switch (tab->mode)
    {
    case ui_mods_code: { if ((ret = ui_code_event(&tab->code, ev))) return true; }
    case ui_mods_asm: { if ((ret = ui_asm_event(&tab->as, ev))) return true; }
    }

    return false;
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void ui_mods_render(
        void *state, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_mods *ui = state;

    struct ui_layout inner = ui_layout_split_x(layout, ui->tree_w);
    {
        ui_input_render(&ui->new_val, &inner, renderer);
        ui_button_render(&ui->new, &inner, renderer);
        ui_layout_next_row(&inner);

        ui_layout_sep_row(&inner);

        ui_tree_render(&ui->tree, &inner, renderer);
    }

    if (!ui_mods_tabs_len(ui)) return;

    ui_tabs_render(&ui->tabs.ui, layout, renderer);
    ui_layout_next_row(layout);

    struct ui_mods_tab *tab = ui_mods_tabs_selected(ui);
    assert(tab);

    {
        ui_button_render(&ui->mode, layout, renderer);
        ui_layout_sep_col(layout);

        ui_button_render(&ui->build, layout, renderer);
        ui_button_render(&ui->publish, layout, renderer);

        ui_layout_sep_col(layout);

        ui_button_render(&ui->import, layout, renderer);
        ui_button_render(&ui->export, layout, renderer);

        ui_layout_sep_col(layout);
        ui_button_render(&ui->load, layout, renderer);
        ui_button_render(&ui->attach, layout, renderer);
        ui_button_render(&ui->step, layout, renderer);

        ui_layout_dir(layout, ui_layout_right_left);
        ui_button_render(&ui->reset, layout, renderer);

        ui_layout_dir(layout, ui_layout_left_right);
        ui_layout_next_row(layout);
        ui_layout_sep_row(layout);
    }

    switch (tab->mode)
    {
    case ui_mods_code: { ui_code_render(&tab->code, layout, renderer); break; }
    case ui_mods_asm: { ui_asm_render(&tab->as, layout, renderer); break; }
    }
}
