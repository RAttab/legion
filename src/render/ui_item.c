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

static struct font *ui_item_font(void) { return font_mono6; }

struct ui_item
{
    id_t id;
    struct coord star;
    bool loading;

    struct ui_panel panel;
    struct ui_button io;
    struct ui_label id_lbl;
    struct ui_link id_val;

    void *states[ITEMS_ACTIVE_LEN];
};

struct ui_item *ui_item_new(void)
{
    struct font *font = ui_item_font();

    size_t width = 38 * ui_item_font()->glyph_w;
    struct pos pos = make_pos(
            render.rect.w - width - ui_star_width(render.ui.star),
            ui_topbar_height());
    struct dim dim = make_dim(width, render.rect.h - pos.y - ui_status_height());

    struct ui_item *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_item) {
        .panel = ui_panel_title(pos, dim, ui_str_c("item")),
        .io = ui_button_new(font, ui_str_c("<< io")),
        .id_lbl = ui_label_new(font, ui_str_c("id: ")),
        .id_val = ui_link_new(font, ui_str_v(id_str_len)),
    };

    ui_panel_hide(&ui->panel);
    ui->io.w.dim.w = ui_layout_inf;

    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i) {
        const struct im_config *config = im_config(ITEM_ACTIVE_FIRST + i);
        if (config && config->ui.alloc) ui->states[i] = config->ui.alloc(font);
    }

    return ui;
}

void ui_item_free(struct ui_item *ui)
{
    ui_panel_free(&ui->panel);
    ui_button_free(&ui->io);
    ui_label_free(&ui->id_lbl);
    ui_link_free(&ui->id_val);

    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i) {
        const struct im_config *config = im_config(ITEM_ACTIVE_FIRST + i);
        if (config && config->ui.free) config->ui.free(ui->states[i]);
    }

    free(ui);
}

int16_t ui_item_width(struct ui_item *ui)
{
    return ui->panel.w.dim.w;
}

static void *ui_item_state(struct ui_item *ui, id_t id)
{
    void *state =  ui->states[id_item(id) - ITEM_ACTIVE_FIRST];
    assert(state);
    return state;
}

static void ui_item_update(struct ui_item *ui)
{
    if (!ui->id || coord_is_nil(ui->star)) return;

    ui_str_set_id(&ui->id_val.str, ui->id);

    struct chunk *chunk = proxy_chunk(render.proxy, ui->star);
    if ((ui->loading = !chunk)) return;

    void *state = ui_item_state(ui, ui->id);
    const struct im_config *config = im_config_assert(id_item(ui->id));

    config->ui.update(state, chunk, ui->id);
}

static bool ui_item_event_user(struct ui_item *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_panel_hide(&ui->panel);
        ui->id = 0;
        ui->star = coord_nil();
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        ui_item_update(ui);
        return false;
    }

    case EV_STAR_SELECT: {
        struct coord new = coord_from_u64((uintptr_t) ev->user.data1);
        if (!coord_eq(ui->star, new)) {
            ui_panel_hide(&ui->panel);
            ui->id = 0;
        }
        return false;
    }

    case EV_STAR_CLEAR: {
        ui_panel_hide(&ui->panel);
        ui->id = 0;
        return false;
    }

    case EV_ITEM_SELECT: {
        ui->id = (uintptr_t) ev->user.data1;
        ui->star = coord_from_u64((uintptr_t) ev->user.data2);
        ui_item_update(ui);
        ui_panel_show(&ui->panel);
        return false;
    }

    case EV_ITEM_CLEAR: {
        ui->id = 0;
        ui_panel_hide(&ui->panel);
        return false;
    }

    default: { return false; }
    }
}

bool ui_item_event(struct ui_item *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_item_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) {
        if (ret == ui_consume && !ui_panel_is_visible(&ui->panel))
            render_push_event(EV_ITEM_CLEAR, 0, 0);
        return ret != ui_skip;
    }

    if (ui->loading) return ui_panel_event_consume(&ui->panel, ev);

    if ((ret = ui_button_event(&ui->io, ev))) {
        render_push_event(EV_IO_TOGGLE, ui->id, coord_to_u64(ui->star));
        return true;
    }

    if ((ret = ui_link_event(&ui->id_val, ev))) {
        ui_clipboard_copy_hex(&render.ui.board, ui->id);
        return true;
    }

    void *state = ui_item_state(ui, ui->id);
    const struct im_config *config = im_config_assert(id_item(ui->id));
    if (config->ui.event && config->ui.event(state, ev)) return true;

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_item_render(struct ui_item *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;
    if (ui->loading) return;

    struct font *font = ui_item_font();

    ui_button_render(&ui->io, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_layout_sep_y(&layout, font->glyph_h);

    ui_label_render(&ui->id_lbl, &layout, renderer);
    ui_link_render(&ui->id_val, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_layout_sep_y(&layout, font->glyph_h);

    void *state = ui_item_state(ui, ui->id);
    const struct im_config *config = im_config_assert(id_item(ui->id));
    config->ui.render(state, &layout, renderer);
}
