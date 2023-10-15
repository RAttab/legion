/* ui_item.c
   Rémi Attab (remi.attab@gmail.com), 23 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ux/ui.h"
#include "game/chunk.h"
#include "items/config.h"

static void ui_item_free(void *);
static void ui_item_update(void *);
static void ui_item_event(void *);
static void ui_item_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// core
// -----------------------------------------------------------------------------

struct ui_item
{
    im_id id;
    struct coord star;
    bool loading, io_visible;

    struct ui_panel *panel;
    struct ui_button io_toggle;
    struct ui_button help;
    struct ui_label id_lbl;
    struct ui_link id_val;
    struct ui_io *io;

    void *states[items_active_len];
};

static struct dim ui_item_dim(struct ui_item *ui)
{
    struct dim cell = engine_cell();

    unit width = 40 * cell.w;
    if (ui && ui->io_visible)
        width += cell.w + ui_io_width();

    return make_dim(width, ui_layout_inf);
}

void ui_item_alloc(struct ui_view_state *state)
{
    struct ui_item *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_item) {
        .id = 0,
        .star = {0},
        .loading = false,
        .io_visible = false,

        .panel = ui_panel_title(ui_item_dim(ui), ui_str_c("item")),
        .io_toggle = ui_button_new(ui_str_c("<< io")),
        .help = ui_button_new(ui_str_c("?")),
        .id_lbl = ui_label_new(ui_str_c("id: ")),
        .id_val = ui_link_new(ui_str_v(im_id_str_len)),
        .io = ui_io_alloc(),
    };

    ui_panel_hide(ui->panel);
    ui->io_toggle.w.w = ui_layout_inf;

    for (size_t i = 0; i < items_active_len; ++i) {
        const struct im_config *config = im_config(items_active_first + i);
        if (config && config->ui.alloc) ui->states[i] = config->ui.alloc();
    }

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_item,
        .parent = ui_view_star,
        .slots = ui_slot_right_sub,
        .panel = ui->panel,
        .fn = {
            .free = ui_item_free,
            .update_frame = ui_item_update,
            .event = ui_item_event,
            .render = ui_item_render,
        },
    };
}

void ui_item_free(void *state)
{
    struct ui_item *ui = state;

    ui_panel_free(ui->panel);
    ui_button_free(&ui->io_toggle);
    ui_button_free(&ui->help);
    ui_label_free(&ui->id_lbl);
    ui_link_free(&ui->id_val);
    ui_io_free(ui->io);

    for (size_t i = 0; i < items_active_len; ++i) {
        const struct im_config *config = im_config(items_active_first + i);
        if (config && config->ui.free) config->ui.free(ui->states[i]);
    }

    free(ui);
}

im_id ui_item_selected(void)
{
    struct ui_item *ui = ui_state(ui_view_item);
    return ui->panel->visible ? ui->id : 0;
}

void ui_item_show(im_id id, struct coord star)
{
    struct ui_item *ui = ui_state(ui_view_item);

    ui->id = id;
    ui->star = star;

    if (!id) { ui_hide(ui_view_item); return; }

    ui_item_update(ui);
    ui_io_show(ui->io, ui->star, ui->id);

    ui_star_show(star);
    ui_show(ui_view_item);

    ev_set_select_item(star, id);
}

bool ui_item_io(enum io io, enum item item, const vm_word *args, size_t len)
{
    struct ui_item *ui = ui_state(ui_view_item);

    if (io <= io_min || io >= io_max) {
        ui_log(st_error,
                "invalid IO command: unknown IO value '%x'", io);
        return false;
    }

    if (!item || item >= items_max) {
        ui_log(st_error,
                "invalid IO command: unknown item value '%x'", item);
        return false;
    }

    if (!ui->panel->visible) {
        struct symbol io_sym = {0};
        bool ok = atoms_str(proxy_atoms(), io, &io_sym);
        assert(ok);

        ui_log(st_error,
                "unable to execute IO command '%s': no item selected",
                io_sym.c);
        return false;
    }

    if (item != im_id_item(ui->id)) {
        struct symbol io_sym = {0};
        bool ok = atoms_str(proxy_atoms(), io, &io_sym);
        assert(ok);

        struct symbol item_sym = {0};
        ok = atoms_str(proxy_atoms(), item, &item_sym);
        assert(ok);

        ui_log(st_error,
                "unable to execute IO command '%s': item selected is not of type '%s'",
                io_sym.c, item_sym.c);
        return false;
    }

    proxy_io(io, ui->id, args, len);
    return true;
}

static void *ui_item_state(struct ui_item *ui, im_id id)
{
    void *state =  ui->states[im_id_item(id) - items_active_first];
    assert(state);
    return state;
}

static void ui_item_update(void *state)
{
    struct ui_item *ui = state;
    if (!ui->id || coord_is_nil(ui->star)) return;

    ui_str_set_id(&ui->id_val.str, ui->id);

    struct chunk *chunk = proxy_chunk(ui->star);
    if ((ui->loading = !chunk)) return;

    if (!chunk_get(chunk, ui->id)) {
        ui_hide(ui_view_item);
        return;
    }

    const struct im_config *config = im_config_assert(im_id_item(ui->id));
    config->ui.update(ui_item_state(ui, ui->id), chunk, ui->id);
}

static void ui_item_event(void *state)
{
    struct ui_item *ui = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        if (!coord_eq(ui->star, ev->star))
            ui_hide(ui_view_item);

    if (ui->loading) return;

    if (ui_button_event(&ui->io_toggle)) {
        ui->io_visible = !ui->io_visible;
        ui_panel_resize(ui->panel, ui_item_dim(ui));
    }

    if (ui_button_event(&ui->help))
        ui_man_show_slot_path(ui_slot_left,
                "/items/%s", item_str_c(im_id_item(ui->id)));

    if (ui_link_event(&ui->id_val))
        ui_clipboard_copy_hex(ui->id);

    void *im_state = ui_item_state(ui, ui->id);
    const struct im_config *config = im_config_assert(im_id_item(ui->id));
    if (config->ui.event) config->ui.event(im_state);
    if (ui->io_visible) ui_io_event(ui->io);
}

static void ui_item_render(void *state, struct ui_layout *layout)
{
    struct ui_item *ui = state;
    if (ui->loading) return;

    if (ui->io_visible) {
        struct ui_layout inner = ui_layout_split_x(layout, ui_io_width());
        ui_io_render(ui->io, &inner);
        ui_layout_split_x(layout, engine_cell().w);
    }

    ui_layout_dir(layout, ui_layout_right_left);
    ui_button_render(&ui->help, layout);
    ui_layout_dir(layout, ui_layout_left_right);
    ui_button_render(&ui->io_toggle, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->id_lbl, layout);
    ui_link_render(&ui->id_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    const struct im_config *config = im_config_assert(im_id_item(ui->id));
    config->ui.render(ui_item_state(ui, ui->id), layout);
}
