/* panel_star.c
   Rémi Attab (remi.attab@gmail.com), 15 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "render/font.h"
#include "render/panel.h"
#include "render/core.h"
#include "game/item.h"
#include "game/coord.h"
#include "game/galaxy.h"
#include "game/hunk.h"
#include "utils/log.h"
#include "utils/vec.h"
#include "SDL.h"


// -----------------------------------------------------------------------------
// panel star
// -----------------------------------------------------------------------------

struct panel_star_state
{
    struct layout *layout;
    struct star star;

    struct vec64 *objs;
    struct ui_toggle *toggles;
};

enum
{
    p_star_coord = 0,
    p_star_coord_sep,
    p_star_power,
    p_star_power_sep,
    p_star_elems,
    p_star_elems_sep,
    p_star_objs,
    p_star_objs_list,
    p_star_len,
};

enum
{
    p_star_val_len = 4,

    p_star_elem_len = 2 + p_star_val_len + 1,
    p_star_elems_cols = 5,
    p_star_elems_rows = 4,

    p_star_objs_len = 8,
};
static_assert(elem_natural_len <= p_star_elems_cols * p_star_elems_rows);

static const char p_star_power_str[] = "power:";
static const char p_star_objs_str[] = "objects:";

static void star_val_str(char *dst, size_t len, size_t val)
{
    assert(len >= p_star_val_len);

    static const char units[] = "ukMG?";

    size_t unit = 0;
    while (val > 1000) {
        val /= 1000;
        unit++;
    }

    dst[3] = units[unit];
    dst[2] = '0' + (val % 10);
    dst[1] = '0' + ((val / 10) % 10);
    dst[0] = '0' + ((val / 100) % 10);

    if (dst[0] == '0') {
        dst[0] = ' ';
        if (dst[1] == '0') dst[1] = ' ';
    }
}

static void panel_star_render_coord(
        struct panel_star_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_star_coord);

    char str[coord_str_len] = {0};
    coord_str(state->star.coord, str, sizeof(str));
    font_render(layout->font, renderer, str, coord_str_len, layout_entry_pos(layout));
}

static void panel_star_render_power(
        struct panel_star_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_star_power);

    font_render(layout->font, renderer, p_star_power_str, sizeof(p_star_power_str),
            layout_entry_pos(layout));

    char val_str[p_star_val_len] = {0};
    star_val_str(val_str, sizeof(val_str), state->star.power);

    uint8_t gray = 0x11 * (u64_log2(state->star.power) / 2);
    sdl_err(SDL_SetTextureColorMod(layout->font->tex, gray, gray, gray));
    font_render(layout->font, renderer, val_str, sizeof(val_str),
            layout_entry_index_pos(layout, 0, sizeof(p_star_power_str)));
}

static void panel_star_render_elems(
        struct panel_star_state *state, SDL_Renderer *renderer)
{
    struct layout_entry *layout = layout_entry(state->layout, p_star_elems);

    for (size_t elem = 0; elem < elem_natural_len; ++elem) {
        size_t row = elem / p_star_elems_cols;
        size_t col = elem % p_star_elems_cols;
        if (elem == elem_natural_len - 1) col = 2;
        SDL_Point pos = layout_entry_index_pos(layout, row, col * p_star_elem_len);

        char elem_str[] = {'A'+elem, ':'};
        sdl_err(SDL_SetTextureColorMod(layout->font->tex, 0xFF, 0xFF, 0xFF));
        font_render(layout->font, renderer, elem_str, sizeof(elem_str), pos);
        pos.x += sizeof(elem_str) * layout->item.w;

        uint16_t value = state->star.elements[elem];

        char val_str[p_star_val_len] = {0};
        star_val_str(val_str, sizeof(val_str), value);

        if (value) {
            uint8_t gray = 0x11 * u64_log2(value);
            sdl_err(SDL_SetTextureColorMod(layout->font->tex, gray, gray, gray));
            font_render(layout->font, renderer, val_str, sizeof(val_str), pos);
        }
    }
}

static void panel_star_render_list(
        struct panel_star_state *state, SDL_Renderer *renderer)
{
    {
        struct layout_entry *layout = layout_entry(state->layout, p_star_objs);
        SDL_Point pos = layout_entry_pos(layout);

        font_render(layout->font, renderer, p_star_objs_str, sizeof(p_star_objs_str),
                layout_entry_pos(layout));

        char val[p_star_objs_len];
        str_utoa(vec64_len(state->objs), val, sizeof(val));
        pos.x = state->layout->bbox.w - (layout->item.w * sizeof(val));
        font_render(layout->font, renderer, val, sizeof(val), pos);
    }

    {
        struct layout_entry *layout = layout_entry(state->layout, p_star_objs_list);

        size_t rows = u64_min(vec64_len(state->objs), layout->rows);
        for (size_t i = 0; i < rows; ++i) {
            SDL_Point pos = layout_entry_index_pos(layout, i, 0);
            ui_toggle_render(&state->toggles[i], renderer, pos, layout->font);
        }
    }
}

static void panel_star_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    (void) rect;

    struct panel_star_state *state = state_;
    panel_star_render_coord(state, renderer);
    panel_star_render_power(state, renderer);
    panel_star_render_elems(state, renderer);
    panel_star_render_list(state, renderer);
}

static void panel_star_reset(struct panel_star_state *state)
{
    free(state->toggles);
    state->toggles = NULL;

    vec64_free(state->objs);
    state->objs = NULL;
}

static void panel_star_update(struct panel_star_state *state)
{
    panel_star_reset(state);

    struct hunk *hunk = sector_hunk(core.state.sector, state->star.coord);
    if (!hunk) return;

    state->objs = hunk_list(hunk);
    state->toggles = calloc(vec64_len(state->objs), sizeof(*state->toggles));
    memcpy(&state->star, hunk_star(hunk), sizeof(state->star));

    struct layout_entry *layout = layout_entry(state->layout, p_star_objs_list);
    struct SDL_Rect rect = layout_abs(state->layout, p_star_objs_list);
    rect.h = layout->item.h;

    for (size_t i = 0; i < vec64_len(state->objs); ++i) {
        char str[id_str_len];
        id_str(state->objs->vals[i], sizeof(str), str);
        ui_toggle_init(&state->toggles[i], &rect, str, sizeof(str));

        rect.y += layout->item.h;
    }
}

static bool panel_star_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_star_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {

        case EV_STAR_SELECT: {
            const struct star *star = event->user.data1;
            if (coord_eq(star->coord, state->star.coord)) return false;
            memcpy(&state->star, star, sizeof(*star));
            panel_star_update(state);
            panel_show(panel);
            break;
        }

        case EV_STAR_UPDATE: {
            panel_star_update(state);
            panel_invalidate(panel);
            break;
        }

        case EV_STAR_CLEAR: {
            state->star = (struct star) {0};
            panel_star_reset(state);
            panel_hide(panel);
            break;
        }

        default: { return false; }
        }
    }

    if (panel->hidden) return false;

    for (size_t i = 0; i < vec64_len(state->objs); ++i) {
        struct ui_toggle *toggle = &state->toggles[i];

        enum ui_toggle_ret ret = ui_toggle_events(toggle, event);
        if (ret & ui_toggle_invalidate) panel_invalidate(panel);
        if (ret & ui_toggle_flip) {
            enum event ev = toggle->selected ? EV_OBJ_SELECT : EV_OBJ_CLEAR;
            id_t obj = state->objs->vals[i];
            uint64_t coord = coord_to_id(state->star.coord);
            core_push_event(ev, obj, coord);

            for (size_t j = 0; j < vec64_len(state->objs); ++j) {
                if (j != i) state->toggles[j].selected = false;
            }
        }
        if (ret & ui_toggle_consume) return true;
    }
    return false;
}

static void panel_star_free(void *state_)
{
    struct panel_star_state *state = state_;
    layout_free(state->layout);
    panel_star_reset(state);
    free(state);
};

struct panel *panel_star_new(void)
{
    struct font *font_b = font_mono10;
    struct font *font_s = font_mono8;
    size_t menu_h = panel_menu_height();

    struct layout *layout = layout_alloc(p_star_len,
            core.rect.w, core.rect.h - menu_h - panel_total_padding);

    layout_text(layout, p_star_coord, font_b, coord_str_len, 1);
    layout_sep(layout, p_star_coord_sep);

    layout_text(layout, p_star_power, font_s, sizeof(p_star_power_str) + p_star_val_len, 1);
    layout_sep(layout, p_star_power_sep);

    layout_text(layout, p_star_elems, font_s, p_star_elem_len * p_star_elems_cols, p_star_elems_rows);
    layout_sep(layout, p_star_elems_sep);

    layout_text(layout, p_star_objs, font_s, sizeof(p_star_objs) + p_star_objs_len, 1);
    layout_text(layout, p_star_objs_list, font_s, id_str_len + 2, layout_inf);

    layout_finish(layout, (SDL_Point) { .x = panel_padding, .y = panel_padding });
    layout->pos = (SDL_Point) {
        .x = core.rect.w - layout->bbox.w - panel_padding,
        .y = menu_h + panel_padding
    };

    struct panel_star_state *state = calloc(1, sizeof(*state));
    state->layout = layout;

    struct panel *panel = panel_new(&(SDL_Rect) {
                .x = layout->pos.x - panel_padding,
                .y = layout->pos.y - panel_padding,
                .w = layout->bbox.w + panel_total_padding,
                .h = core.rect.h - menu_h });
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_star_render;
    panel->events = panel_star_events;
    panel->free = panel_star_free;
    return panel;
}
