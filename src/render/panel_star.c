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
    const struct star *star;
    size_t coord_y;
    size_t star_y;
    size_t elem_y;

    struct {
        struct vec64 *list;
        struct ui_toggle *toggles;

        SDL_Rect abs;
        size_t rel_y;
        int toggle_h;
    } objs;
};

static struct font *panel_star_font_big(void) { return font_mono8; }
static struct font *panel_star_font_small(void) { return font_mono6; }

enum {
    panel_star_val_len = 4,
    panel_star_elems_cols = 5,
    panel_star_elems_lines = elem_natural_len / panel_star_elems_cols + 1,
};

size_t panel_star_width(void)
{
    size_t glyphs =
        (2 + panel_star_val_len) * panel_star_elems_cols + // values
        (panel_star_elems_cols - 1); // spacing

    return glyphs * panel_star_font_small()->glyph_w;
}


static void star_val_str(char *dst, size_t len, size_t val)
{
    assert(len >= panel_star_val_len);

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
        struct panel_star_state *state, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct font *font = panel_star_font_big();
    font_reset(font);

    char str[coord_str_len+1] = {0};
    coord_str(state->star->coord, str, sizeof(str));
    font_render(font, renderer, str, coord_str_len, (SDL_Point) {
                .x = rect->x, .y = rect->y + state->coord_y });
}

static void panel_star_render_power(
        struct panel_star_state *state, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct font *font = panel_star_font_small();
    font_reset(font);

    static const char star_str[] = "power:"; // \0 will act as the space
    SDL_Point pos = { .x = rect->x, .y = rect->y + state->star_y };

    sdl_err(SDL_SetTextureColorMod(font->tex, 0xFF, 0xFF, 0xFF));
    font_render(font, renderer, star_str, sizeof(star_str), pos);
    pos.x += sizeof(star_str) * font->glyph_w;


    char val_str[panel_star_val_len];
    star_val_str(val_str, sizeof(val_str), state->star->power);

    uint8_t gray = 0x11 * (u64_log2(state->star->power) / 2);
    sdl_err(SDL_SetTextureColorMod(font->tex, gray, gray, gray));

    font_render(font, renderer, val_str, sizeof(val_str), pos);
}

static void panel_star_render_elems(
        struct panel_star_state *state, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct font *font = panel_star_font_small();
    font_reset(font);

    SDL_Point pos = { .x = rect->x, .y = rect->y + state->elem_y };

    for (size_t elem = 0; elem < elem_natural_len; ++elem) {
        if (elem == elem_natural_len - 1) pos.x += font->glyph_w * 7 * 2;

        char elem_str[] = {'A'+elem, ':'};
        sdl_err(SDL_SetTextureColorMod(font->tex, 0xFF, 0xFF, 0xFF));
        font_render(font, renderer, elem_str, sizeof(elem_str), pos);
        pos.x += sizeof(elem_str) * font->glyph_w;

        uint16_t value = state->star->elements[elem];

        char val_str[panel_star_val_len];
        star_val_str(val_str, sizeof(val_str), value);

        uint8_t gray = 0x11 * u64_log2(value);
        sdl_err(SDL_SetTextureColorMod(font->tex, gray, gray, gray));

        if (value) font_render(font, renderer, val_str, sizeof(val_str), pos);
        pos.x += sizeof(val_str) * font->glyph_w;

        if (elem % panel_star_elems_cols == panel_star_elems_cols - 1) {
            pos.x = rect->x;
            pos.y += font->glyph_h;
        }
        else pos.x += font->glyph_w;
    }
}

static void panel_star_render_list(
        struct panel_star_state *state, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct font *font = panel_star_font_small();
    font_reset(font);

    SDL_Point pos = { .x = rect->x, .y = state->objs.rel_y };

    {
        static const char list_str[] = "objects:";
        font_render(font, renderer, list_str, sizeof(list_str), pos);

        char val_str[8];
        size_t n = str_utoa(state->objs.list->len, val_str, sizeof(val_str));
        pos.x = rect->w - (n * font->glyph_w);
        font_render(font, renderer, val_str, sizeof(val_str), pos);

        pos.x  = rect->x;
        pos.y += font->glyph_h;
    }

    for (size_t i = 0; i < state->objs.list->len; ++i) {
        ui_toggle_render(&state->objs.toggles[i], renderer, pos, font);
        pos.y += state->objs.toggle_h;
    }
}

static void panel_star_render(void *state_, SDL_Renderer *renderer, SDL_Rect *rect)
{
    struct panel_star_state *state = state_;
    panel_star_render_coord(state, renderer, rect);
    panel_star_render_power(state, renderer, rect);
    panel_star_render_elems(state, renderer, rect);
    panel_star_render_list(state, renderer, rect);
}

