/* ui_star.c
   Rémi Attab (remi.attab@gmail.com), 19 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"
#include "ui/ui.h"
#include "utils/str.h"


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

struct ui_star
{
    struct star star;

    struct ui_panel panel;
    struct ui_label power, power_val;
    struct ui_label elem, elem_val;
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
    size_t width = (ui_star_elems_total_len + 1) * font->glyph_w;
    struct pos pos = make_pos(core.rect.w - width, ui_topbar_height(core.ui.topbar));
    struct dim dim = make_dim(width, core.rect.h - pos.y);

    struct ui_star *star = calloc(1, sizeof(*star));
    *star = (struct ui_star) {
        .panel = ui_panel_title(pos, dim, ui_str_v(coord_str_len + 7)),

        .power = ui_label_new(font, ui_str_c("power: ")),
        .power_val = ui_label_new(font, ui_str_v(str_scaled_len)),

        .elem = ui_label_new(font, ui_str_c(ui_star_elems[0])),
        .elem_val = ui_label_new(font, ui_str_v(str_scaled_len)),
    };

    star->panel.state = ui_panel_hidden;
    return star;

}

void ui_star_free(struct ui_star *star) {
    ui_panel_free(&star->panel);
    free(star);
}


int16_t ui_star_width(const struct ui_star *star)
{
    return star->panel.w.dim.w;
}

static void ui_star_update(struct ui_star *star)
{
    struct chunk *chunk = sector_chunk(core.state.sector, star->star.coord);
    if (!chunk) return;

    // \todo

    {
        char str[coord_str_len] = {0};
        coord_str(star->star.coord, str, sizeof(str));
        ui_str_setf(&star->panel.title.str, "star - %s", str);
    }

    return;
}

static bool ui_star_event_user(struct ui_star *star, SDL_Event *ev)
{
    switch (ev->user.code)
    {
        case EV_STAR_SELECT: {
            const struct star *new = ev->user.data1;
            star->star = *new;
            ui_star_update(star);
            star->panel.state = ui_panel_visible;
            core_push_event(EV_FOCUS, (uintptr_t) &star->panel, 0);
            return true;
        }

        case EV_STAR_CLEAR: {
            star->star = (struct star) {0};
            star->panel.state = ui_panel_hidden;
            core_push_event(EV_FOCUS, 0, 0);
            return true;
        }

        case EV_STATE_UPDATE: {
            if (star->panel.state == ui_panel_hidden) return false;
            ui_star_update(star);
            return false;
        }

    default: { return false; }
    }
}


bool ui_star_event(struct ui_star *star, SDL_Event *ev)
{
    if (ev->type == core.event && ui_star_event_user(star, ev)) return true;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_panel_event(&star->panel, ev))) return ret == ui_consume;

    return false;
}

void ui_star_render(struct ui_star *star, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_panel_render(&star->panel, renderer);
    if (ui_layout_is_nil(&layout)) return;

    struct font *font = ui_star_font();

    {
        ui_label_render(&star->power, &layout, renderer);

        uint32_t value = star->star.power;
        ui_str_set_scaled(&star->power_val.str, value);
        star->power_val.fg = rgba_gray(0x11 * (u64_log2(value) / 2));
        ui_label_render(&star->power_val, &layout, renderer);

        ui_layout_next_row(&layout);
        ui_layout_sep_y(&layout, font->glyph_h);
    }

    {
        for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i) {
            if (i == ITEMS_NATURAL_LEN-1)
                ui_layout_sep_x(&layout, (ui_star_elems_col_len+1)*2 * font->glyph_w);

            ui_str_setc(&star->elem.str, ui_star_elems[i]);
            ui_label_render(&star->elem, &layout, renderer);

            uint16_t value = star->star.elements[i];
            ui_str_set_scaled(&star->elem_val.str, value);
            star->elem_val.fg = rgba_gray(0x11 * u64_log2(value));
            ui_label_render(&star->elem_val, &layout, renderer);

            size_t col = i % 5;
            if (col < 4) ui_layout_sep_x(&layout, font->glyph_w);
            else ui_layout_next_row(&layout);
        }

        ui_layout_next_row(&layout);
        ui_layout_sep_y(&layout, font->glyph_h);
    }

}
