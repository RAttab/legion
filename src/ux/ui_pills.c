/* ui_pills.c
   Rémi Attab (remi.attab@gmail.com), 12 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ux/ui.h"
#include "game/chunk.h"
#include "game/pills.h"

static void ui_pills_free(void *);
static void ui_pills_update(void *);
static void ui_pills_event(void *);
static void ui_pills_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// pills
// -----------------------------------------------------------------------------

enum ui_pills_sort
{
    ui_pills_sort_nil = 0,
    ui_pills_sort_source,
    ui_pills_sort_cargo,
};

struct ui_pills_data
{
    struct coord source;
    struct cargo cargo;
};

struct ui_pills_row
{
    struct ui_link source_link;
    struct ui_label cargo_item;
    struct ui_label cargo_count;
};

struct ui_pills
{
    struct coord star;

    size_t len;
    enum ui_pills_sort sort;
    struct ui_pills_data *data;

    struct ui_panel *panel;
    struct ui_scroll scroll;
    struct ui_button source_title, cargo_title;
    struct ui_pills_row *rows;
};


void ui_pills_alloc(struct ui_view_state *state)
{
    struct dim cell = engine_cell();
    unit width = (symbol_cap + item_str_len + 1 + 3 + 2) * cell.w;

    struct ui_pills *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_pills) {
        .star = coord_nil(),
        .len = 0,
        .sort = ui_pills_sort_nil,

        .panel = ui_panel_title(make_dim(width, ui_layout_inf), ui_str_c("pills")),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), cell),
        .source_title = ui_button_new_s(&ui_st.button.list.close, ui_str_c("source")),
        .cargo_title = ui_button_new_s(&ui_st.button.list.close, ui_str_c("cargo")),
    };

    ui->source_title.w.w = symbol_cap * cell.w;
    ui->cargo_title.w.w = ui_layout_inf;

    ui->rows = calloc(pills_cap, sizeof(*ui->rows));
    ui->data = calloc(pills_cap, sizeof(*ui->data));

    for (size_t i = 0; i < pills_cap; ++i) {
        ui->rows[i] = (struct ui_pills_row) {
            .source_link = ui_link_new(ui_str_v(symbol_cap)),
            .cargo_item = ui_label_new(ui_str_v(item_str_len)),
            .cargo_count = ui_label_new(ui_str_v(3)),
        };
    }

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_pills,
        .parent = ui_view_star,
        .slots = ui_slot_right_sub,
        .panel = ui->panel,
        .fn = {
            .free = ui_pills_free,
            .update_frame = ui_pills_update,
            .event = ui_pills_event,
            .render = ui_pills_render,
        },
    };
}

static void ui_pills_free(void *state)
{
    struct ui_pills *ui = state;

    ui_panel_free(ui->panel);
    ui_scroll_free(&ui->scroll);
    ui_button_free(&ui->source_title);
    ui_button_free(&ui->cargo_title);

    for (size_t i = 0; i < pills_cap; ++i) {
        struct ui_pills_row *row = ui->rows + i;
        ui_link_free(&row->source_link);
        ui_label_free(&row->cargo_item);
        ui_label_free(&row->cargo_count);
    }

    free(ui->rows);
    free(ui->data);
    free(ui);
}

void ui_pills_show(struct coord star)
{
    struct ui_pills *ui = ui_state(ui_view_pills);

    ui->star = star;
    if (coord_is_nil(ui->star)) { ui_hide(ui_view_pills); return; }

    ui_pills_update(ui);
    ui_show(ui_view_pills);
}

static void ui_pills_sort(struct ui_pills *ui)
{
    int source_cmpv(const void *_lhs, const void *_rhs) {
        const struct ui_pills_data *lhs = _lhs;
        const struct ui_pills_data *rhs = _rhs;
        return coord_cmp(lhs->source, rhs->source);
    }

    int cargo_cmpv(const void *_lhs, const void *_rhs) {
        const struct ui_pills_data *lhs = _lhs;
        const struct ui_pills_data *rhs = _rhs;
        return cargo_cmp(lhs->cargo, rhs->cargo);
    }

    if (ui->sort == ui_pills_sort_source)
        qsort(ui->data, ui->len, sizeof(ui->data[0]), source_cmpv);
    else if (ui->sort == ui_pills_sort_cargo)
        qsort(ui->data, ui->len, sizeof(ui->data[0]), cargo_cmpv);

    for (size_t i = 0; i < ui->len; ++i) {
        struct ui_pills_data *data = ui->data + i;
        struct ui_pills_row *row = ui->rows + i;

        ui_str_set_coord_name(&row->source_link.str, data->source);
        ui_str_set_item(&row->cargo_item.str, data->cargo.item);
        ui_str_set_u64(&row->cargo_count.str, data->cargo.count);
    }
}

static void ui_pills_update(void *state)
{
    struct ui_pills *ui = state;

    struct chunk *chunk = proxy_chunk(ui->star);
    if (!chunk) { ui_pills_show(coord_nil()); return; }

    struct pills *pills = chunk_pills(chunk);

    ui->len = 0;
    size_t index = 0;
    struct pills_ret ret = {0};
    while ((ret = pills_next(pills, &index)).ok) {
        ui->data[ui->len] = (struct ui_pills_data) {
            .source = ret.coord,
            .cargo = ret.cargo,
        };
        ui->len++;
        index++;
    }

    ui_scroll_update_rows(&ui->scroll, ui->len);
    ui_pills_sort(ui);
}

static void ui_pills_event(void *state)
{
    struct ui_pills *ui = state;

    if (ui_button_event(&ui->source_title)) {
        if (ui->sort == ui_pills_sort_source) {
            ui->sort = ui_pills_sort_nil;
            ui->source_title.s = ui_st.button.list.close;
        }
        else {
            ui->sort = ui_pills_sort_source;
            ui->source_title.s = ui_st.button.list.open;
            ui->cargo_title.s = ui_st.button.list.close;
            ui_pills_sort(ui);
        }
    }

    if (ui_button_event(&ui->cargo_title)) {
        if (ui->sort == ui_pills_sort_cargo) {
            ui->sort = ui_pills_sort_nil;
            ui->cargo_title.s = ui_st.button.list.close;
        }
        else {
            ui->sort = ui_pills_sort_cargo;
            ui->cargo_title.s = ui_st.button.list.open;
            ui->source_title.s = ui_st.button.list.close;
            ui_pills_sort(ui);
        }
    }

    ui_scroll_event(&ui->scroll);

    size_t first = ui_scroll_first_row(&ui->scroll);
    size_t last = ui_scroll_last_row(&ui->scroll);
    for (size_t i = first; i < last; ++i) {
        if (ui_link_event(&ui->rows[i].source_link))
            ui_star_show(ui->data[i].source);
    }
}

static void ui_pills_render(void *state, struct ui_layout *layout)
{
    struct ui_pills *ui = state;

    ui_button_render(&ui->source_title, layout);
    ui_button_render(&ui->cargo_title, layout);
    ui_layout_next_row(layout);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout);
    size_t first = ui_scroll_first_row(&ui->scroll);
    size_t last = ui_scroll_last_row(&ui->scroll);

    for (size_t i = first; i < last; ++i) {
        struct ui_pills_row *row = ui->rows + i;

        ui_link_render(&row->source_link, &inner);
        ui_label_render(&row->cargo_item, &inner);
        ui_layout_sep_col(&inner);
        ui_label_render(&row->cargo_count, &inner);
        ui_layout_next_row(&inner);
    }
}