static void panel_star_str_id(id_t id, char *dst, size_t len)
{
    switch(id_item(id)) {
    case ITEM_BRAIN: { dst[0] = 'b'; break; }
    case ITEM_WORKER: { dst[0] = 'w'; break; }
    case ITEM_PRINTER: { dst[0] = 'p'; break; }
    case ITEM_LAB: { dst[0] = 'l'; break; }
    case ITEM_COMM: { dst[0] = 'c'; break; }
    case ITEM_SHIP: { dst[0] = 's'; break; }
    default: { assert(false); }
    }

    id = id_bot(id);
    assert(id);

    for (size_t j = len - 1; id; j--) {
        uint8_t v = id & 0xF;
        dst[j] = v < 10 ? '0' + v : 'A' + v;
        id >>= 4;
    }
}

static void panel_star_update(struct panel_star_state *state)
{
    struct hunk *hunk = sector_hunk(core.state.sector, state->star->coord);

    free(state->objs.list);
    state->objs.list = hunk_list(hunk);

    struct SDL_Rect rect = state->objs.abs;
    rect.h = state->objs.toggle_h;

    for (size_t i = 0; i < state->objs.list->len; ++i) {
        char str[8];
        panel_star_str_id(state->objs.list->vals[i], str, sizeof(str));

        ui_toggle_init(&state->objs.toggles[i], &rect, str, sizeof(str));
        rect.y += state->objs.toggle_h;
    }
}

static bool panel_star_events(void *state_, struct panel *panel, SDL_Event *event)
{
    struct panel_star_state *state = state_;

    if (event->type == core.event) {
        switch (event->user.code) {

        case EV_STAR_SELECT: {
            const struct star *star = event->user.data1;
            if (state->star && coord_eq(star->coord, state->star->coord))
                return false;

            state->star = star;
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
            state->star = NULL;
            panel_hide(panel);
            break;
        }

        default: { return false; }
        }
    }

    if (panel->hidden) return false;

    for (size_t i = 0; i < state->objs.list->len; ++i) {
        struct ui_toggle *toggle = &state->objs.toggles[i];

        enum ui_toggle_ret ret = ui_toggle_events(toggle, event);
        if (ret & ui_toggle_invalidate) panel_invalidate(panel);
        if (ret & ui_toggle_flip) {
            enum event ev = toggle->selected ? EV_OBJ_SELECT : EV_OBJ_CLEAR;
            id_t obj = state->objs.list->vals[i];
            uint64_t coord = coord_to_id(state->star->coord);
            core_push_event(ev, obj, coord);

            for (size_t j = 0; j < state->objs.list->len; ++j) {
                if (j != i) state->objs.toggles[j].selected = false;
            }
        }
        if (ret & ui_toggle_consume) return true;
    }
    return false;
}

static void panel_star_free(void *state_)
{
    struct panel_star_state *state = state_;
    vec64_free(state->objs.list);
    free(state->objs.toggles);
    free(state);
};

struct panel *panel_star_new(void)
{
    enum { spacing = 5 };

    struct font *font_big = panel_star_font_big();
    struct font *font_small = panel_star_font_small();

    size_t menu_h = panel_menu_height();
    size_t inner_w = panel_star_width();
    size_t inner_h = core.rect.h - menu_h - panel_total_padding;

    int outer_w = 0, outer_h = 0;
    panel_add_borders(inner_w, inner_h, &outer_w, &outer_h);

    struct panel_star_state *state = calloc(1, sizeof(*state));
    state->coord_y = 0;
    state->star_y = state->coord_y + font_big->glyph_h + spacing;
    state->elem_y = state->star_y + font_small->glyph_h + spacing;

    struct SDL_Rect rect = {
        .x = core.rect.w - outer_w,
        .y = panel_menu_height(),
        .w = outer_w, .h = outer_h };

    state->objs.list = vec64_reserve(0);
    state->objs.rel_y = state->elem_y +
        (panel_star_elems_lines * font_small->glyph_h) + spacing;
    state->objs.abs = (SDL_Rect) {
        .x = rect.x + panel_padding,
        .y = rect.y + state->objs.rel_y + font_small->glyph_h + panel_padding,
        .w = inner_w,
        .h = inner_h - state->objs.rel_y };
    ui_toggle_size(font_small, vm_atom_cap, NULL, &state->objs.toggle_h);

    struct panel *panel = panel_new(&rect);
    panel->hidden = true;
    panel->state = state;
    panel->render = panel_star_render;
    panel->events = panel_star_events;
    panel->free = panel_star_free;
    return panel;
}
