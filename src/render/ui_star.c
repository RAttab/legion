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
    struct ui_label power, power_val;
    struct ui_label elem, elem_val;

    struct ui_button tab_items;
    struct ui_scroll scroll;
    struct ui_toggles items;
};

static struct font *ui_star_font(void) { return font_mono6; }

static const char *ui_star_elems[] = {
    "A:", "B:", "C:", "D:", "E:",
    "F:", "G:", "H:", "I:", "J:",
    "K:", "L:", "M:", "N:", "O:",
    "P:"
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

        .power = ui_label_new(font, ui_str_c("power: ")),
        .power_val = ui_label_new(font, ui_str_v(str_scaled_len)),

        .elem = ui_label_new(font, ui_str_c(ui_star_elems[0])),
        .elem_val = ui_label_new(font, ui_str_v(str_scaled_len)),

        .tab_items = ui_button_new(font, ui_str_c("items")),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .items = ui_toggles_new(font, ui_str_v(vm_atom_cap)),
    };

    ui->panel.state = ui_panel_hidden;
    return ui;
}

void ui_star_free(struct ui_star *ui) {
    ui_panel_free(&ui->panel);
    ui_label_free(&ui->power);
    ui_label_free(&ui->power_val);
    ui_label_free(&ui->elem);
    ui_label_free(&ui->elem_val);
    ui_scroll_free(&ui->scroll);
    ui_toggles_free(&ui->items);
    free(ui);
}


int16_t ui_star_width(const struct ui_star *ui)
{
    return ui->panel.w.dim.w;
}

static void ui_star_update(struct ui_star *ui)
{
    struct chunk *chunk = sector_chunk(core.state.sector, ui->star.coord);
    if (!chunk) return;

    struct vec64 *items = chunk_list(chunk);
    ui_toggles_resize(&ui->items, items->len);
    ui_scroll_update(&ui->scroll, items->len);

    for (size_t i = 0; i < items->len; ++i) {
        id_t id = items->vals[i];
        struct ui_toggle *toggle = &ui->items.items[i];

        toggle->user = id;
        ui_str_set_id(&toggle->str, id);
    }

    {
        char str[coord_str_len+1] = {0};
        coord_str(ui->star.coord, str, sizeof(str));
        ui_str_setf(&ui->panel.title.str, "star - %s", str);
    }

    free(items);
}

static bool ui_star_event_user(struct ui_star *ui, SDL_Event *ev)
{
    switch (ev->user.code)
    {

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
        id_t id = (uintptr_t) ev->user.data1;
        ui_toggles_select(&ui->items, id);
        return false;
    }

    case EV_ITEM_CLEAR: {
        ui_toggles_clear(&ui->items);
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
    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;

    if ((ret = ui_button_event(&ui->tab_items, ev))) {
        // \todo
        return ret == ui_consume;
    }

    struct ui_toggle *toggle = NULL;
    if ((ret = ui_toggles_event(&ui->items, ev, &ui->scroll, &toggle, NULL))) {
        enum event type = toggle->state == ui_toggle_selected ? EV_ITEM_SELECT : EV_ITEM_CLEAR;
        core_push_event(type, toggle->user, coord_to_id(ui->star.coord));
        return true;
    }

    return false;
}

void ui_star_render(struct ui_star *ui, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&ui->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    struct font *font = ui_star_font();

    {
        ui_label_render(&ui->power, &layout, renderer);

        uint32_t value = ui->star.power;
        ui_str_set_scaled(&ui->power_val.str, value);
        ui->power_val.fg = rgba_gray(0x11 * (u64_log2(value) / 2));
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

            uint16_t value = ui->star.elements[i];
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

    {
        ui_button_render(&ui->tab_items, &layout, renderer);
        ui_layout_next_row(&layout);

        struct ui_layout inner = ui_scroll_render(&ui->scroll, &layout, renderer);
        if (ui_layout_is_nil(&inner)) return;

        ui_toggles_render(&ui->items, &inner, renderer, &ui->scroll);
    }
}
