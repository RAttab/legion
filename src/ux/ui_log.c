/* ui_log.c
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ux/ui.h"
#include "game/log.h"
#include "game/chunk.h"
#include "game/world.h"

static void ui_log_free(void *);
static void ui_log_hide(void *);
static void ui_log_update(void *);
static void ui_log_event(void *);
static void ui_log_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// ui_log
// -----------------------------------------------------------------------------


struct ui_logi
{
    struct
    {
        struct coord star;
        im_id id;
    } state;

    struct ui_label time;
    struct ui_link star;
    struct ui_link id;
    struct ui_label key;
    struct ui_label value;
};

struct ui_log
{
    struct coord coord;
    struct dim cell;

    struct ui_panel *panel;
    struct ui_scroll scroll;

    size_t len;
    struct ui_logi items[world_log_cap];
};

struct ui_logi ui_logi_alloc(void)
{
    struct ui_logi ui = {
        .time = ui_label_new(ui_str_v(10)),
        .star = ui_link_new(ui_str_v(symbol_cap)),
        .id = ui_link_new(ui_str_v(im_id_str_len)),
        .key = ui_label_new(ui_str_v(symbol_cap)),
        .value = ui_label_new(ui_str_v(symbol_cap)),
    };

    return ui;
}

void ui_log_alloc(struct ui_view_state *state)
{
    struct dim cell = engine_cell();
    cell.h *= 2;

    struct ui_log *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_log) {
        .cell = cell,

        .panel = ui_panel_title(
                make_dim((10 + (symbol_cap * 2) + 4) * cell.w, ui_layout_inf),
                ui_str_c("log")),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), cell),
    };

    for (size_t i = 0; i < array_len(ui->items); ++i)
        ui->items[i] = ui_logi_alloc();

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_log,
        .slots = ui_slot_left,
        .panel = ui->panel,
        .fn = {
            .free = ui_log_free,
            .hide = ui_log_hide,
            .update_frame = ui_log_update,
            .event = ui_log_event,
            .render = ui_log_render,
        },
    };
}

static void ui_logi_free(struct ui_logi *ui)
{
    ui_label_free(&ui->time);
    ui_link_free(&ui->star);
    ui_link_free(&ui->id);
    ui_label_free(&ui->key);
    ui_label_free(&ui->value);
}

static void ui_log_free(void *state)
{
    struct ui_log *ui = state;

    ui_panel_free(ui->panel);
    ui_scroll_free(&ui->scroll);
    for (size_t i = 0; i < array_len(ui->items); ++i)
        ui_logi_free(ui->items + i);

    free(ui);
}

void ui_log_show(struct coord star)
{
    struct ui_log *ui = ui_state(ui_view_log);

    ui->coord = star;

    ui_log_update(ui);
    ui_show(ui_view_log);
}

static void ui_log_hide(void *state)
{
    struct ui_log *ui = state;
    ui->coord = coord_nil();
}


static void ui_logi_update(struct ui_logi *ui, const struct logi *it)
{
    ui_str_set_u64(&ui->time.str, it->time);

    ui->state.star = it->star;
    ui_str_set_coord_name(&ui->star.str, it->star);

    ui->state.id = it->id;
    ui_str_set_id(&ui->id.str, it->id);

    ui_str_set_atom(&ui->key.str, it->key);
    ui_str_set_atom(&ui->value.str, it->value);
}

static void ui_log_update(void *state)
{
    struct ui_log *ui = state;
    const struct log *logs = proxy_logs();

    if (!coord_is_nil(ui->coord)) {
        struct chunk *chunk = proxy_chunk(ui->coord);
        if (!chunk) { ui->len = 0; return; }
        logs = chunk_logs(chunk);
    }

    size_t index = 0;
    for (const struct logi *it = log_next(logs, NULL);
         it; index++, it = log_next(logs, it))
    {
        assert(index < array_len(ui->items));
        ui_logi_update(ui->items + index, it);
    }

    assert(index <= array_len(ui->items));
    ui->len = index;

    ui_scroll_update_rows(&ui->scroll, ui->len);
}


static void ui_logi_event(struct ui_logi *ui, struct coord star)
{
    if (coord_is_nil(star) && ui_link_event(&ui->star)) {
        ui_star_show(ui->state.star);
        ui_map_show(ui->state.star);
    }

    if (ui_link_event(&ui->id)) {
        struct coord coord = coord_is_nil(star) ? ui->state.star : star;
        ui_item_show(ui->state.id, coord);
        ui_map_goto(coord);
    }
}

static void ui_log_event(void *state)
{
    struct ui_log *ui = state;
    ui_scroll_event(&ui->scroll);

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        if (!coord_is_nil(ui->coord))
            ui_log_show(ev->star);


    size_t first = ui_scroll_first_row(&ui->scroll);
    size_t last = ui_scroll_last_row(&ui->scroll);
    for (size_t i = first; i < last; ++i)
        ui_logi_event(ui->items + i, ui->coord);
}


static void ui_logi_render(
        struct ui_log *ui,
        size_t row,
        struct ui_layout *layout)
{
    struct ui_logi *entry = ui->items + row;

    const render_layer layer = render_layer_push(1);

    {
        struct rect rect = {
            .x = layout->row.pos.x, .y = layout->row.pos.y,
            .w = layout->row.dim.w, .h = ui->cell.h,
        };

        struct rgba bg =
            ev_mouse_in(rect) ? ui_st.rgba.list.hover :
            row % 2 == 1 ? ui_st.rgba.list.selected :
            ui_st.rgba.bg;

        render_rect_fill(layer, bg, rect);
    }

    {
        ui_label_render(&entry->time, layout);
        ui_layout_sep_col(layout);

        if (!coord_is_nil(entry->state.star))
            ui_link_render(&entry->star, layout);
        else ui_layout_sep_x(layout, entry->star.w.w);
        ui_layout_sep_col(layout);

        ui_link_render(&entry->id, layout);
        ui_layout_next_row(layout);
    }

    {
        ui_layout_sep_x(layout, entry->time.w.w);
        ui_layout_sep_col(layout);

        ui_label_render(&entry->key, layout);
        ui_layout_sep_col(layout);

        ui_label_render(&entry->value, layout);
        ui_layout_next_row(layout);
    }

    render_layer_pop();
}

static void ui_log_render(void *state, struct ui_layout *layout)
{
    struct ui_log *ui = state;

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first_row(&ui->scroll);
    size_t last = ui_scroll_last_row(&ui->scroll);

    for (size_t row = first; row < last; ++row)
        ui_logi_render(ui, row, &inner);
}
