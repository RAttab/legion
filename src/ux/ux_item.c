/* ux_item.c
   Rémi Attab (remi.attab@gmail.com), 23 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_item_free(void *);
static void ux_item_update(void *);
static void ux_item_event(void *);
static void ux_item_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct ux_item
{
    im_id id;
    struct coord star;
    bool loading, io_visible;

    struct ui_panel *panel;
    struct ui_button io_toggle;
    struct ui_button help;
    struct ui_label id_lbl;
    struct ui_link id_val;
    struct ux_io *io;

    void *states[items_active_len];
};

static struct dim ux_item_dim(struct ux_item *ux)
{
    struct dim cell = engine_cell();

    unit width = 40 * cell.w;
    if (ux && ux->io_visible)
        width += cell.w + ux_io_width();

    return make_dim(width, ui_layout_inf);
}

void ux_item_alloc(struct ux_view_state *state)
{
    struct ux_item *ux = mem_alloc_t(ux);
    *ux = (struct ux_item) {
        .id = 0,
        .star = {0},
        .loading = false,
        .io_visible = false,

        .panel = ui_panel_title(ux_item_dim(ux), ui_str_c("item")),
        .io_toggle = ui_button_new(ui_str_c("<< io")),
        .help = ui_button_new(ui_str_c("?")),
        .id_lbl = ui_label_new(ui_str_c("id: ")),
        .id_val = ui_link_new(ui_str_v(im_id_str_len)),
        .io = ux_io_alloc(),
    };

    ui_panel_hide(ux->panel);
    ux->io_toggle.w.w = ui_layout_inf;

    for (size_t i = 0; i < items_active_len; ++i) {
        const struct im_config *config = im_config(items_active_first + i);
        if (config && config->ui.alloc) ux->states[i] = config->ui.alloc();
    }

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_item,
        .parent = ux_view_star,
        .slots = ux_slot_right_sub,
        .panel = ux->panel,
        .fn = {
            .free = ux_item_free,
            .update = ux_item_update,
            .event = ux_item_event,
            .render = ux_item_render,
        },
    };
}

void ux_item_free(void *state)
{
    struct ux_item *ux = state;

    ui_panel_free(ux->panel);
    ui_button_free(&ux->io_toggle);
    ui_button_free(&ux->help);
    ui_label_free(&ux->id_lbl);
    ui_link_free(&ux->id_val);
    ux_io_free(ux->io);

    for (size_t i = 0; i < items_active_len; ++i) {
        const struct im_config *config = im_config(items_active_first + i);
        if (config && config->ui.free) config->ui.free(ux->states[i]);
    }

    mem_free(ux);
}

im_id ux_item_selected(void)
{
    struct ux_item *ux = ux_state(ux_view_item);
    return ux->panel->visible ? ux->id : 0;
}

void ux_item_show(im_id id, struct coord star)
{
    struct ux_item *ux = ux_state(ux_view_item);

    ux->id = id;
    ux->star = star;

    if (!id) { ux_hide(ux_view_item); return; }

    ux_item_update(ux);
    ux_io_show(ux->io, ux->star, ux->id);

    ux_star_show(star);
    ux_show(ux_view_item);

    ev_set_select_item(star, id);
}

bool ux_item_io(enum io io, enum item item, const vm_word *args, size_t len)
{
    struct ux_item *ux = ux_state(ux_view_item);

    if (io <= io_min || io >= io_max) {
        ux_log(st_error,
                "invalid IO command: unknown IO value '%x'", io);
        return false;
    }

    if (!item || item >= items_max) {
        ux_log(st_error,
                "invalid IO command: unknown item value '%x'", item);
        return false;
    }

    if (!ux->panel->visible) {
        struct symbol io_sym = {0};
        bool ok = atoms_str(proxy_atoms(), io, &io_sym);
        assert(ok);

        ux_log(st_error,
                "unable to execute IO command '%s': no item selected",
                io_sym.c);
        return false;
    }

    if (item != im_id_item(ux->id)) {
        struct symbol io_sym = {0};
        bool ok = atoms_str(proxy_atoms(), io, &io_sym);
        assert(ok);

        struct symbol item_sym = {0};
        ok = atoms_str(proxy_atoms(), item, &item_sym);
        assert(ok);

        ux_log(st_error,
                "unable to execute IO command '%s': item selected is not of type '%s'",
                io_sym.c, item_sym.c);
        return false;
    }

    proxy_io(io, ux->id, args, len);
    return true;
}

static void *ux_item_state(struct ux_item *ux, im_id id)
{
    void *state =  ux->states[im_id_item(id) - items_active_first];
    assert(state);
    return state;
}

static void ux_item_update(void *state)
{
    struct ux_item *ux = state;
    if (!ux->id || coord_is_nil(ux->star)) return;

    ui_str_set_id(&ux->id_val.str, ux->id);

    struct chunk *chunk = proxy_chunk(ux->star);
    if ((ux->loading = !chunk)) return;

    if (!chunk_get(chunk, ux->id)) {
        ux_hide(ux_view_item);
        return;
    }

    const struct im_config *config = im_config_assert(im_id_item(ux->id));
    config->ui.update(ux_item_state(ux, ux->id), chunk, ux->id);
}

static void ux_item_event(void *state)
{
    struct ux_item *ux = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        if (!coord_eq(ux->star, ev->star))
            ux_hide(ux_view_item);

    if (ux->loading) return;

    if (ui_button_event(&ux->io_toggle)) {
        ux->io_visible = !ux->io_visible;
        ui_panel_resize(ux->panel, ux_item_dim(ux));
    }

    if (ui_button_event(&ux->help))
        ux_man_show_slot_path(ux_slot_left,
                "/items/%s", item_str_c(im_id_item(ux->id)));

    if (ui_link_event(&ux->id_val))
        ui_clipboard_copy_hex(ux->id);

    void *im_state = ux_item_state(ux, ux->id);
    const struct im_config *config = im_config_assert(im_id_item(ux->id));
    if (config->ui.event) config->ui.event(im_state);
    if (ux->io_visible) ux_io_event(ux->io);
}

static void ux_item_render(void *state, struct ui_layout *layout)
{
    struct ux_item *ux = state;
    if (ux->loading) return;

    if (ux->io_visible) {
        struct ui_layout inner = ui_layout_split_x(layout, ux_io_width());
        ux_io_render(ux->io, &inner);
        ui_layout_split_x(layout, engine_cell().w);
    }

    ui_layout_dir(layout, ui_layout_right_left);
    ui_button_render(&ux->help, layout);
    ui_layout_dir(layout, ui_layout_left_right);
    ui_button_render(&ux->io_toggle, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ux->id_lbl, layout);
    ui_link_render(&ux->id_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    const struct im_config *config = im_config_assert(im_id_item(ux->id));
    config->ui.render(ux_item_state(ux, ux->id), layout);
}
