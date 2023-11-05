/* ux_mods.c
   Rémi Attab (remi.attab@gmail.com), 18 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_mods_free(void *);
static void ux_mods_update(void *);
static void ux_mods_event(void *);
static void ux_mods_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

constexpr uint32_t ux_mods_cols = 80;
enum ux_mods_mode { ux_mods_code = 0, ux_mods_asm = 1 };

struct ux_mods_tab
{
    uint64_t user;
    bool init, write;

    mod_id id;
    const struct mod *mod;

    struct { bool loaded; bool attached; vm_ip ip; } debug;

    enum ux_mods_mode mode;
    struct ui_code code;
    struct ui_asm as;
};

struct ux_mods
{
    struct ui_panel *panel;
    unit tree_w, code_w, pad_w;

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
        struct ux_mods_tab *list;
        struct ux_mods_tab **order;
    } tabs;

    struct {
        bool new;
        mod_id id; bool write;
        vm_ip ip;
    } request;
};

void ux_mods_alloc(struct ux_view_state *state)
{
    struct dim cell = engine_cell();
    unit tree_w = (symbol_cap + 4) * cell.w;
    unit code_w = (5 + ux_mods_cols + 2) * cell.w;
    unit pad_w = cell.w;

    struct ux_mods *ux = calloc(1, sizeof(*ux));
    *ux = (struct ux_mods) {
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

    ux->mode.s.align = ui_align_center;
    ux->attach.s.align = ui_align_center;

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_mods,
        .slots = ux_slot_left,
        .panel = ux->panel,
        .fn = {
            .free = ux_mods_free,
            .update_frame = ux_mods_update,
            .event = ux_mods_event,
            .render = ux_mods_render,
        },
    };
}

static void ux_mods_free(void *state)
{
    struct ux_mods *ux = state;

    ui_panel_free(ux->panel);

    ui_button_free(&ux->new);
    ui_input_free(&ux->new_val);
    ui_tree_free(&ux->tree);

    ui_button_free(&ux->mode);
    ui_button_free(&ux->build);
    ui_button_free(&ux->publish);
    ui_button_free(&ux->import);
    ui_button_free(&ux->export);
    ui_button_free(&ux->load);
    ui_button_free(&ux->attach);
    ui_button_free(&ux->step);
    ui_button_free(&ux->reset);

    for (size_t i = 0; i < ux->tabs.cap; ++i) {
        struct ux_mods_tab *tab = ux->tabs.list + i;
        if (!tab->init) continue;
        ui_code_free(&tab->code);
        ui_asm_free(&tab->as);
        mod_free(tab->mod);
    }
    free(ux->tabs.list);
    free(ux->tabs.order);
    ui_tabs_free(&ux->tabs.ui);

    free(ux);
}


// -----------------------------------------------------------------------------
// tabs
// -----------------------------------------------------------------------------

static size_t ux_mods_tabs_len(struct ux_mods *ux)
{
    size_t len = 0;
    for (size_t i = 0; i < ux->tabs.len; ++i)
        len += !!ux->tabs.list[i].user;
    return len;
}

static struct ux_mods_tab *ux_mods_tabs_user(struct ux_mods *ux, uint64_t user)
{
    for (size_t i = 0; i < ux->tabs.len; ++i) {
        struct ux_mods_tab *tab = ux->tabs.list + i;
        if (user == tab->user) return tab;
    }
    return nullptr;
}

static struct ux_mods_tab *ux_mods_tabs_find(
        struct ux_mods *ux, mod_id id, bool write)
{
    return ux_mods_tabs_user(ux, vm_pack(id, write));
}

static struct ux_mods_tab *ux_mods_tabs_selected(struct ux_mods *ux)
{
    return ux_mods_tabs_user(ux, ui_tabs_selected(&ux->tabs.ui));
}

static struct ux_mods_tab *ux_mods_tabs_alloc(
        struct ux_mods *ux, mod_id id, bool write)
{
    struct ux_mods_tab *tab = nullptr;
    for (size_t i = 0; i < ux->tabs.len; ++i) {
        struct ux_mods_tab *it = ux->tabs.list + i;
        if (!it->user) { tab = it; break; }
    }

    if (!tab) {
        if (ux->tabs.len == ux->tabs.cap) {
            size_t old = legion_xchg(&ux->tabs.cap, ux->tabs.cap ? ux->tabs.cap * 2 : 8);
            ux->tabs.list = realloc_zero(
                    ux->tabs.list, old, ux->tabs.cap, sizeof(*ux->tabs.list));
            ux->tabs.order = realloc_zero(
                    ux->tabs.order, old, ux->tabs.cap, sizeof(*ux->tabs.order));
        }

        tab = ux->tabs.list + ux->tabs.len;
        ux->tabs.order[ux->tabs.len] = tab;
        ux->tabs.len++;
    }

    tab->id = id;
    tab->write = write;
    tab->user = vm_pack(id, write);

    tab->mode = ux_mods_code;
    tab->mod = nullptr;

    tab->debug.ip = vm_ip_nil;

    if (!tab->init) {
        tab->code = ui_code_new(make_dim(ui_layout_inf, ui_layout_inf));
        tab->as = ui_asm_new(make_dim(ui_layout_inf, ui_layout_inf));
        tab->code.p = tab->as.p = ux->panel;
    }
    else {
        ui_code_reset(&tab->code);
        ui_asm_reset(&tab->as);
    }

    tab->code.writable = tab->write;
    tab->init = true;
    return tab;
}

static void ux_mods_tabs_close(struct ux_mods *ux, uint64_t user)
{
    struct ux_mods_tab *tab = ux_mods_tabs_user(ux, user);
    assert(tab);

    if (tab->mod) mod_free(tab->mod);

    tab->user = 0;
    tab->id = 0;
    tab->mod = nullptr;
}

static void ux_mods_tabs_update(struct ux_mods *ux)
{
    int cmp(const void *l, const void *r)
    {
        const struct ux_mods_tab *const *lhs = l;
        const struct ux_mods_tab *const *rhs = r;
        return
            (*lhs)->user < (*rhs)->user ? -1 :
            (*lhs)->user > (*rhs)->user ? +1 : 0;
    }
    qsort(ux->tabs.order, ux->tabs.len, sizeof(*ux->tabs.order), &cmp);

    ui_tabs_reset(&ux->tabs.ui);
    for (size_t i = 0; i < ux->tabs.len; ++i) {
        struct ux_mods_tab *tab = ux->tabs.order[i];
        if (!tab->user || !tab->mod) continue;

        struct rgba fg = ui_st.rgba.code.read;
        if (tab->write)
            fg = ui_code_modified(&tab->code) ?
                ui_st.rgba.code.modified : ui_st.rgba.code.write;
        struct ui_str *str = ui_tabs_add_s(&ux->tabs.ui, tab->user, fg);

        struct symbol name = {0};
        bool ok = proxy_mod_name(mod_major(tab->id), &name);
        assert(ok);

        mod_ver version = mod_version(tab->id);
        if (!version) ui_str_setf(str, "%s", name.c);
        else ui_str_setf(str, "%s.%u", name.c, version);
    }
}

static bool ux_mods_tabs_set_write(
        struct ux_mods *ux, struct ux_mods_tab *tab, bool write)
{
    if (tab->write == write) return true;
    if (ux_mods_tabs_find(ux, tab->id, write)) return false;
    if (!write && ui_code_modified(&tab->code)) return false;

    tab->write = tab->code.writable = write;
    uint64_t old = legion_xchg(&tab->user, vm_pack(tab->id, tab->write));

    if (old == ui_tabs_selected(&ux->tabs.ui))
        ui_tabs_select(&ux->tabs.ui, tab->user);

    ux_mods_tabs_update(ux);
    return true;
}


// -----------------------------------------------------------------------------
// mode
// -----------------------------------------------------------------------------

static void ux_mods_mode_update(struct ux_mods *ux, struct ux_mods_tab *tab)
{
    switch (tab->mode)
    {

    case ux_mods_code: {
        ui_code_focus(&tab->code);
        ui_str_setc(&ux->mode.str, "asm");
        break;
    }

    case ux_mods_asm: {
        ui_asm_focus(&tab->as);
        ui_str_setc(&ux->mode.str, "code");
        break;
    }

    default: { assert(false); }
    }
}

static void ux_mods_mode_set(
        struct ux_mods *ux, struct ux_mods_tab *tab, enum ux_mods_mode mode)
{
    tab->mode = mode;
    ux_mods_mode_update(ux, tab);
}

static void ux_mods_mode_toggle(struct ux_mods *ux, struct ux_mods_tab *tab)
{
    switch (tab->mode)
    {

    case ux_mods_code: {
        ux_mods_mode_set(ux, tab, ux_mods_asm);
        ui_asm_goto(&tab->as, ui_code_ip(&tab->code));
        break;
    }

    case ux_mods_asm: {
        ux_mods_mode_set(ux, tab, ux_mods_code);
        ui_code_goto(&tab->code, ui_asm_ip(&tab->as));
        break;
    }

    default: { assert(false); }
    }
}

static void ux_mods_mode_goto(
        struct ux_mods *, struct ux_mods_tab *tab, vm_ip ip)
{
    if (ip == vm_ip_nil) return;

    switch (tab->mode)
    {
    case ux_mods_code: { ui_code_goto(&tab->code, ip); break; }
    case ux_mods_asm: { ui_asm_goto(&tab->as, ip); break; }
    default: { assert(false); }
    }
}

static void ux_mods_mode_debug(
        struct ux_mods *ux, struct ux_mods_tab *tab,
        bool debug, vm_ip ip, vm_ip bp)
{
    tab->debug.attached = debug;
    ui_str_setc(&ux->attach.str, debug ? "detach" : "attach");

    ux->attach.disabled = false;
    ux->step.disabled = !tab->debug.attached;
    if (debug && ip != tab->debug.ip) ux_mods_mode_goto(ux, tab, ip);
    tab->debug.ip = ip;

    ui_code_breakpoint(&tab->code, bp);
    ui_asm_breakpoint(&tab->as, bp);

    if (!debug && bp == vm_ip_nil)
        ux_mods_tabs_set_write(ux, tab, true);
}


// -----------------------------------------------------------------------------
// requests
// -----------------------------------------------------------------------------

static void ux_mods_request_reset(struct ux_mods *ux)
{
    ux->request.id = 0;
    ux->request.write = false;
    ux->request.new = false;
    ux->request.ip = vm_ip_nil;
}

static struct ux_mods_tab *ux_mods_select(
        struct ux_mods *ux, mod_id id, bool write)
{
    struct ux_mods_tab *tab = ux_mods_tabs_find(ux, id, write);
    if (!tab) return nullptr;

    ui_tree_select(&ux->tree, id);
    if (tab->user != ui_tabs_selected(&ux->tabs.ui)) {
        ui_tabs_select(&ux->tabs.ui, tab->user);
        ux_mods_mode_update(ux, tab);
    }

    return tab;
}

static void ux_mods_load(mod_id id, bool write, vm_ip ip)
{
    struct ux_mods *ux = ux_state(ux_view_mods);
    if (!mod_version(id)) id = proxy_mod_latest(mod_major(id));

    struct ux_mods_tab *tab = ux_mods_select(ux, id, write);

    if (!tab && (tab = ux_mods_tabs_find(ux, id, !write))) {
        if (!ux_mods_tabs_set_write(ux, tab, write))
            tab = nullptr;
    }

    if (tab) { ux_mods_mode_goto(ux, tab, ip); return; }

    ux->request.id = id;
    ux->request.write = write;
    ux->request.ip = ip;
    proxy_mod_select(id);
}

void ux_mods_show(mod_id id, vm_ip ip)
{
    if (!id) return;

    ux_mods_load(id, false, ip);
    ux_show(ux_view_mods);
}

void ux_mods_debug(mod_id id, bool debug, vm_ip ip, vm_ip bp)
{
    struct ux_mods *ux = ux_state(ux_view_mods);
    if (!ux->panel->visible) return;

    struct ux_mods_tab *tab = ux_mods_tabs_selected(ux);
    if (!tab || tab->id != id) {
        ux->attach.disabled = true;
        ui_str_setc(&ux->attach.str, "attach");
        return;
    }

    ux_mods_mode_debug(ux, tab, debug, ip, bp);
}


// -----------------------------------------------------------------------------
// update
// -----------------------------------------------------------------------------

static void ux_mods_update_tree(struct ux_mods *ux)
{
    ui_tree_reset(&ux->tree);
    const struct mods_list *list = proxy_mods();
    for (size_t i = 0; i < list->len; ++i) {
        const struct mods_item *mod = list->items + i;

        uint64_t user = make_mod(mod->maj, 0);
        ui_tree_node parent = ui_tree_index(&ux->tree);
        struct ui_str *str = ui_tree_add(&ux->tree, ui_tree_node_nil, user);
        ui_str_set_symbol(str, &mod->str);

        for (mod_ver ver = mod->ver; ver > 0; --ver) {
            uint64_t user = make_mod(mod->maj, ver);
            struct ui_str *str = ui_tree_add(&ux->tree, parent, user);
            ui_str_setf(str, "%u", ver);
        }
    }
}

static void ux_mods_update_mod(struct ux_mods *ux)
{
    if (!ux->request.new && !ux->request.id) return;

    const struct mod *mod = proxy_mod();
    if (!mod) return;

    assert(ux->request.new || mod_major(mod->id) == mod_major(ux->request.id));

    struct ux_mods_tab *tab = ux->request.new ? nullptr :
        ux_mods_tabs_find(ux, ux->request.id, ux->request.write);

    if (!tab) tab = ux_mods_tabs_alloc(ux, mod->id, ux->request.write);

    if (tab->mod) mod_free(tab->mod);
    tab->mod = mod;

    tab->id = mod->id;
    tab->user = vm_pack(tab->id, tab->write);

    ui_code_set_mod(&tab->code, tab->mod);
    ui_asm_set_mod(&tab->as, tab->mod);
    ux_mods_select(ux, tab->id, tab->write);

    if (ux->request.ip) ux_mods_mode_goto(ux, tab, ux->request.ip);

    ux->reset.disabled = !tab->write;
    ux->build.disabled = !tab->write;
    ux->publish.disabled = !tab->write || mod->errs_len || mod_version(tab->id);

    ux_mods_request_reset(ux);
}

static void ux_mods_update(void *state)
{
    struct ux_mods *ux = state;

    ux_mods_update_tree(ux);
    ux_mods_update_mod(ux);
    ux_mods_tabs_update(ux);

    struct ux_mods_tab *tab = ux_mods_tabs_selected(ux);
    bool disabled =
        im_id_item(ux_item_selected()) != item_brain ||
        !tab || ui_code_modified(&tab->code);
    if (disabled) ux->attach.disabled = ux->step.disabled = true;
    ux->load.disabled = disabled;

    struct dim dim = make_dim(ux->tree_w, ui_layout_inf);
    if (ux_mods_tabs_len(ux)) dim.w += ux->pad_w + ux->code_w;
    ui_panel_resize(ux->panel, dim);
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static void ux_mods_new(struct ux_mods *ux)
{
    struct symbol name = {0};
    if (!ui_input_get_symbol(&ux->new_val, &name)) {
        ux_log(st_error, "invalid module name: '%s'", name.c);
        return;
    }

    ux->request.new = true;
    ux->request.write = true;
    proxy_mod_register(name);
}

static void ux_mods_build(struct ux_mods *ux, struct ux_mods_tab *tab)
{
    if (!tab) { ux_log(st_error, "no mod selected"); return; }
    if (!tab->mod) { ux_log(st_warn, "mod is loading"); return; }
    if (!tab->write) { ux_log(st_error, "unable to buxld read-only mod"); return; }

    size_t len = code_len(tab->code.code);
    char *buffer = calloc(len, sizeof(*buffer));
    (void) code_write(tab->code.code, buffer, len);

    ux->request.id = tab->id;
    ux->request.write = tab->write;
    proxy_mod_compile(mod_major(tab->id), buffer, len);

    free(buffer);
}

static void ux_mods_publish(struct ux_mods *ux, struct ux_mods_tab *tab)
{
    if (!tab) { ux_log(st_error, "no mod selected"); return; }
    if (!tab->mod) { ux_log(st_warn, "mod is loading"); return; }
    if (!tab->write) { ux_log(st_error, "unable to publish read-only mod"); return; }
    if (tab->mod->errs_len) {ux_log(st_error, "unable to publish mod with errors"); return; }

    ux->request.id = tab->id;
    ux->request.write = tab->write;
    proxy_mod_publish(mod_major(tab->id));

    ux->publish.disabled = true;
}

static void ux_mods_import(struct ux_mods *, struct ux_mods_tab *tab)
{
    if (!tab) { ux_log(st_error, "no mod selected"); return; }
    if (!tab->mod) { ux_log(st_warn, "mod is loading"); return; }
    if (!tab->write) {ux_log(st_error, "unable to import into read-only mod"); return; }

    struct symbol name = {0};
    bool ok = proxy_mod_name(mod_major(tab->id), &name);
    assert(ok);

    char path[PATH_MAX] = {0};
    engine_path_mod(name.c, path, sizeof(path));

    if (!file_exists(path)) {
        ux_log(st_error,
                "unable to import mod '%s': '%s' doesn't exist", name.c, path);
        return;
    }

    struct mfile file = mfile_open(path);
    ui_code_set_text(&tab->code, file.ptr, file.len);
    mfile_close(&file);

    ux_log(st_info, "mod '%s' imported from '%s'", name.c, path);
}

static void ux_mods_export(struct ux_mods *, struct ux_mods_tab *tab)
{
    if (!tab) { ux_log(st_error, "no mod selected"); return; }
    if (!tab->mod) { ux_log(st_warn, "mod is loading"); return; }

    struct symbol name = {0};
    bool ok = proxy_mod_name(mod_major(tab->id), &name);
    assert(ok);

    char path[PATH_MAX] = {0};
    engine_path_mod(name.c, path, sizeof(path));

    struct mfilew file = mfilew_create_tmp(path, code_len(tab->code.code));
    (void) code_write(tab->code.code, file.ptr, file.len);
    mfilew_close(&file);

    file_tmp_swap(path);

    ux_log(st_info, "mod '%s' exported to '%s'", name.c, path);
}

static void ux_mods_reset(struct ux_mods *ux, struct ux_mods_tab *tab)
{
    if (!tab) { ux_log(st_error, "no mod selected"); return; }
    if (!tab->mod) { ux_log(st_warn, "mod is loading"); return; }
    if (!tab->write) { ux_log(st_error, "unable to reset read-only mod"); return; }

    ux->request.id = tab->id;
    ux->request.write = tab->write;
    proxy_mod_select(tab->id);
}

static bool ux_mods_shortcuts(struct ux_mods *ux, const struct ev_key *ev)
{
    if (ui_focus_panel() != ux->panel) return false;
    if (ev->state != ev_state_down) return false;
    if (ev->mods != ev_mods_ctrl) return false;

    struct ux_mods_tab *tab = ux_mods_tabs_selected(ux);

    switch (ev->c)
    {
    case 'n': { ui_input_focus(&ux->new_val); return true; }
    case 'm': { if (tab) ux_mods_mode_toggle(ux, tab); return true; }
    case 'b': { ux_mods_build(ux, tab); return true; }
    case 'p': { ux_mods_publish(ux, tab); return true; }
    case 'i': { ux_mods_import(ux, tab); return true; }
    case 'e': { ux_mods_export(ux, tab); return true; }
    case 's': { ux_item_io(io_dbg_step, item_brain, nullptr, 0); return true; }
    default: { return false; }
    }
}

static void ux_mods_event(void *state)
{
    struct ux_mods *ux = state;

    for (auto ev = ev_load(); ev; ev = nullptr) {
        ux->tabs.len = 0;
        ui_tabs_clear(&ux->tabs.ui);
        ui_tree_reset(&ux->tree);
        ux_mods_request_reset(ux);
    }

    for (auto ev = ev_mod_break(); ev; ev = nullptr) {
        struct ux_mods_tab *tab = ux_mods_select(ux, ev->mod, false);
        if (!tab) ux_mods_load(ev->mod, false, ev->ip);
    }

    for (auto ev = ev_next_key(nullptr); ev; ev = ev_next_key(ev))
        if (ux_mods_shortcuts(ux, ev)) ev_consume_key(ev);

    if (ui_input_event(&ux->new_val)) ux_mods_new(ux);
    if (ui_button_event(&ux->new)) ux_mods_new(ux);
    if (ui_tree_event(&ux->tree))
        ux_mods_load(ux->tree.selected, true, vm_ip_nil);

    if (!ux_mods_tabs_len(ux)) return;

    switch (ui_tabs_event(&ux->tabs.ui))
    {
    case ui_tabs_ev_close: {
        ux_mods_tabs_close(ux, ui_tabs_closed(&ux->tabs.ui));
        break;
    }
    case ui_tabs_ev_select: {
        struct ux_mods_tab *tab = ux_mods_tabs_selected(ux);
        if (tab) {
            ui_tree_select(&ux->tree, tab->id);
            ux_mods_mode_update(ux, tab);
        }
        break;
    }
    default: { break; }
    }

    struct ux_mods_tab *tab = ux_mods_tabs_selected(ux);
    if (!tab) return;

    if (ui_button_event(&ux->mode)) ux_mods_mode_toggle(ux, tab);
    if (ui_button_event(&ux->build)) ux_mods_build(ux, tab);
    if (ui_button_event(&ux->publish)) ux_mods_publish(ux, tab);
    if (ui_button_event(&ux->import)) ux_mods_import(ux, tab);
    if (ui_button_event(&ux->export)) ux_mods_export(ux, tab);
    if (ui_button_event(&ux->reset)) ux_mods_reset(ux, tab);
    
    if (ui_button_event(&ux->load)) {
        vm_word args = tab->id;
        ux_item_io(io_mod, item_brain, &args, 1);
    }

    if (ui_button_event(&ux->attach)) {
        enum io io = tab->debug.attached ? io_dbg_detach : io_dbg_attach;
        ux_item_io(io, item_brain, nullptr, 0);
    }

    if (ui_button_event(&ux->step))
        ux_item_io(io_dbg_step, item_brain, nullptr, 0);

    switch (tab->mode)
    {
    case ux_mods_code: { ui_code_event(&tab->code); }
    case ux_mods_asm: { ui_asm_event(&tab->as); }
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void ux_mods_render(void *state, struct ui_layout *layout)
{
    struct ux_mods *ux = state;

    struct ui_layout inner = ui_layout_split_x(layout, ux->tree_w);
    {
        ui_input_render(&ux->new_val, &inner);
        ui_button_render(&ux->new, &inner);
        ui_layout_next_row(&inner);

        ui_layout_sep_row(&inner);

        ui_tree_render(&ux->tree, &inner);
    }

    if (!ux_mods_tabs_len(ux)) return;

    ui_tabs_render(&ux->tabs.ui, layout);
    ui_layout_next_row(layout);

    struct ux_mods_tab *tab = ux_mods_tabs_selected(ux);
    assert(tab);

    {
        ui_button_render(&ux->mode, layout);
        ui_layout_sep_col(layout);

        ui_button_render(&ux->build, layout);
        ui_button_render(&ux->publish, layout);

        ui_layout_sep_col(layout);
        ui_button_render(&ux->reset, layout);

        ui_layout_sep_col(layout);
        ui_button_render(&ux->load, layout);
        ui_button_render(&ux->attach, layout);
        ui_button_render(&ux->step, layout);

        ui_layout_dir(layout, ui_layout_right_left);
        ui_button_render(&ux->export, layout);
        ui_button_render(&ux->import, layout);

        ui_layout_dir(layout, ui_layout_left_right);
        ui_layout_next_row(layout);
        ui_layout_sep_row(layout);
    }

    switch (tab->mode)
    {
    case ux_mods_code: { ui_code_render(&tab->code, layout); break; }
    case ux_mods_asm: { ui_asm_render(&tab->as, layout); break; }
    }
}
