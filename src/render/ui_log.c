/* ui_log.c
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/log.h"
#include "game/chunk.h"
#include "game/world.h"

static void ui_log_free(void *);
static void ui_log_hide(void *);
static void ui_log_update(void *);
static bool ui_log_event(void *, SDL_Event *);
static void ui_log_render(void *, struct ui_layout *, SDL_Renderer *);


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
    int16_t row_h;

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
    int16_t row_h = ui_st.font.dim.h * 2;
    struct dim dim = make_dim(
            (10 + (symbol_cap * 2) + 4) * ui_st.font.dim.w,
            ui_layout_inf);

    struct ui_log *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_log) {
        .row_h = row_h,
        .panel = ui_panel_title(dim, ui_str_c("log")),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), row_h),
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

    ui_scroll_update(&ui->scroll, ui->len);
}


static void ui_log_event_user(struct ui_log *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case ev_star_select: {
        if (!coord_is_nil(ui->coord))
            ui_log_show(coord_from_u64((uintptr_t) ev->user.data1));
        return;
    }

    default: { return; }
    }
}

static bool ui_logi_event(struct ui_logi *ui, struct coord star, SDL_Event *ev)
{
    enum ui_ret ret = ui_nil;

    if (coord_is_nil(star) && (ret = ui_link_event(&ui->star, ev))) {
        if (ret != ui_action) return true;
        ui_star_show(ui->state.star);
        ui_map_show(ui->state.star);
        return true;
    }

    if ((ret = ui_link_event(&ui->id, ev))) {
        if (ret != ui_action) return true;
        struct coord coord = coord_is_nil(star) ? ui->state.star : star;
        ui_item_show(ui->state.id, coord);
        ui_map_goto(coord);
        return true;
    }

    return false;
}

static bool ui_log_event(void *state, SDL_Event *ev)
{
    struct ui_log *ui = state;

    if (render_user_event(ev))
        ui_log_event_user(ui, ev);

    if (ui_scroll_event(&ui->scroll, ev)) return true;

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);
    for (size_t i = first; i < last; ++i)
        if (ui_logi_event(ui->items + i, ui->coord, ev))
            return true;

    return false;
}


static void ui_logi_render(
        struct ui_log *ui,
        size_t row,
        struct ui_layout *layout,
        SDL_Renderer *renderer)
{
    struct ui_logi *entry = ui->items + row;

    {
        SDL_Rect rect = {
            .x = layout->row.pos.x, .y = layout->row.pos.y,
            .w = layout->row.dim.w, .h = ui->row_h,
        };

        struct rgba rgba = ui_st.rgba.bg;

        if (ui_cursor_in(&rect))
            rgba = ui_st.rgba.list.hover;
        else if (row % 2 == 1)
            rgba = ui_st.rgba.list.selected;

        rgba_render(rgba, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }

    {
        ui_label_render(&entry->time, layout, renderer);
        ui_layout_sep_col(layout);

        if (!coord_is_nil(entry->state.star))
            ui_link_render(&entry->star, layout, renderer);
        else ui_layout_sep_x(layout, entry->star.w.dim.w);
        ui_layout_sep_col(layout);

        ui_link_render(&entry->id, layout, renderer);
        ui_layout_next_row(layout);
    }

    {
        ui_layout_sep_x(layout, entry->time.w.dim.w);
        ui_layout_sep_col(layout);

        ui_label_render(&entry->key, layout, renderer);
        ui_layout_sep_col(layout);

        ui_label_render(&entry->value, layout, renderer);
        ui_layout_next_row(layout);
    }
}

static void ui_log_render(
        void *state, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_log *ui = state;

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);

    for (size_t row = first; row < last; ++row)
        ui_logi_render(ui, row, &inner, renderer);
}
