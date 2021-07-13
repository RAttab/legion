/* ui_star.c
   Rémi Attab (remi.attab@gmail.com), 19 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "game/chunk.h"
#include "game/item.h"
#include "utils/vec.h"


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

struct ui_star
{
    struct star star;

    struct ui_panel panel;

    struct ui_label coord;
    struct ui_link coord_val;

    struct ui_label power, power_val;
    struct ui_label elem, elem_val;

    struct ui_button control, factory, logistic;

    id_t selected;
    struct ui_scroll control_scroll, factory_scroll;
    struct ui_toggles control_list, factory_list;

    struct ui_label workers, workers_val;
    struct ui_label idle, idle_val;
    struct ui_label fail, fail_val;
    struct ui_label queue, queue_val;
};

static struct font *ui_star_font(void) { return font_mono6; }

static const char *ui_star_elems[] = {
    "A:", "B:", "C:", "D:", "E:",
    "F:", "G:", "H:", "I:", "J:",
    "K:"
};

enum
{
    ui_star_elems_col_len = 2 + str_scaled_len,
    ui_star_elems_total_len = ui_star_elems_col_len * 5 + 4,
};

struct ui_star *ui_star_new(void)
{
    struct font *font = ui_star_font();
    size_t width = 35 * font->glyph_w;
    struct pos pos = make_pos(core.rect.w - width, ui_topbar_height(core.ui.topbar));
    struct dim dim = make_dim(width, core.rect.h - pos.y);

    struct ui_star *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_star) {
        .panel = ui_panel_title(pos, dim, ui_str_v(coord_str_len + 8)),

        .coord = ui_label_new(font, ui_str_c("coord: ")),
        .coord_val = ui_link_new(font, ui_str_v(coord_str_len)),

        .power = ui_label_new(font, ui_str_c("power: ")),
        .power_val = ui_label_new(font, ui_str_v(str_scaled_len)),

        .elem = ui_label_new(font, ui_str_c(ui_star_elems[0])),
        .elem_val = ui_label_new(font, ui_str_v(str_scaled_len)),

        .control = ui_button_new(font, ui_str_c("control")),
        .factory = ui_button_new(font, ui_str_c("factory")),
        .logistic = ui_button_new(font, ui_str_c("logistic")),

        .control_scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .control_list = ui_toggles_new(font, ui_str_v(id_str_len)),

        .factory_scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .factory_list = ui_toggles_new(font, ui_str_v(id_str_len)),

        .workers = ui_label_new(font, ui_str_c("workers: ")),
        .workers_val = ui_label_new(font, ui_str_v(10)),
        .idle = ui_label_new(font, ui_str_c("- idle:  ")),
        .idle_val = ui_label_new(font, ui_str_v(10)),
        .fail = ui_label_new(font, ui_str_c("- fail:  ")),
        .fail_val = ui_label_new(font, ui_str_v(10)),
        .queue = ui_label_new(font, ui_str_c("- queue: ")),
        .queue_val = ui_label_new(font, ui_str_v(10)),
    };

    ui->panel.state = ui_panel_hidden;
    ui->control.disabled = true;
    return ui;
}

void ui_star_free(struct ui_star *ui) {
    ui_panel_free(&ui->panel);
    ui_label_free(&ui->power);
    ui_label_free(&ui->power_val);
    ui_label_free(&ui->elem);
    ui_label_free(&ui->elem_val);
    ui_button_free(&ui->control);
    ui_button_free(&ui->factory);
    ui_button_free(&ui->logistic);
    ui_scroll_free(&ui->control_scroll);
    ui_scroll_free(&ui->factory_scroll);
    ui_toggles_free(&ui->control_list);
    ui_toggles_free(&ui->factory_list);
    ui_label_free(&ui->workers);
    ui_label_free(&ui->workers_val);
    ui_label_free(&ui->idle);
    ui_label_free(&ui->idle_val);
    ui_label_free(&ui->fail);
    ui_label_free(&ui->fail_val);
    ui_label_free(&ui->queue);
    ui_label_free(&ui->queue_val);
    free(ui);
}


int16_t ui_star_width(const struct ui_star *ui)
{
    return ui->panel.w.dim.w;
}

static void ui_star_update_list(
        struct ui_star *ui, struct chunk *chunk,
        struct ui_toggles *toggles, struct ui_scroll *scroll,
        const enum item *filter, size_t len)
{
    struct vec64 *ids = chunk_list_filter(chunk, filter, len);
    ui_toggles_resize(toggles, ids->len);
    ui_scroll_update(scroll, ids->len);

    for (size_t i = 0; i < ids->len; ++i) {
        id_t id = ids->vals[i];
        struct ui_toggle *toggle = &toggles->items[i];

        toggle->user = id;
        ui_str_set_id(&toggle->str, id);
    }

    ui_toggles_select(toggles, ui->selected);
    vec64_free(ids);
}

static void ui_star_update(struct ui_star *ui)
{
    {
        char str[coord_str_len+1] = {0};
        coord_str(ui->star.coord, str, sizeof(str));
        ui_str_setf(&ui->panel.title.str, "star - %s", str);
    }

    ui_str_set_coord(&ui->coord_val.str, ui->star.coord);

    struct chunk *chunk = world_chunk(core.state.world, ui->star.coord);
    if (!chunk) {
        ui_toggles_resize(&ui->control_list, 0);
        ui_scroll_update(&ui->control_scroll, 0);
        ui_toggles_resize(&ui->factory_list, 0);
        ui_scroll_update(&ui->factory_scroll, 0);

        ui_str_set_u64(&ui->workers_val.str, 0);
        ui_str_set_u64(&ui->idle_val.str, 0);
        ui_str_set_u64(&ui->fail_val.str, 0);
        ui_str_set_u64(&ui->queue_val.str, 0);
        return;
    }

    {
        static const enum item filter[] = {
            ITEM_DB_1, ITEM_DB_2, ITEM_DB_3,
            ITEM_BRAIN_1, ITEM_BRAIN_2, ITEM_BRAIN_3,
            ITEM_LEGION_1, ITEM_LEGION_2, ITEM_LEGION_3,
        };
        ui_star_update_list(
                ui, chunk,
                &ui->control_list, &ui->control_scroll,
                filter, array_len(filter));
    }

    {
        static const enum item filter[] = {
            ITEM_DEPLOY, ITEM_STORAGE,
            ITEM_EXTRACT_1, ITEM_EXTRACT_2, ITEM_EXTRACT_3,
            ITEM_PRINTER_1, ITEM_PRINTER_2, ITEM_PRINTER_3,
            ITEM_ASSEMBLY_1, ITEM_ASSEMBLY_2, ITEM_ASSEMBLY_3,
        };
        ui_star_update_list(
                ui, chunk,
                &ui->factory_list, &ui->factory_scroll,
                filter, array_len(filter));
    }

    {
        struct workers workers = chunk_workers(chunk);
        ui_str_set_u64(&ui->workers_val.str, workers.count);
        ui_str_set_u64(&ui->idle_val.str, workers.idle);
        ui_str_set_u64(&ui->fail_val.str, workers.fail);
        ui_str_set_u64(&ui->queue_val.str, workers.queue);
    }
}

static bool ui_star_event_user(struct ui_star *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        ui->panel.state = ui_panel_hidden;
        ui->star = (struct star) {0};
        ui->selected = 0;
        return false;
    }

    case EV_STAR_SELECT: {
        const struct star *new = ev->user.data1;
        ui->star = *new;
        ui_star_update(ui);
        ui->panel.state = ui_panel_visible;
        core_push_event(EV_FOCUS_PANEL, (uintptr_t) &ui->panel, 0);
        return true;
    }

    case EV_STAR_CLEAR: {
        ui->star = (struct star) {0};
        ui->panel.state = ui_panel_hidden;
        core_push_event(EV_FOCUS_PANEL, 0, 0);
        return true;
    }

    case EV_ITEM_SELECT: {
        ui->selected = (uintptr_t) ev->user.data1;
        ui_toggles_select(&ui->control_list, ui->selected);
        ui_toggles_select(&ui->factory_list, ui->selected);
        return false;
    }

    case EV_ITEM_CLEAR: {
        ui_toggles_clear(&ui->control_list);
        ui_toggles_clear(&ui->factory_list);
        return false;
    }

    case EV_STATE_UPDATE: {
        if (ui->panel.state == ui_panel_hidden) return false;
        ui_star_update(ui);
        return false;
    }

    default: { return false; }
    }
}


bool ui_star_event(struct ui_star *ui, SDL_Event *ev)
{
    if (ev->type == core.event && ui_star_event_user(ui, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&ui->panel, ev))) return ret == ui_consume;

    if ((ret = ui_link_event(&ui->coord_val, ev))) {
        ui_clipboard_copy_hex(&core.ui.board, coord_to_id(ui->star.coord));
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->control, ev))) {
        ui->control.disabled = true;
        ui->factory.disabled = ui->logistic.disabled = false;
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->factory, ev))) {
        ui->factory.disabled = true;
        ui->control.disabled = ui->logistic.disabled = false;
        return ret == ui_consume;
    }

    if ((ret = ui_button_event(&ui->logistic, ev))) {
        ui->logistic.disabled = true;
        ui->control.disabled = ui->factory.disabled = false;
        return ret == ui_consume;
    }


    if (ui->control.disabled) {
        if ((ret = ui_scroll_event(&ui->control_scroll, ev)))
            return ret == ui_consume;

        struct ui_toggle *toggle = NULL;
        if ((ret = ui_toggles_event(&ui->control_list, ev, &ui->control_scroll, &toggle, NULL))) {
            enum event type = toggle->state == ui_toggle_selected ?
                EV_ITEM_SELECT : EV_ITEM_CLEAR;
            core_push_event(type, toggle->user, coord_to_id(ui->star.coord));
            return true;
        }
    }

    if (ui->factory.disabled) {
        if ((ret = ui_scroll_event(&ui->factory_scroll, ev)))
            return ret == ui_consume;

        struct ui_toggle *toggle = NULL;
        if ((ret = ui_toggles_event(&ui->factory_list, ev, &ui->factory_scroll, &toggle, NULL))) {
            enum event type = toggle->state == ui_toggle_selected ?
                EV_ITEM_SELECT : EV_ITEM_CLEAR;
            core_push_event(type, toggle->user, coord_to_id(ui->star.coord));
            return true;
        }
    }

    return ui_panel_event_consume(&ui->panel, ev);
}

void ui_star_render(struct ui_star *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    struct font *font = ui_star_font();

    ui_label_render(&ui->coord, &layout, renderer);
    ui_link_render(&ui->coord_val, &layout, renderer);
    ui_layout_next_row(&layout);

    {
        ui_label_render(&ui->power, &layout, renderer);

        uint32_t value = ui->star.power;
        ui_str_set_scaled(&ui->power_val.str, value);
        ui->power_val.fg = rgba_gray(0x11 * u64_log2(value));
        ui_label_render(&ui->power_val, &layout, renderer);

        ui_layout_next_row(&layout);
        ui_layout_sep_y(&layout, font->glyph_h);
    }

    {
        for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i) {
            if (i == ITEMS_NATURAL_LEN-1)
                ui_layout_sep_x(&layout, (ui_star_elems_col_len+1)*2 * font->glyph_w);

            ui_str_setc(&ui->elem.str, ui_star_elems[i]);
            ui_label_render(&ui->elem, &layout, renderer);

            uint16_t value = ui->star.elems[i];
            ui_str_set_scaled(&ui->elem_val.str, value);
            ui->elem_val.fg = rgba_gray(0x11 * u64_log2(value));
            ui_label_render(&ui->elem_val, &layout, renderer);

            size_t col = i % 5;
            if (col < 4) ui_layout_sep_x(&layout, font->glyph_w);
            else ui_layout_next_row(&layout);
        }

        ui_layout_next_row(&layout);
        ui_layout_sep_y(&layout, font->glyph_h);
    }

    ui_button_render(&ui->control, &layout, renderer);
    ui_button_render(&ui->factory, &layout, renderer);
    ui_button_render(&ui->logistic, &layout, renderer);
    ui_layout_next_row(&layout);
    ui_layout_sep_y(&layout, font->glyph_h);

    if (ui->control.disabled) {
        struct ui_layout inner = ui_scroll_render(&ui->control_scroll, &layout, renderer);
        if (ui_layout_is_nil(&inner)) return;
        ui_toggles_render(&ui->control_list, &inner, renderer, &ui->control_scroll);
    }

    if (ui->factory.disabled) {
        struct ui_layout inner = ui_scroll_render(&ui->factory_scroll, &layout, renderer);
        if (ui_layout_is_nil(&inner)) return;
        ui_toggles_render(&ui->factory_list, &inner, renderer, &ui->factory_scroll);
    }

    if (ui->logistic.disabled) {
        ui_label_render(&ui->workers, &layout, renderer);
        ui_label_render(&ui->workers_val, &layout, renderer);
        ui_layout_next_row(&layout);

        ui_label_render(&ui->idle, &layout, renderer);
        ui_label_render(&ui->idle_val, &layout, renderer);
        ui_layout_next_row(&layout);

        ui_label_render(&ui->fail, &layout, renderer);
        ui_label_render(&ui->fail_val, &layout, renderer);
        ui_layout_next_row(&layout);

        ui_label_render(&ui->queue, &layout, renderer);
        ui_label_render(&ui->queue_val, &layout, renderer);
        ui_layout_next_row(&layout);
    }
}
