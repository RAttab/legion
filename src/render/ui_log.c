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


// -----------------------------------------------------------------------------
// ui_log
// -----------------------------------------------------------------------------


struct ui_logi
{
    struct
    {
        struct coord star;
        id_t id;
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
    size_t len;

    struct ui_panel panel;
    struct ui_logi items[world_log_cap];
};

static struct font *ui_log_font(void) { return font_mono6; }


struct ui_logi ui_logi_new(void)
{
    struct font *font = ui_log_font();

    struct ui_logi ui = {
        .time = ui_label_new(font, ui_str_v(10)),
        .star = ui_link_new(font, ui_str_v(symbol_cap)),
        .id = ui_link_new(font, ui_str_v(id_str_len)),
        .key = ui_label_new(font, ui_str_v(symbol_cap)),
        .value = ui_label_new(font, ui_str_v(symbol_cap)),
    };

    ui.time.fg = rgba_gray(0x88);
    ui.time.bg = rgba_gray_a(0x44, 0x88);
    return ui;
}

struct ui_log *ui_log_new(void)
{
    struct font *font = ui_log_font();
    struct pos pos = make_pos(0, ui_topbar_height());
    struct dim dim = make_dim(
            (10 + symbol_cap * 3 + id_str_len + 3) * font->glyph_w,
            render.rect.h - pos.y - ui_status_height());

    struct ui_log *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_log) {
        .panel = ui_panel_title(pos, dim, ui_str_c("log")),
    };

    ui_panel_hide(&ui->panel);
    for (size_t i = 0; i < array_len(ui->items); ++i)
        ui->items[i] = ui_logi_new();

    return ui;
}


void ui_logi_free(struct ui_logi *ui)
{
    ui_label_free(&ui->time);
    ui_link_free(&ui->star);
    ui_link_free(&ui->id);
    ui_label_free(&ui->key);
    ui_label_free(&ui->value);
}

void ui_log_free(struct ui_log *ui)
{
    ui_panel_free(&ui->panel);
    for (size_t i = 0; i < array_len(ui->items); ++i)
        ui_logi_free(ui->items + i);
    free(ui);
}


static void ui_logi_update(struct ui_logi *ui, const struct logi *it)
{
    ui_str_set_u64(&ui->time.str, it->time);

    ui->state.star = it->star;
    word_t name = proxy_star_name(render.proxy, it->star);
    ui_str_set_atom(&ui->star.str, name);

    ui->state.id = it->id;
    ui_str_set_id(&ui->id.str, it->id);

    ui_str_set_atom(&ui->key.str, it->key);
    ui_str_set_atom(&ui->value.str, it->value);
}

static void ui_log_update(struct ui_log *ui)
{
    const struct log *logs = proxy_logs(render.proxy);

    if (!coord_is_nil(ui->coord)) {
        struct chunk *chunk = proxy_chunk(render.proxy, ui->coord);
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
}


static bool ui_log_event_user(struct ui_log *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui_panel_hide(&ui->panel);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        ui_log_update(ui);
        return false;
    }

    case EV_LOG_TOGGLE: {
        if (!ui_panel_is_visible(&ui->panel) || !coord_is_nil(ui->coord)) {
            ui->coord = coord_nil();
            ui_log_update(ui);
            ui_panel_show(&ui->panel);
        }
        else ui_panel_hide(&ui->panel);
        return false;
    }

    case EV_LOG_SELECT: {
        ui->coord = coord_from_u64((uintptr_t) ev->user.data1);
        ui_log_update(ui);
        ui_panel_show(&ui->panel);
        return false;
    }

    case EV_STAR_SELECT: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        if (coord_is_nil(ui->coord)) return false;
        ui->coord = coord_from_u64((uintptr_t) ev->user.data1);
        ui_log_update(ui);
        return false;
    }

    case EV_ITEM_SELECT: {
        if (!ui_panel_is_visible(&ui->panel)) return false;
        if (coord_is_nil(ui->coord)) return false;
        ui->coord = coord_from_u64((uintptr_t) ev->user.data2);
        ui_log_update(ui);
        return false;
    }

    case EV_TAPES_TOGGLE:
    case EV_TAPE_SELECT:
    case EV_MODS_TOGGLE:
    case EV_MOD_SELECT:
    case EV_STARS_TOGGLE: {
        ui_panel_hide(&ui->panel);
        return false;
    }

    default: { return false; }
    }
}

bool ui_logi_event(struct ui_logi *ui, struct coord star, SDL_Event *ev)
{
    enum ui_ret ret = ui_nil;

    if (coord_is_nil(star) && (ret = ui_link_event(&ui->star, ev))) {
        render_push_event(EV_STAR_SELECT, coord_to_u64(ui->state.star), 0);
        return true;
    }

    if ((ret = ui_link_event(&ui->id, ev))) {
        struct coord coord = coord_is_nil(star) ? ui->state.star : star;
        render_push_event(EV_ITEM_SELECT, ui->state.id, coord_to_u64(coord));
        return true;
    }

    return false;
}

bool ui_log_event(struct ui_log *ui, SDL_Event *ev)
{
    if (ev->type == render.event && ui_log_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret != ui_skip;

    for (size_t i = 0; i < ui->len; ++i) {
        if (ui_logi_event(ui->items + i, ui->coord, ev)) return true;
    }

    return ui_panel_event_consume(&ui->panel, ev);
}


void ui_logi_render(
        struct ui_logi *ui,
        struct ui_layout *layout,
        SDL_Renderer *renderer)
{
    struct font *font = ui_log_font();

    ui_label_render(&ui->time, layout, renderer);
    ui_layout_sep_x(layout, font->glyph_w);

    if (!coord_is_nil(ui->state.star))
        ui_link_render(&ui->star, layout, renderer);

    ui_link_render(&ui->id, layout, renderer);
    ui_label_render(&ui->key, layout, renderer);
    ui_layout_sep_x(layout, font->glyph_w);
    ui_label_render(&ui->value, layout, renderer);
}

void ui_log_render(struct ui_log *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    for (size_t i = 0; i < ui->len; ++i) {
        ui_logi_render(ui->items + i, &layout, renderer);
        ui_layout_next_row(&layout);
    }
}
