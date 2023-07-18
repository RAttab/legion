/* ui_item.c
   Rémi Attab (remi.attab@gmail.com), 23 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"
#include "items/config.h"

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
    int16_t height = render.rect.h - ui_topbar_height() - ui_status_height();

    int16_t width = 42 * ui_st.font.dim.w;
    if (ui && ui->io_visible)
        width += ui_st.font.dim.w + ui_io_width();

    return make_dim(width, height);
}

static struct pos ui_item_pos(struct ui_item *ui)
{
    struct dim dim = ui_item_dim(ui);
    return make_pos(
            render.rect.w - dim.w - ui_star_width(render.ui.star),
            ui_topbar_height());
}

struct ui_item *ui_item_new(void)
{
    struct ui_item *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_item) {
        .id = 0,
        .star = {0},
        .loading = false,
        .io_visible = false,

        .panel = ui_panel_title(ui_item_pos(ui), ui_item_dim(ui), ui_str_c("item")),
        .io_toggle = ui_button_new(ui_str_c("<< io")),
        .help = ui_button_new(ui_str_c("?")),
        .id_lbl = ui_label_new(ui_str_c("id: ")),
        .id_val = ui_link_new(ui_str_v(im_id_str_len)),
        .io = ui_io_new(),
    };

    ui_panel_hide(ui->panel);
    ui->io_toggle.w.dim.w = ui_layout_inf;

    for (size_t i = 0; i < items_active_len; ++i) {
        const struct im_config *config = im_config(items_active_first + i);
        if (config && config->ui.alloc) ui->states[i] = config->ui.alloc();
    }

    return ui;
}

void ui_item_free(struct ui_item *ui)
{
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

int16_t ui_item_width(struct ui_item *ui)
{
    return ui->panel->w.dim.w;
}

static void *ui_item_state(struct ui_item *ui, im_id id)
{
    void *state =  ui->states[im_id_item(id) - items_active_first];
    assert(state);
    return state;
}

static void ui_item_update(struct ui_item *ui)
{
    if (!ui->id || coord_is_nil(ui->star)) return;

    ui_str_set_id(&ui->id_val.str, ui->id);

    struct chunk *chunk = proxy_chunk(render.proxy, ui->star);
    if ((ui->loading = !chunk)) return;

    if (!chunk_get(chunk, ui->id)) {
        render_push_event(EV_ITEM_CLEAR, 0, 0);
        return;
    }

    const struct im_config *config = im_config_assert(im_id_item(ui->id));
    config->ui.update(ui_item_state(ui, ui->id), chunk, ui->id);
}

static void ui_item_event_io(struct ui_item *ui, uintptr_t a1, uintptr_t a2)
{
    uint32_t io_raw = 0, item_raw = 0;
    vm_unpack(a1, &io_raw, &item_raw);

    enum io io = io_raw;
    if (io_raw <= io_min || io_raw >= io_max) {
        render_log(st_error, "invalid IO command: io out-of-bounds '%x'", io_raw);
        return;
    }

    enum item item = item_raw;
    if (!item_raw || item_raw >= items_max) {
        render_log(st_error, "invalid IO command: item out-of-bounds '%x'", item_raw);
        return;
    }

    if (!ui->id) {
        struct symbol io_str = {0};
        atoms_str(proxy_atoms(render.proxy), io, &io_str);

        render_log(st_error, "unable to execute IO command '%s': no item is selected",
                io_str.c);
        return;
    }

    if (item != im_id_item(ui->id)) {
        struct symbol io_str = {0};
        atoms_str(proxy_atoms(render.proxy), io, &io_str);

        struct symbol exp_str = {0};
        atoms_str(proxy_atoms(render.proxy), item, &exp_str);

        render_log(st_error, "unable to execute IO command '%s': item selected is not of type '%s'",
                io_str.c, exp_str.c);
        return;
    }

    vm_word arg = (uintptr_t) a2;
    proxy_io(render.proxy, io, ui->id, &arg, 1);
}

static void ui_item_reset(struct ui_item *ui)
{
    ui->id = 0;
    ui->star = coord_nil();
    ui_io_clear(ui->io);
    ui_panel_hide(ui->panel);
}

static bool ui_item_event_user(struct ui_item *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD:
    case EV_MAN_GOTO:
    case EV_MAN_TOGGLE:
    case EV_PILLS_TOGGLE:
    case EV_WORKER_TOGGLE:
    case EV_ENERGY_TOGGLE:
    case EV_STAR_CLEAR:
    case EV_ITEM_CLEAR: {
        ui_item_reset(ui);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(ui->panel)) return false;
        ui_item_update(ui);
        return false;
    }

    case EV_STAR_SELECT: {
        struct coord new = coord_from_u64((uintptr_t) ev->user.data1);
        if (!coord_eq(ui->star, new))
            ui_item_reset(ui);
        return false;
    }

    case EV_ITEM_SELECT: {
        ui->id = (uintptr_t) ev->user.data1;
        ui->star = coord_from_u64((uintptr_t) ev->user.data2);
        ui_item_update(ui);
        ui_io_select(ui->io, ui->star, ui->id);
        ui_panel_show(ui->panel);
        return false;
    }

    case EV_IO: {
        ui_item_event_io(ui,
                (uintptr_t) ev->user.data1,
                (uintptr_t) ev->user.data2);
        return true;
    }

    default: { return false; }
    }
}

void ui_item_event_help(struct ui_item *ui)
{
    char path[man_path_max] = {0};
    size_t len = snprintf(path, sizeof(path),
            "/items/%s", item_str_c(im_id_item(ui->id)));

    struct link link = man_link(path, len);
    if (link_is_nil(link)) {
        render_log(st_error, "unable to open link to '%s'", path);
        return;
    }

    render_push_event(EV_MAN_GOTO, link_to_u64(link), 0);
}

bool ui_item_event(struct ui_item *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_item_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(ui->panel, ev))) {
        if (ret == ui_consume && !ui_panel_is_visible(ui->panel))
            render_push_event(EV_ITEM_CLEAR, 0, 0);
        return ret != ui_skip;
    }

    if (ui->loading) return ui_panel_event_consume(ui->panel, ev);

    if ((ret = ui_button_event(&ui->io_toggle, ev))) {
        if (ret != ui_action) return true;
        ui->io_visible = !ui->io_visible;
        ui_panel_resize(ui->panel, ui_item_dim(ui));
        ui_panel_move(ui->panel, ui_item_pos(ui));
        return true;
    }

    if ((ret = ui_button_event(&ui->help, ev))) {
        if (ret != ui_action) return true;
        ui_item_event_help(ui);
        return true;
    }

    if ((ret = ui_link_event(&ui->id_val, ev))) {
        if (ret != ui_action) return true;
        ui_clipboard_copy_hex(&render.ui.board, ui->id);
        return true;
    }

    void *state = ui_item_state(ui, ui->id);
    const struct im_config *config = im_config_assert(im_id_item(ui->id));
    if (config->ui.event && config->ui.event(state, ev)) return true;

    if (ui->io_visible && (ret = ui_io_event(ui->io, ev))) return ret;

    return ui_panel_event_consume(ui->panel, ev);
}

void ui_item_render(struct ui_item *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;
    if (ui->loading) return;

    if (ui->io_visible) {
        struct ui_layout inner = ui_layout_split_x(&layout, ui_io_width());
        ui_io_render(ui->io, &inner, renderer);
        ui_layout_split_x(&layout, ui_st.font.dim.w);
    }

    ui_layout_dir(&layout, ui_layout_left);
    ui_button_render(&ui->help, &layout, renderer);
    ui_layout_dir(&layout, ui_layout_right);
    ui_button_render(&ui->io_toggle, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_layout_sep_row(&layout);

    ui_label_render(&ui->id_lbl, &layout, renderer);
    ui_link_render(&ui->id_val, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_layout_sep_row(&layout);

    const struct im_config *config = im_config_assert(im_id_item(ui->id));
    config->ui.render(ui_item_state(ui, ui->id), &layout, renderer);
}
